/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/systemlink.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"

#ifdef XE_PLATFORM_WIN32
#include <winsock2.h>  // NOLINT(build/include_order)
#include <ws2tcpip.h>  // NOLINT(build/include_order)
#define poll WSAPoll
#define close_socket closesocket
using socket_t = SOCKET;
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#define close_socket close
using socket_t = int;
constexpr socket_t INVALID_SOCKET = -1;
#endif

DEFINE_bool(systemlink, false,
            "System link: play with the consoles on the local network (the "
            "same Wi-Fi), or named in systemlink_peers.",
            "Network");
DEFINE_string(systemlink_peers, "",
              "Consoles also reached directly, by IPv4 address, separated by "
              "commas: those on another subnet or a VPN (ZeroTier, Tailscale) "
              "that the local network's broadcasts don't reach. Naming one on "
              "either console is enough.",
              "Network");
DEFINE_transient_string(systemlink_address, "",
              "This console's IPv4 address on the network (the app sets it at "
              "launch); empty: the first Wi-Fi or Ethernet interface's.",
              "Network");
DEFINE_transient_string(systemlink_broadcast, "",
              "The local network's broadcast address and prefix, A.B.C.D/BITS "
              "(the app sets it at launch); empty: from the interface.",
              "Network");
DEFINE_transient_string(systemlink_mac, "",
              "This console's Ethernet address, XX:XX:XX:XX:XX:XX, its own on "
              "each device (the app keeps one); empty: one made from its IPv4 "
              "address.",
              "Network");

namespace xe {
namespace kernel {
namespace systemlink {

namespace {

constexpr uint32_t kAllOnes = 0xFFFFFFFFu;
constexpr uint32_t kLoopbackNet = 0x7F000000u;
// Ports below this need privileges; a bind there takes port + kPortShift.
constexpr uint16_t kPrivilegedPorts = 1024;
constexpr uint16_t kPortShift = 10000;
// Consoles heard from outside the local network: how many, and how long one
// stays without a word.
constexpr size_t kRemotesMax = 16;
constexpr auto kRemoteTtl = std::chrono::minutes(5);

using Clock = std::chrono::steady_clock;

struct Remote {
  uint32_t address;
  Clock::time_point seen;
};

struct State {
  uint32_t local = 0;
  uint32_t broadcast = kAllOnes;
  uint32_t mask = 0;  // 0: not known
  uint8_t mac[6] = {};
  std::vector<uint32_t> peers;

  std::mutex mutex;
  std::vector<Remote> remotes;
};

bool ParseAddress(const std::string& text, uint32_t* out) {
  in_addr a;
  if (inet_pton(AF_INET, text.c_str(), &a) != 1) {
    return false;
  }
  *out = ntohl(a.s_addr);
  return true;
}

std::string Trim(const std::string& s) {
  const auto begin = s.find_first_not_of(" \t");
  if (begin == std::string::npos) {
    return {};
  }
  return s.substr(begin, s.find_last_not_of(" \t") - begin + 1);
}

uint32_t PrefixMask(unsigned bits) {
  return bits == 0 ? 0 : bits >= 32 ? kAllOnes : kAllOnes << (32 - bits);
}

std::string AddressText(uint32_t address) {
  return fmt::format("{}.{}.{}.{}", address >> 24, (address >> 16) & 0xFF,
                     (address >> 8) & 0xFF, address & 0xFF);
}

// The interface to play on: the one with systemlink_address, else the first
// Wi-Fi one up, else any other one up that isn't loopback.
void FindInterface(State& s) {
#ifndef XE_PLATFORM_WIN32
  ifaddrs* list;
  if (getifaddrs(&list) != 0) {
    return;
  }
  const ifaddrs* chosen = nullptr;
  for (const ifaddrs* i = list; i; i = i->ifa_next) {
    if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET ||
        !(i->ifa_flags & IFF_UP) || (i->ifa_flags & IFF_LOOPBACK)) {
      continue;
    }
    const uint32_t address =
        ntohl(reinterpret_cast<const sockaddr_in*>(i->ifa_addr)->sin_addr.s_addr);
    if (s.local) {
      if (address == s.local) {
        chosen = i;
        break;
      }
      continue;
    }
    const bool wifi = i->ifa_name && !std::strncmp(i->ifa_name, "wlan", 4);
    if (!chosen || (wifi && std::strncmp(chosen->ifa_name, "wlan", 4))) {
      chosen = i;
    }
  }
  if (chosen) {
    s.local = ntohl(
        reinterpret_cast<const sockaddr_in*>(chosen->ifa_addr)->sin_addr.s_addr);
    if (!s.mask && chosen->ifa_netmask) {
      s.mask = ntohl(reinterpret_cast<const sockaddr_in*>(chosen->ifa_netmask)
                         ->sin_addr.s_addr);
      if (s.mask) {
        s.broadcast = (s.local & s.mask) | ~s.mask;
      }
    }
  }
  freeifaddrs(list);
#endif  // XE_PLATFORM_WIN32
}

State& Get() {
  static State state;
  static std::once_flag once;
  std::call_once(once, []() {
    State& s = state;
    if (!cvars::systemlink_address.empty() &&
        !ParseAddress(Trim(cvars::systemlink_address), &s.local)) {
      XELOGW("System link: systemlink_address {} is not an IPv4 address",
             cvars::systemlink_address);
    }

    const std::string bcast = Trim(cvars::systemlink_broadcast);
    if (!bcast.empty()) {
      const auto slash = bcast.find('/');
      uint32_t address;
      if (ParseAddress(bcast.substr(0, slash), &address)) {
        s.broadcast = address;
        if (slash != std::string::npos) {
          s.mask = PrefixMask(
              static_cast<unsigned>(std::strtoul(bcast.c_str() + slash + 1,
                                                 nullptr, 10)));
        }
      } else {
        XELOGW("System link: systemlink_broadcast {} is not A.B.C.D/BITS",
               bcast);
      }
    }

    FindInterface(s);

    std::string peers = cvars::systemlink_peers;
    size_t at = 0;
    while (at <= peers.size()) {
      size_t end = peers.find_first_of(", ;", at);
      if (end == std::string::npos) {
        end = peers.size();
      }
      // A port after the address is the title's own; the address is enough.
      const std::string item = Trim(peers.substr(at, end - at));
      const std::string host = item.substr(0, item.find(':'));
      uint32_t address;
      if (!host.empty()) {
        if (ParseAddress(host, &address)) {
          s.peers.push_back(address);
        } else {
          XELOGW("System link: peer {} is not an IPv4 address", item);
        }
      }
      at = end + 1;
    }

    unsigned mac[6];
    if (std::sscanf(cvars::systemlink_mac.c_str(),
                    "%2x:%2x:%2x:%2x:%2x:%2x", &mac[0], &mac[1], &mac[2],
                    &mac[3], &mac[4], &mac[5]) == 6) {
      for (int i = 0; i < 6; i++) {
        s.mac[i] = static_cast<uint8_t>(mac[i]);
      }
    } else {
      // Microsoft's prefix marked locally administered (no real console has
      // it), then the address: two devices on one network never share it.
      s.mac[0] = 0x02;
      s.mac[1] = 0x22;
      s.mac[2] = 0x48;
      s.mac[3] = static_cast<uint8_t>(s.local >> 16);
      s.mac[4] = static_cast<uint8_t>(s.local >> 8);
      s.mac[5] = static_cast<uint8_t>(s.local);
    }

    if (cvars::systemlink) {
      XELOGI(
          "System link: this console is {} ({:02X}:{:02X}:{:02X}:{:02X}:{:02X}:"
          "{:02X}) (.{}), broadcasts to {} (/{}), {} peers named",
          s.local ? AddressText(s.local) : std::string("not on a network"),
          s.mac[0], s.mac[1], s.mac[2], s.mac[3], s.mac[4], s.mac[5],
          s.local & 0xFF, AddressText(s.broadcast), __builtin_popcount(s.mask),
          s.peers.size());
    }
  });
  return state;
}

bool InLocalNetwork(const State& s, uint32_t address) {
  return s.mask && s.local && (address & s.mask) == (s.local & s.mask);
}

}  // namespace

bool Enabled() { return cvars::systemlink; }

uint32_t LocalAddress() { return Get().local; }

uint32_t BroadcastAddress() { return Get().broadcast; }

bool IsBroadcast(uint32_t address) {
  return address == kAllOnes || address == Get().broadcast;
}

bool IsLocalAddress(uint32_t address) {
  // The address broadcasts leave from. Asked of every datagram from a
  // console's port, so no interface listing here.
  return address == Get().local;
}

void MacAddress(uint8_t mac[6]) { std::memcpy(mac, Get().mac, 6); }

std::vector<uint32_t> BroadcastPeers() {
  State& s = Get();
  std::vector<uint32_t> out = s.peers;
  const auto now = Clock::now();
  std::lock_guard<std::mutex> lock(s.mutex);
  for (const Remote& r : s.remotes) {
    if (now - r.seen < kRemoteTtl &&
        std::find(out.begin(), out.end(), r.address) == out.end()) {
      out.push_back(r.address);
    }
  }
  return out;
}

void Heard(uint32_t address) {
  if (!Enabled()) {
    return;
  }
  State& s = Get();
  // Only those outside the local network need telling of broadcasts.
  if (!s.mask || InLocalNetwork(s, address) ||
      (address & 0xFF000000u) == kLoopbackNet || address == s.local) {
    return;
  }
  const auto now = Clock::now();
  std::lock_guard<std::mutex> lock(s.mutex);
  Remote* oldest = nullptr;
  for (Remote& r : s.remotes) {
    if (r.address == address) {
      r.seen = now;
      return;
    }
    if (!oldest || r.seen < oldest->seen) {
      oldest = &r;
    }
  }
  XELOGI("System link: console at {} (outside the local network, .{})",
         AddressText(address), address & 0xFF);
  if (s.remotes.size() < kRemotesMax) {
    s.remotes.push_back({address, now});
  } else {
    *oldest = {address, now};
  }
}

bool IsConsole(uint32_t address) {
  if ((address & 0xFF000000u) == kLoopbackNet) {
    return true;
  }
  if (!Enabled()) {
    return false;
  }
  State& s = Get();
  if (IsBroadcast(address) || address == s.local ||
      InLocalNetwork(s, address) ||
      std::find(s.peers.begin(), s.peers.end(), address) != s.peers.end()) {
    return true;
  }
  std::lock_guard<std::mutex> lock(s.mutex);
  for (const Remote& r : s.remotes) {
    if (r.address == address) {
      return true;
    }
  }
  return false;
}

uint16_t HostPort(uint16_t guest_port) {
  return guest_port != 0 && guest_port < kPrivilegedPorts
             ? static_cast<uint16_t>(guest_port + kPortShift)
             : guest_port;
}

uint16_t GuestPort(uint16_t host_port) {
  return host_port > kPortShift && host_port < kPortShift + kPrivilegedPorts
             ? static_cast<uint16_t>(host_port - kPortShift)
             : host_port;
}

std::string Describe(uint32_t address) {
  State& s = Get();
  const char* what =
      address == s.local                      ? "this console"
      : IsBroadcast(address)                  ? "broadcast"
      : (address & 0xFF000000u) == kLoopbackNet ? "loopback"
      : InLocalNetwork(s, address)            ? "local network"
      : std::find(s.peers.begin(), s.peers.end(), address) != s.peers.end()
          ? "named peer"
          : IsConsole(address) ? "console heard outside"
                               : "not a console";
  if (!IsConsole(address)) {
    // Its first byte too, to tell a mangled address from a server's.
    return fmt::format("{} {}.x.x.{}", what, address >> 24, address & 0xFF);
  }
  return fmt::format("{} .{}", what, address & 0xFF);
}

void Trace(bool sent, uint32_t address, uint16_t port, uint16_t local_port,
           const uint8_t* data, size_t size) {
  constexpr uint32_t kPerFlow = 3;
  constexpr size_t kBytesShown = 16;
  static std::mutex mutex;
  static std::map<uint64_t, uint32_t> flows;
  const uint64_t key = (uint64_t(sent) << 63) | (uint64_t(address) << 31) |
                       (uint64_t(port) << 15) | (local_port & 0x7FFF);
  uint32_t n;
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (flows.size() >= 256 && !flows.count(key)) {
      return;
    }
    n = flows[key]++;
  }
  if (n >= kPerFlow) {
    return;
  }
  std::string bytes;
  for (size_t i = 0; i < std::min(size, kBytesShown); i++) {
    bytes += fmt::format("{:02X}", data[i]);
  }
  XELOGI("System link: #{} {} {} port {} (title's {}), socket on port {}, {} "
         "bytes: {}{}",
         n + 1, sent ? "to" : "from", Describe(address), port,
         GuestPort(port), local_port, size, bytes,
         size > kBytesShown ? ".." : "");
}

namespace {

// A probe: magic, kind, sequence, the session's XNKID; an answer adds the
// data's length (big-endian) and the data.
constexpr uint8_t kQosMagic[4] = {'X', 'Q', 'O', 'S'};
constexpr uint8_t kQosProbe = 1;
constexpr uint8_t kQosAnswer = 2;
constexpr uint8_t kQosDisabled = 3;
constexpr size_t kQosHeader = 4 + 1 + 1 + 8;
constexpr size_t kQosDataMax = 1024;
constexpr unsigned kQosProbesMax = 8;
constexpr int kQosProbeTimeoutMs = 500;

struct Listener {
  bool enabled = false;
  std::vector<uint8_t> data;
  Clock::time_point changed;
};

std::mutex qos_mutex;
std::map<uint64_t, Listener> listeners;

uint64_t KidValue(const uint8_t kid[8]) {
  uint64_t v;
  std::memcpy(&v, kid, 8);
  return v;
}

bool IsQosPacket(const uint8_t* p, size_t len) {
  return len >= kQosHeader && !std::memcmp(p, kQosMagic, 4);
}

void AnswerProbes() {
  const socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    XELOGE("System link: no socket for QoS answers");
    return;
  }
  const int one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<const char*>(&one), sizeof(one));
  sockaddr_in local = {};
  local.sin_family = AF_INET;
  local.sin_port = htons(kQosPort);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
    XELOGE("System link: port {} for QoS answers is taken", kQosPort);
    close_socket(s);
    return;
  }
  XELOGI("System link: answering QoS probes on port {}", kQosPort);
  std::vector<uint8_t> in(kQosHeader + 2 + kQosDataMax);
  std::vector<uint8_t> out;
  for (;;) {
    sockaddr_in from = {};
    socklen_t from_len = sizeof(from);
    const auto len =
        recvfrom(s, reinterpret_cast<char*>(in.data()),
                 static_cast<int>(in.size()), 0,
                 reinterpret_cast<sockaddr*>(&from), &from_len);
    if (len < 0) {
      continue;
    }
    if (!IsQosPacket(in.data(), size_t(len)) || in[4] != kQosProbe) {
      continue;
    }
    out.assign(in.begin(), in.begin() + kQosHeader);
    {
      std::lock_guard<std::mutex> lock(qos_mutex);
      auto it = listeners.find(KidValue(&in[6]));
      if (it == listeners.end()) {
        // A title that probes with another key than the one it published
        // under still means this console's session: the latest one taking
        // players.
        for (auto i = listeners.begin(); i != listeners.end(); ++i) {
          if (i->second.enabled &&
              (it == listeners.end() || i->second.changed > it->second.changed)) {
            it = i;
          }
        }
      }
      const uint32_t prober = ntohl(from.sin_addr.s_addr);
      static std::vector<uint32_t> logged;
      if (std::find(logged.begin(), logged.end(), prober) == logged.end() &&
          logged.size() < 64) {
        logged.push_back(prober);
        XELOGI("System link: QoS probe from {}: {}", Describe(prober),
               it == listeners.end() ? "no session published, not answering"
               : it->second.enabled  ? "answering, taking players"
                                     : "answering, not taking players");
      }
      if (it == listeners.end()) {
        continue;
      }
      out[4] = it->second.enabled ? kQosAnswer : kQosDisabled;
      const auto& data = it->second.data;
      out.push_back(static_cast<uint8_t>(data.size() >> 8));
      out.push_back(static_cast<uint8_t>(data.size()));
      out.insert(out.end(), data.begin(), data.end());
    }
    sendto(s, reinterpret_cast<const char*>(out.data()),
           static_cast<int>(out.size()), 0,
           reinterpret_cast<sockaddr*>(&from), from_len);
  }
}

// The answering thread, started by the first session published.
void StartAnswering() {
  static std::once_flag once;
  std::call_once(once, []() { std::thread(AnswerProbes).detach(); });
}

}  // namespace

void QosSetEnabled(const uint8_t kid[8], bool enabled) {
  if (!Enabled()) {
    return;
  }
  StartAnswering();
  std::lock_guard<std::mutex> lock(qos_mutex);
  Listener& l = listeners[KidValue(kid)];
  l.enabled = enabled;
  l.changed = Clock::now();
}

void QosSetData(const uint8_t kid[8], const uint8_t* data, size_t size) {
  if (!Enabled()) {
    return;
  }
  StartAnswering();
  std::lock_guard<std::mutex> lock(qos_mutex);
  Listener& l = listeners[KidValue(kid)];
  l.data.assign(data, data + std::min(size, kQosDataMax));
  l.changed = Clock::now();
}

void QosRelease(const uint8_t kid[8]) {
  std::lock_guard<std::mutex> lock(qos_mutex);
  listeners.erase(KidValue(kid));
}

QosResult QosProbe(uint32_t address, const uint8_t kid[8], unsigned probes) {
  QosResult result;
  probes = std::clamp(probes, 1u, kQosProbesMax);
  const socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    return result;
  }
  sockaddr_in to = {};
  to.sin_family = AF_INET;
  to.sin_port = htons(kQosPort);
  to.sin_addr.s_addr = htonl(address);

  std::vector<uint8_t> probe(kQosHeader);
  std::memcpy(probe.data(), kQosMagic, 4);
  probe[4] = kQosProbe;
  std::memcpy(&probe[6], kid, 8);
  std::vector<uint8_t> in(kQosHeader + 2 + kQosDataMax);
  std::vector<Clock::time_point> sent(probes);
  std::vector<int> rtts;

  for (unsigned i = 0; i < probes; i++) {
    probe[5] = static_cast<uint8_t>(i);
    sent[i] = Clock::now();
    sendto(s, reinterpret_cast<const char*>(probe.data()),
           static_cast<int>(probe.size()), 0,
           reinterpret_cast<sockaddr*>(&to), sizeof(to));
    result.probes_sent++;
    // Wait for this probe's answer; an earlier one's counts too.
    const auto deadline =
        sent[i] + std::chrono::milliseconds(kQosProbeTimeoutMs);
    for (;;) {
      const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                            deadline - Clock::now())
                            .count();
      if (left <= 0) {
        break;
      }
      pollfd pfd = {};
      pfd.fd = s;
      pfd.events = POLLIN;
      if (poll(&pfd, 1, static_cast<int>(left)) <= 0) {
        break;
      }
      sockaddr_in from = {};
      socklen_t from_len = sizeof(from);
      const auto len = recvfrom(s, reinterpret_cast<char*>(in.data()),
                                static_cast<int>(in.size()), 0,
                                reinterpret_cast<sockaddr*>(&from), &from_len);
      if (len < 0 || !IsQosPacket(in.data(), size_t(len)) ||
          (in[4] != kQosAnswer && in[4] != kQosDisabled) || in[5] > i) {
        continue;
      }
      const unsigned seq = in[5];
      rtts.push_back(static_cast<int>(
          std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                sent[seq])
              .count()));
      result.probes_received++;
      result.contacted = true;
      result.disabled = in[4] == kQosDisabled;
      if (size_t(len) >= kQosHeader + 2) {
        const size_t size = std::min<size_t>(
            (size_t(in[kQosHeader]) << 8) | in[kQosHeader + 1],
            size_t(len) - kQosHeader - 2);
        result.data.assign(in.begin() + kQosHeader + 2,
                           in.begin() + kQosHeader + 2 + size);
      }
      if (seq == i) {
        break;
      }
    }
  }
  close_socket(s);

  if (!rtts.empty()) {
    std::sort(rtts.begin(), rtts.end());
    result.rtt_min_ms = static_cast<uint16_t>(rtts.front());
    result.rtt_med_ms = static_cast<uint16_t>(rtts[rtts.size() / 2]);
  }
  XELOGI("System link: QoS probe of {}: {}, {}/{} answers, {} bytes, {} ms",
         Describe(address),
         !result.contacted   ? "no answer"
         : result.disabled   ? "not taking players"
                             : "taking players",
         result.probes_received, result.probes_sent, result.data.size(),
         result.rtt_med_ms);
  return result;
}

}  // namespace systemlink
}  // namespace kernel
}  // namespace xe
