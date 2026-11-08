/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "xenia/base/logging.h"
#include "xenia/kernel/guest_scheduler.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/systemlink.h"
#include "xenia/kernel/util/shim_utils.h"
#include "xenia/kernel/xam/xam_module.h"
#include "xenia/kernel/xam/xam_private.h"
#include "xenia/kernel/xboxkrnl/xboxkrnl_error.h"
#include "xenia/kernel/xboxkrnl/xboxkrnl_threading.h"
#include "xenia/kernel/xevent.h"
#include "xenia/kernel/xsocket.h"
#include "xenia/kernel/xthread.h"
#include "xenia/xbox.h"

DEFINE_bool(network_enabled, true,
            "Let titles open host sockets. Without system link (systemlink) "
            "the XNet address and session layer is stubbed, so guest traffic "
            "cannot reach a peer, but titles that do not check for socket "
            "creation failure crash outright when this is off. Turn it off "
            "only for titles that block indefinitely on a socket that never "
            "answers.",
            "Kernel");

#ifdef XE_PLATFORM_WIN32
// NOTE: must be included last as it expects windows.h to already be included.
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // inet_addr
#endif
#include <winsock2.h>  // NOLINT(build/include_order)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#endif

namespace xe {
namespace kernel {
namespace xam {

// https://github.com/G91/TitanOffLine/blob/1e692d9bb9dfac386d08045ccdadf4ae3227bb5e/xkelib/xam/xamNet.h
enum {
  XNCALLER_INVALID = 0x0,
  XNCALLER_TITLE = 0x1,
  XNCALLER_SYSAPP = 0x2,
  XNCALLER_XBDM = 0x3,
  XNCALLER_TEST = 0x4,
  NUM_XNCALLER_TYPES = 0x4,
};

// https://github.com/pmrowla/hl2sdk-csgo/blob/master/common/xbox/xboxstubs.h
typedef struct {
  // FYI: IN_ADDR should be in network-byte order.
  in_addr ina;                   // IP address (zero if not static/DHCP)
  in_addr inaOnline;             // Online IP address (zero if not online)
  xe::be<uint16_t> wPortOnline;  // Online port
  uint8_t abEnet[6];             // Ethernet MAC address
  uint8_t abOnline[20];          // Online identification
} XNADDR;

// A session's key id. Its first byte says what kind: system link when the top
// three bits are clear (XNetXnKidIsSystemLink).
struct XNKID {
  uint8_t ab[8];
};
static_assert_size(XNKID, 8);

struct XNKEY {
  uint8_t ab[16];
};
static_assert_size(XNKEY, 16);

// System link (xenia/kernel/systemlink.h): a console's XNADDR carries its
// real address, so its IN_ADDR is that address, and the XNADDRs titles hand
// XNetXnAddrToInAddr are kept to answer XNetInAddrToXnAddr.
namespace {
std::mutex xnaddr_mutex;
std::map<uint32_t, XNADDR> xnaddrs;  // by IPv4 address, host order
XNKID registered_key = {};
bool key_registered = false;
uint16_t systemlink_port = 3074;

// XNetQosLookup results not yet released, which their probes may still fill.
std::mutex qos_mutex;
std::set<uint32_t> qos_lookups;

void RandomBytes(uint8_t* out, size_t count) {
  std::random_device rnd;
  std::mt19937_64 gen(rnd());
  std::uniform_int_distribution<int> dist(0,
                                          std::numeric_limits<uint8_t>::max());
  std::generate(out, out + count,
                [&]() { return static_cast<uint8_t>(dist(gen)); });
}
}  // namespace

struct XNDNS {
  xe::be<int32_t> status;
  xe::be<uint32_t> cina;
  in_addr aina[8];
};
static_assert_size(XNDNS, 0x28);

struct XNQOSINFO {
  uint8_t flags;
  uint8_t reserved;
  xe::be<uint16_t> probes_xmit;
  xe::be<uint16_t> probes_recv;
  xe::be<uint16_t> data_len;
  xe::be<uint32_t> data_ptr;
  xe::be<uint16_t> rtt_min_in_msecs;
  xe::be<uint16_t> rtt_med_in_msecs;
  xe::be<uint32_t> up_bits_per_sec;
  xe::be<uint32_t> down_bits_per_sec;
};
static_assert_size(XNQOSINFO, 0x18);

struct XNQOS {
  xe::be<uint32_t> count;
  xe::be<uint32_t> count_pending;
  XNQOSINFO info[1];
};

struct Xsockaddr_t {
  xe::be<uint16_t> sa_family;
  char sa_data[14];
};
static_assert_size(XNQOS, 0x20);

struct X_WSADATA {
  xe::be<uint16_t> version;
  xe::be<uint16_t> version_high;
  char description[256 + 1];
  char system_status[128 + 1];
  xe::be<uint16_t> max_sockets;
  xe::be<uint16_t> max_udpdg;
  xe::be<uint32_t> vendor_info_ptr;
};
static_assert_size(X_WSADATA, 0x190);

struct XWSABUF {
  xe::be<uint32_t> len;
  xe::be<uint32_t> buf_ptr;
};

struct XWSAOVERLAPPED {
  xe::be<uint32_t> internal;
  xe::be<uint32_t> internal_high;
  union {
    struct {
      xe::be<uint32_t> low;
      xe::be<uint32_t> high;
    } offset;  // must be named to avoid GCC error
    xe::be<uint32_t> pointer;
  };
  xe::be<uint32_t> event_handle;
};

void LoadSockaddr(const uint8_t* ptr, sockaddr* out_addr) {
  out_addr->sa_family = xe::load_and_swap<uint16_t>(ptr + 0);
  switch (out_addr->sa_family) {
    case AF_INET: {
      auto in_addr = reinterpret_cast<sockaddr_in*>(out_addr);
      in_addr->sin_port = xe::load_and_swap<uint16_t>(ptr + 2);
      // Maybe? Depends on type.
      in_addr->sin_addr.s_addr = *(uint32_t*)(ptr + 4);
      break;
    }
    default:
      assert_unhandled_case(out_addr->sa_family);
      break;
  }
}

void StoreSockaddr(const sockaddr& addr, uint8_t* ptr) {
  switch (addr.sa_family) {
    case AF_UNSPEC:
      std::memset(ptr, 0, sizeof(addr));
      break;
    case AF_INET: {
      auto& in_addr = reinterpret_cast<const sockaddr_in&>(addr);
      xe::store_and_swap<uint16_t>(ptr + 0, in_addr.sin_family);
      xe::store_and_swap<uint16_t>(ptr + 2, in_addr.sin_port);
      // Maybe? Depends on type.
      xe::store_and_swap<uint32_t>(ptr + 4, in_addr.sin_addr.s_addr);
      break;
    }
    default:
      assert_unhandled_case(addr.sa_family);
      break;
  }
}

// https://github.com/joolswills/mameox/blob/master/MAMEoX/Sources/xbox_Network.cpp#L136
struct XNetStartupParams {
  uint8_t cfgSizeOfStruct;
  uint8_t cfgFlags;
  uint8_t cfgSockMaxDgramSockets;
  uint8_t cfgSockMaxStreamSockets;
  uint8_t cfgSockDefaultRecvBufsizeInK;
  uint8_t cfgSockDefaultSendBufsizeInK;
  uint8_t cfgKeyRegMax;
  uint8_t cfgSecRegMax;
  uint8_t cfgQosDataLimitDiv4;
  uint8_t cfgQosProbeTimeoutInSeconds;
  uint8_t cfgQosProbeRetries;
  uint8_t cfgQosSrvMaxSimultaneousResponses;
  uint8_t cfgQosPairWaitTimeInSeconds;
};
static_assert_size(XNetStartupParams, kStartupParamsSize);

dword_result_t NetDll_XNetStartup_entry(dword_t caller,
                                        pointer_t<XNetStartupParams> params) {
  XELOGI("NetDll_XNetStartup: caller={}, params={:08X}", caller.value(),
         params.guest_address());

  auto xam = kernel_state()->GetKernelModule<XamModule>("xam.xex");
  std::span<const uint8_t> given;
  if (params) {
    given = {reinterpret_cast<const uint8_t*>(params.host_address()),
             sizeof(XNetStartupParams)};
  }
  xam->StartXNet(false, given);

  /*
  if (!xam->xnet()) {
    auto xnet = new XNet(kernel_state());
    xnet->Initialize();

    xam->set_xnet(xnet);
  }
  */

  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetStartup, kNetworking, kImplemented);

// https://github.com/jogolden/testdev/blob/master/xkelib/syssock.h#L46
dword_result_t NetDll_XNetStartupEx_entry(dword_t caller,
                                          pointer_t<XNetStartupParams> params,
                                          dword_t versionReq) {
  // versionReq
  // MW3, Ghosts: 0x20501400

  return NetDll_XNetStartup_entry(caller, params);
}
DECLARE_XAM_EXPORT1(NetDll_XNetStartupEx, kNetworking, kImplemented);

dword_result_t NetDll_XNetCleanup_entry(dword_t caller, lpvoid_t params) {
  auto xam = kernel_state()->GetKernelModule<XamModule>("xam.xex");
  xam->CleanupXNet(false);
  // auto xnet = xam->xnet();
  // xam->set_xnet(nullptr);

  // TODO: Shut down and delete.
  // delete xnet;

  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetCleanup, kNetworking, kImplemented);

dword_result_t NetDll_XNetGetOpt_entry(dword_t one, dword_t option_id,
                                       lpvoid_t buffer_ptr,
                                       lpdword_t buffer_size) {
  assert_true(one == 1);
  switch (option_id) {
    case 1: {
      if (*buffer_size < sizeof(XNetStartupParams)) {
        *buffer_size = sizeof(XNetStartupParams);
        return uint32_t(X_WSAError::X_WSAEMSGSIZE);
      }
      const auto params = kernel_state()
                              ->GetKernelModule<XamModule>("xam.xex")
                              ->xnet_startup_params();
      std::memcpy(buffer_ptr, params.data(), params.size());
      *buffer_size = sizeof(XNetStartupParams);
      return 0;
    }
    default:
      XELOGE("NetDll_XNetGetOpt: option {} unimplemented",
             static_cast<uint32_t>(option_id));
      return uint32_t(X_WSAError::X_WSAEINVAL);
  }
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetOpt, kNetworking, kSketchy);

dword_result_t NetDll_XNetRandom_entry(dword_t caller, lpvoid_t buffer_ptr,
                                       dword_t length) {
  uint8_t* buffer_data_ptr = buffer_ptr.as<uint8_t*>();

  if (buffer_data_ptr == nullptr || length == 0) {
    return X_ERROR_SUCCESS;
  }

  std::random_device rnd;
  std::mt19937_64 gen(rnd());
  std::uniform_int_distribution<int> dist(0,
                                          std::numeric_limits<uint8_t>::max());

  std::generate(buffer_data_ptr, buffer_data_ptr + length,
                [&]() { return static_cast<uint8_t>(dist(gen)); });

  return X_ERROR_SUCCESS;
}
DECLARE_XAM_EXPORT1(NetDll_XNetRandom, kNetworking, kImplemented);

dword_result_t NetDll_WSAStartup_entry(dword_t caller, word_t version,
                                       pointer_t<X_WSADATA> data_ptr) {
  // TODO(benvanik): abstraction layer needed.
  XELOGI("NetDll_WSAStartup: version={:04X}, data_ptr={:08X}", version.value(),
         data_ptr.guest_address());

  kernel_state()->GetKernelModule<XamModule>("xam.xex")->StartXNet(true, {});

  int ret = 0;

#ifdef XE_PLATFORM_WIN32
  WSADATA wsaData = {};

  ret = WSAStartup(version, &wsaData);
#endif

  if (data_ptr) {
    auto data_out = kernel_state()->memory()->TranslateVirtual(data_ptr);

#ifdef XE_PLATFORM_WIN32
    data_ptr->version = wsaData.wVersion;
    data_ptr->version_high = wsaData.wHighVersion;
#else
    data_ptr->version = version.value();
    data_ptr->version_high = 0x0202;
#endif

    // Some games (5841099F) want this value round-tripped - they'll compare if
    // it changes and bugcheck if it does.
    // vendor_info_ptr is at offset 0x18A (after max_udpdg at 0x188)
    uint32_t vendor_ptr = xe::load_and_swap<uint32_t>(data_out + 0x18A);
    xe::store_and_swap<uint32_t>(data_out + 0x18A, vendor_ptr);
  }

  // DEBUG
  /*
  auto xam = kernel_state()->GetKernelModule<XamModule>("xam.xex");
  if (!xam->xnet()) {
    auto xnet = new XNet(kernel_state());
    xnet->Initialize();

    xam->set_xnet(xnet);
  }
  */

  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_WSAStartup, kNetworking, kImplemented);

dword_result_t NetDll_WSAStartupEx_entry(dword_t caller, word_t version,
                                         pointer_t<X_WSADATA> data_ptr,
                                         dword_t versionReq) {
  return NetDll_WSAStartup_entry(caller, version, data_ptr);
}
DECLARE_XAM_EXPORT1(NetDll_WSAStartupEx, kNetworking, kImplemented);

dword_result_t NetDll_WSACleanup_entry(dword_t caller) {
  // Host WSA keeps running, Xenia needs it.
  kernel_state()->GetKernelModule<XamModule>("xam.xex")->CleanupXNet(true);
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_WSACleanup, kNetworking, kImplemented);

// Instead of using dedicated storage for WSA error like on OS.
// Xbox shares space between normal error codes and WSA errors.
// This under the hood returns directly value received from RtlGetLastError.
dword_result_t NetDll_WSAGetLastError_entry() {
  return XThread::GetLastError();
}
DECLARE_XAM_EXPORT1(NetDll_WSAGetLastError, kNetworking, kImplemented);

dword_result_t NetDll_WSARecvFrom_entry(
    dword_t caller, dword_t socket, pointer_t<XWSABUF> buffers_ptr,
    dword_t buffer_count, lpdword_t num_bytes_recv, lpdword_t flags_ptr,
    pointer_t<XSOCKADDR_IN> from_addr, pointer_t<XWSAOVERLAPPED> overlapped_ptr,
    lpvoid_t completion_routine_ptr) {
  if (overlapped_ptr) {
    // auto evt = kernel_state()->object_table()->LookupObject<XEvent>(
    //    overlapped_ptr->event_handle);

    // if (evt) {
    //  //evt->Set(0, false);
    //}
  }

  // we're not going to be receiving packets any time soon
  // return error so we don't wait on that - Cancerous
  return -1;
}
DECLARE_XAM_EXPORT2(NetDll_WSARecvFrom, kNetworking, kStub, kHighFrequency);

// If the socket is a VDP socket, buffer 0 is the game data length, and buffer 1
// is the unencrypted game data.
dword_result_t NetDll_WSASendTo_entry(
    dword_t caller, dword_t socket_handle, pointer_t<XWSABUF> buffers,
    dword_t num_buffers, lpdword_t num_bytes_sent, dword_t flags,
    pointer_t<XSOCKADDR_IN> to_ptr, dword_t to_len,
    pointer_t<XWSAOVERLAPPED> overlapped, lpvoid_t completion_routine) {
  assert(!overlapped);
  assert(!completion_routine);

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  // Our sockets implementation doesn't support multiple buffers, so we need
  // to combine the buffers the game has given us!
  std::vector<uint8_t> combined_buffer_mem;
  uint32_t combined_buffer_size = 0;
  uint32_t combined_buffer_offset = 0;
  for (uint32_t i = 0; i < num_buffers; i++) {
    combined_buffer_size += buffers[i].len;
    combined_buffer_mem.resize(combined_buffer_size);
    uint8_t* combined_buffer = combined_buffer_mem.data();

    std::memcpy(combined_buffer + combined_buffer_offset,
                kernel_memory()->TranslateVirtual(buffers[i].buf_ptr),
                buffers[i].len);
    combined_buffer_offset += buffers[i].len;
  }

  // A NULL destination sends to the connected peer.
  N_XSOCKADDR_IN native_to;
  N_XSOCKADDR_IN* to = nullptr;
  if (to_ptr) {
    native_to = *to_ptr;
    to = &native_to;
  }
  int ret = socket->SendTo(combined_buffer_mem.data(), combined_buffer_size,
                           flags, to, to_len);
  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
    return -1;
  }
  if (num_bytes_sent) {
    *num_bytes_sent = static_cast<uint32_t>(ret);
  }

  // TODO: Instantly complete overlapped

  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_WSASendTo, kNetworking, kImplemented);

dword_result_t NetDll_WSAWaitForMultipleEvents_entry(dword_t num_events,
                                                     lpdword_t events,
                                                     dword_t wait_all,
                                                     dword_t timeout,
                                                     dword_t alertable) {
  if (num_events > 64) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSA_INVALID_PARAMETER));
    return ~0u;
  }

  uint64_t timeout_wait = (uint64_t)timeout;

  X_STATUS result = 0;
  do {
    result = xboxkrnl::xeNtWaitForMultipleObjectsEx(
        num_events, events,
        wait_all ? X_KWAIT_REASON::WaitAll : X_KWAIT_REASON::WaitAny, 1,
        alertable, timeout != -1 ? &timeout_wait : nullptr);
  } while (result == X_STATUS_ALERTED);

  if (XFAILED(result)) {
    uint32_t error = xboxkrnl::xeRtlNtStatusToDosError(result);
    XThread::SetLastError(error);
    return ~0u;
  }
  // The signaled event's index, WSA_WAIT_TIMEOUT or WSA_WAIT_IO_COMPLETION.
  return result;
}
DECLARE_XAM_EXPORT2(NetDll_WSAWaitForMultipleEvents, kNetworking, kImplemented,
                    kBlocking);

dword_result_t NetDll_WSACreateEvent_entry() {
  auto ev = object_ref<XEvent>(new XEvent(kernel_state()));
  ev->Initialize(true, false);
  return ev->handle();
}
DECLARE_XAM_EXPORT1(NetDll_WSACreateEvent, kNetworking, kImplemented);

dword_result_t NetDll_WSACloseEvent_entry(dword_t event_handle) {
  X_STATUS result = kernel_state()->object_table()->ReleaseHandle(event_handle);
  if (XFAILED(result)) {
    uint32_t error = xboxkrnl::xeRtlNtStatusToDosError(result);
    XThread::SetLastError(error);
    return 0;
  }
  return 1;
}
DECLARE_XAM_EXPORT1(NetDll_WSACloseEvent, kNetworking, kImplemented);

dword_result_t NetDll_WSAResetEvent_entry(dword_t event_handle) {
  X_STATUS result = xboxkrnl::xeNtClearEvent(event_handle);
  if (XFAILED(result)) {
    uint32_t error = xboxkrnl::xeRtlNtStatusToDosError(result);
    XThread::SetLastError(error);
    return 0;
  }
  return 1;
}
DECLARE_XAM_EXPORT1(NetDll_WSAResetEvent, kNetworking, kImplemented);

dword_result_t NetDll_WSASetEvent_entry(dword_t event_handle) {
  X_STATUS result = xboxkrnl::xeNtSetEvent(event_handle, nullptr);
  if (XFAILED(result)) {
    uint32_t error = xboxkrnl::xeRtlNtStatusToDosError(result);
    XThread::SetLastError(error);
    return 0;
  }
  return 1;
}
DECLARE_XAM_EXPORT1(NetDll_WSASetEvent, kNetworking, kImplemented);

struct XnAddrStatus {
  // Address acquisition is not yet complete
  static constexpr uint32_t XNET_GET_XNADDR_PENDING = 0x00000000;
  // XNet is uninitialized or no debugger found
  static constexpr uint32_t XNET_GET_XNADDR_NONE = 0x00000001;
  // Host has ethernet address (no IP address)
  static constexpr uint32_t XNET_GET_XNADDR_ETHERNET = 0x00000002;
  // Host has statically assigned IP address
  static constexpr uint32_t XNET_GET_XNADDR_STATIC = 0x00000004;
  // Host has DHCP assigned IP address
  static constexpr uint32_t XNET_GET_XNADDR_DHCP = 0x00000008;
  // Host has PPPoE assigned IP address
  static constexpr uint32_t XNET_GET_XNADDR_PPPOE = 0x00000010;
  // Host has one or more gateways configured
  static constexpr uint32_t XNET_GET_XNADDR_GATEWAY = 0x00000020;
  // Host has one or more DNS servers configured
  static constexpr uint32_t XNET_GET_XNADDR_DNS = 0x00000040;
  // Host is currently connected to online service
  static constexpr uint32_t XNET_GET_XNADDR_ONLINE = 0x00000080;
  // Network configuration requires troubleshooting
  static constexpr uint32_t XNET_GET_XNADDR_TROUBLESHOOT = 0x00008000;
};

// A console's machine id, by which titles tell the consoles in a party apart:
// with system link, made from its Ethernet address (its own on each device),
// under the 0xFA prefix real machine ids have.
static uint64_t MachineIdFromEnet(const uint8_t enet[6]) {
  uint64_t id = 0xFA00ull << 48;
  for (int i = 0; i < 6; i++) {
    id |= uint64_t(enet[i]) << (8 * (5 - i));
  }
  return id;
}

dword_result_t NetDll_XNetGetTitleXnAddr_entry(dword_t caller,
                                               pointer_t<XNADDR> addr_ptr) {
  if (systemlink::Enabled()) {
    // This console as the others on the network reach it.
    const uint32_t local = systemlink::LocalAddress();
    addr_ptr.Zero();
    if (!local) {
      // No network yet (Wi-Fi off): what a console without a cable says.
      return XnAddrStatus::XNET_GET_XNADDR_ETHERNET;
    }
    addr_ptr->ina.s_addr = htonl(local);
    systemlink::MacAddress(addr_ptr->abEnet);
    // abOnline, the console's identity (SGADDR): its address, a security
    // parameter index, its machine id and its platform, a 360 (some titles
    // join only their own). Its own on each console: titles tell consoles
    // apart, themselves included, by it.
    uint8_t* online = addr_ptr->abOnline;
    xe::store_and_swap<uint32_t>(online + 0, local);
    xe::store_and_swap<uint32_t>(
        online + 4, uint32_t(MachineIdFromEnet(addr_ptr->abEnet) * 2654435761u));
    xe::store_and_swap<uint64_t>(online + 8,
                                 MachineIdFromEnet(addr_ptr->abEnet));
    online[16] = 1;
    return XnAddrStatus::XNET_GET_XNADDR_DHCP |
           XnAddrStatus::XNET_GET_XNADDR_GATEWAY |
           XnAddrStatus::XNET_GET_XNADDR_DNS;
  }

  // Just return a loopback address atm.
  addr_ptr->ina.s_addr = htonl(INADDR_LOOPBACK);
  addr_ptr->inaOnline.s_addr = 0;
  addr_ptr->wPortOnline = 0;

  // TODO(gibbed): A proper mac address.
  // RakNet's 360 version appears to depend on abEnet to create "random" 64-bit
  // numbers. A zero value will cause RakPeer::Startup to fail. This causes
  // 58411436 to crash on startup.
  // The 360-specific code is scrubbed from the RakNet repo, but there's still
  // traces of what it's doing which match the game code.
  // https://github.com/facebookarchive/RakNet/blob/master/Source/RakPeer.cpp#L382
  // https://github.com/facebookarchive/RakNet/blob/master/Source/RakPeer.cpp#L4527
  // https://github.com/facebookarchive/RakNet/blob/master/Source/RakPeer.cpp#L4467
  // "Mac address is a poor solution because you can't have multiple connections
  // from the same system"
  std::memset(addr_ptr->abEnet, 0xCC, 6);

  std::memset(addr_ptr->abOnline, 0, 20);

  return XnAddrStatus::XNET_GET_XNADDR_STATIC;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetTitleXnAddr, kNetworking, kStub);

dword_result_t NetDll_XNetGetDebugXnAddr_entry(dword_t caller,
                                               pointer_t<XNADDR> addr_ptr) {
  addr_ptr.Zero();

  // XNET_GET_XNADDR_NONE causes caller to gracefully return.
  return XnAddrStatus::XNET_GET_XNADDR_NONE;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetDebugXnAddr, kNetworking, kStub);

dword_result_t NetDll_XNetXnAddrToMachineId_entry(dword_t caller,
                                                  pointer_t<XNADDR> addr_ptr,
                                                  lpqword_t id_ptr) {
  if (!systemlink::Enabled()) {
    // Tell the caller we're not signed in to live (non-zero ret)
    return 1;
  }
  if (!addr_ptr || !id_ptr) {
    return uint32_t(X_WSAError::X_WSAEINVAL);
  }
  *id_ptr = MachineIdFromEnet(addr_ptr->abEnet);
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetXnAddrToMachineId, kNetworking, kImplemented);

dword_result_t XNetLogonGetMachineID_entry(lpqword_t id_ptr) {
  if (!systemlink::Enabled()) {
    return X_E_FAIL;
  }
  if (!id_ptr) {
    return X_E_INVALIDARG;
  }
  uint8_t mac[6];
  systemlink::MacAddress(mac);
  *id_ptr = MachineIdFromEnet(mac);
  return X_E_SUCCESS;
}
DECLARE_XAM_EXPORT1(XNetLogonGetMachineID, kNetworking, kImplemented);

dword_result_t XNetLogonGetTitleID_entry(dword_t caller, lpvoid_t params) {
  return kernel_state()->title_id();
}
DECLARE_XAM_EXPORT1(XNetLogonGetTitleID, kNetworking, kImplemented);

dword_result_t NetDll_XNetInAddrToString_entry(dword_t caller, dword_t in_addr,
                                               lpstring_t string_out,
                                               dword_t string_size) {
  const std::string text =
      fmt::format("{}.{}.{}.{}", in_addr >> 24, (in_addr >> 16) & 0xFF,
                  (in_addr >> 8) & 0xFF, in_addr & 0xFF);
  if (string_out && string_size) {
    const size_t count = std::min<size_t>(text.size(), string_size - 1);
    std::memcpy(string_out, text.data(), count);
    string_out[count] = '\0';
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetInAddrToString, kNetworking, kImplemented);

// This converts a XNet address to an IN_ADDR. The IN_ADDR is used for
// subsequent socket calls (like a handle to a XNet address)
// With system link the IN_ADDR is the console's real address, which the
// title then sends to and connects to like any other.
dword_result_t NetDll_XNetXnAddrToInAddr_entry(dword_t caller,
                                               pointer_t<XNADDR> xn_addr,
                                               pointer_t<XNKID> xid,
                                               lpdword_t in_addr) {
  if (!systemlink::Enabled()) {
    return 1;
  }
  if (!xn_addr || !in_addr) {
    return uint32_t(X_WSAError::X_WSAEINVAL);
  }
  const uint32_t address = ntohl(xn_addr->ina.s_addr);
  if (!address) {
    XELOGW("System link: title asked the IN_ADDR of an XNADDR without one");
    return uint32_t(X_WSAError::X_WSAEINVAL);
  }
  uint8_t mac[6];
  systemlink::MacAddress(mac);
  if (!std::memcmp(mac, xn_addr->abEnet, 6) &&
      address != systemlink::LocalAddress()) {
    // Two devices with one identity: each drops the other's frames as its
    // own on a real network. Here it only confuses the title's peer lists.
    XELOGW(
        "System link: the console at {}.{}.{}.{} has this one's Ethernet "
        "address; give each device its own (systemlink_mac)",
        address >> 24, (address >> 16) & 0xFF, (address >> 8) & 0xFF,
        address & 0xFF);
  }
  {
    std::lock_guard<std::mutex> lock(xnaddr_mutex);
    if (!xnaddrs.count(address)) {
      XELOGI("System link: title resolved the XNADDR of {}",
             systemlink::Describe(address));
    }
    xnaddrs[address] = *xn_addr;
  }
  // The guest's IN_ADDR is in network order, as XNADDR's.
  *in_addr = address;
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetXnAddrToInAddr, kNetworking, kImplemented);

// Does the reverse of the above: INT XNetInAddrToXnAddr(IN_ADDR ina,
// XNADDR* pxna, XNKID* pxnkid), the IN_ADDR by value.
dword_result_t NetDll_XNetInAddrToXnAddr_entry(dword_t caller, dword_t in_addr,
                                               pointer_t<XNADDR> xn_addr,
                                               pointer_t<XNKID> xid) {
  if (!systemlink::Enabled()) {
    return 1;
  }
  const uint32_t address = in_addr;
  if (xn_addr) {
    std::lock_guard<std::mutex> lock(xnaddr_mutex);
    const auto known = xnaddrs.find(address);
    if (known != xnaddrs.end()) {
      *xn_addr = known->second;
    } else {
      xn_addr.Zero();
      xn_addr->ina.s_addr = htonl(address);
      if (address == systemlink::LocalAddress()) {
        systemlink::MacAddress(xn_addr->abEnet);
      }
    }
  }
  if (xid) {
    std::lock_guard<std::mutex> lock(xnaddr_mutex);
    *xid = registered_key;
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetInAddrToXnAddr, kNetworking, kImplemented);

dword_result_t NetDll_XNetUnregisterInAddr_entry(dword_t caller,
                                                 dword_t in_addr) {
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetUnregisterInAddr, kNetworking, kImplemented);

// The security association a real console negotiates first: none here, the
// address is reachable as it is.
dword_result_t NetDll_XNetConnect_entry(dword_t caller, dword_t in_addr) {
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetConnect, kNetworking, kImplemented);

dword_result_t NetDll_XNetGetConnectStatus_entry(dword_t caller,
                                                 dword_t in_addr) {
  constexpr uint32_t XNET_CONNECT_STATUS_IDLE = 0;
  constexpr uint32_t XNET_CONNECT_STATUS_CONNECTED = 2;
  return systemlink::Enabled() ? XNET_CONNECT_STATUS_CONNECTED
                               : XNET_CONNECT_STATUS_IDLE;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetConnectStatus, kNetworking, kImplemented);

// https://www.google.com/patents/WO2008112448A1?cl=en
// Reserves a port for use by system link
dword_result_t NetDll_XNetSetSystemLinkPort_entry(dword_t caller,
                                                  dword_t port) {
  if (!systemlink::Enabled()) {
    return 1;
  }
  // Big-endian in the guest's word, as the port it is.
  systemlink_port = static_cast<uint16_t>(port);
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetSetSystemLinkPort, kNetworking, kImplemented);

dword_result_t NetDll_XNetGetSystemLinkPort_entry(dword_t caller,
                                                  lpword_t port) {
  if (port) {
    *port = systemlink_port;
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetSystemLinkPort, kNetworking, kImplemented);

// https://github.com/ILOVEPIE/Cxbx-Reloaded/blob/master/src/CxbxKrnl/EmuXOnline.h#L39
struct XEthernetStatus {
  static constexpr uint32_t XNET_ETHERNET_LINK_ACTIVE = 0x01;
  static constexpr uint32_t XNET_ETHERNET_LINK_100MBPS = 0x02;
  static constexpr uint32_t XNET_ETHERNET_LINK_10MBPS = 0x04;
  static constexpr uint32_t XNET_ETHERNET_LINK_FULL_DUPLEX = 0x08;
  static constexpr uint32_t XNET_ETHERNET_LINK_HALF_DUPLEX = 0x10;
};

dword_result_t NetDll_XNetGetEthernetLinkStatus_entry(dword_t caller) {
  // Titles offer system link only with a cable in.
  if (systemlink::Enabled() && systemlink::LocalAddress()) {
    return XEthernetStatus::XNET_ETHERNET_LINK_ACTIVE |
           XEthernetStatus::XNET_ETHERNET_LINK_100MBPS |
           XEthernetStatus::XNET_ETHERNET_LINK_FULL_DUPLEX;
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetEthernetLinkStatus, kNetworking,
                    kSketchy);

// No System Link broadcast ever reaches a title here, so none is from another
// version.
dword_result_t NetDll_XNetGetBroadcastVersionStatus_entry(dword_t caller,
                                                          dword_t reset) {
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetGetBroadcastVersionStatus, kNetworking,
                    kImplemented);

dword_result_t NetDll_XNetDnsLookup_entry(dword_t caller, lpstring_t host,
                                          dword_t event_handle,
                                          lpdword_t pdns) {
  // TODO(gibbed): actually implement this
  if (pdns) {
    auto dns_guest = kernel_memory()->SystemHeapAlloc(sizeof(XNDNS));
    auto dns = kernel_memory()->TranslateVirtual<XNDNS*>(dns_guest);
    dns->status = 1;  // non-zero = error
    *pdns = dns_guest;
  }
  if (event_handle) {
    auto ev =
        kernel_state()->object_table()->LookupObject<XEvent>(event_handle);
    assert_not_null(ev);
    ev->Set(0, false);
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetDnsLookup, kNetworking, kStub);

dword_result_t NetDll_XNetDnsRelease_entry(dword_t caller,
                                           pointer_t<XNDNS> dns) {
  if (!dns) {
    return X_STATUS_INVALID_PARAMETER;
  }
  kernel_memory()->SystemHeapFree(dns.guest_address());
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetDnsRelease, kNetworking, kStub);

dword_result_t NetDll_XNetQosServiceLookup_entry(dword_t caller, dword_t flags,
                                                 dword_t event_handle,
                                                 lpdword_t pqos) {
  // Set pqos as some games will try accessing it despite non-successful result
  if (pqos) {
    auto qos_guest = kernel_memory()->SystemHeapAlloc(sizeof(XNQOS));
    auto qos = kernel_memory()->TranslateVirtual<XNQOS*>(qos_guest);
    qos->count = qos->count_pending = 0;
    *pqos = qos_guest;
  }
  if (event_handle) {
    auto ev =
        kernel_state()->object_table()->LookupObject<XEvent>(event_handle);
    assert_not_null(ev);
    ev->Set(0, false);
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetQosServiceLookup, kNetworking, kStub);

dword_result_t NetDll_XNetQosRelease_entry(dword_t caller,
                                           pointer_t<XNQOS> qos) {
  if (!qos) {
    return X_STATUS_INVALID_PARAMETER;
  }
  // A lookup still probing finds it gone and leaves it be.
  std::lock_guard<std::mutex> lock(qos_mutex);
  qos_lookups.erase(qos.guest_address());
  kernel_memory()->SystemHeapFree(qos.guest_address());
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetQosRelease, kNetworking, kStub);

// A system link host publishes its session here, for the QoS probes the
// others send before joining it (systemlink::QosProbe).
dword_result_t NetDll_XNetQosListen_entry(dword_t caller, pointer_t<XNKID> id,
                                          lpvoid_t data, dword_t data_size,
                                          dword_t bits_per_sec, dword_t flags) {
  constexpr uint32_t XNET_QOS_LISTEN_ENABLE = 0x01;
  constexpr uint32_t XNET_QOS_LISTEN_DISABLE = 0x02;
  constexpr uint32_t XNET_QOS_LISTEN_SET_DATA = 0x04;
  constexpr uint32_t XNET_QOS_LISTEN_RELEASE = 0x10;
  if (!systemlink::Enabled()) {
    return X_ERROR_FUNCTION_FAILED;
  }
  if (!id) {
    return uint32_t(X_WSAError::X_WSAEINVAL);
  }
  XELOGI("System link: title publishes session {:02X}{:02X}.. for QoS, "
         "flags {:02X}, {} bytes",
         id->ab[0], id->ab[1], uint32_t(flags), uint32_t(data_size));
  if (flags & XNET_QOS_LISTEN_RELEASE) {
    systemlink::QosRelease(id->ab);
    return 0;
  }
  if (flags & XNET_QOS_LISTEN_SET_DATA) {
    systemlink::QosSetData(id->ab, data ? data.as<const uint8_t*>() : nullptr,
                           data ? uint32_t(data_size) : 0u);
  }
  if (flags & (XNET_QOS_LISTEN_ENABLE | XNET_QOS_LISTEN_DISABLE)) {
    systemlink::QosSetEnabled(id->ab, (flags & XNET_QOS_LISTEN_ENABLE) != 0);
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetQosListen, kNetworking, kImplemented);

// INT XNetQosLookup(UINT cxna, const XNADDR* apxna[], const XNKID* apxnkid[],
// const XNKEY* apxnkey[], UINT cina, const IN_ADDR aina[],
// const DWORD adwServiceId[], UINT cProbes, DWORD dwBitsPerSec, DWORD dwFlags,
// WSAEVENT hEvent, XNQOS** ppxnqos): the consoles by XNADDR are probed in the
// background, each answer filled in as it comes; servers by IN_ADDR (Xbox
// Live's) never answer.
dword_result_t NetDll_XNetQosLookup_entry(
    dword_t caller, dword_t num_xnaddrs, lpdword_t xnaddrs_ptr,
    lpdword_t xnkids_ptr, lpdword_t xnkeys_ptr, dword_t num_ins,
    lpdword_t ins_ptr, lpdword_t service_ids_ptr, dword_t probes,
    dword_t bits_per_sec, dword_t flags, dword_t event_handle,
    lpdword_t pqos) {
  constexpr uint8_t XNET_XNQOSINFO_COMPLETE = 0x01;
  constexpr uint8_t XNET_XNQOSINFO_TARGET_CONTACTED = 0x02;
  constexpr uint8_t XNET_XNQOSINFO_TARGET_DISABLED = 0x04;
  constexpr uint8_t XNET_XNQOSINFO_DATA_RECEIVED = 0x08;
  constexpr uint32_t kDataMax = 1024;
  // What a LAN measures; titles only compare it against their minimum.
  constexpr uint32_t kBitsPerSec = 100000000;

  if (!pqos) {
    return uint32_t(X_WSAError::X_WSAEINVAL);
  }
  const bool probing = systemlink::Enabled() && xnaddrs_ptr && xnkids_ptr;
  const uint32_t consoles = probing ? uint32_t(num_xnaddrs) : 0;
  const uint32_t count = uint32_t(num_xnaddrs) + uint32_t(num_ins);
  const uint32_t infos = std::max(count, 1u);
  const uint32_t data_at = 8 + infos * sizeof(XNQOSINFO);
  const uint32_t size = data_at + count * kDataMax;

  const uint32_t qos_guest = kernel_memory()->SystemHeapAlloc(size);
  if (!qos_guest) {
    return uint32_t(X_WSAError::X_WSAEFAULT);
  }
  kernel_memory()->Zero(qos_guest, size);
  auto qos = kernel_memory()->TranslateVirtual<XNQOS*>(qos_guest);
  qos->count = count;
  qos->count_pending = consoles;
  for (uint32_t i = 0; i < count; i++) {
    XNQOSINFO& info = qos->info[i];
    info.data_ptr = qos_guest + data_at + i * kDataMax;
    if (i >= consoles) {
      info.flags = XNET_XNQOSINFO_COMPLETE;
    }
  }

  object_ref<XEvent> ev;
  if (event_handle) {
    ev = kernel_state()->object_table()->LookupObject<XEvent>(event_handle);
  }
  {
    std::lock_guard<std::mutex> lock(qos_mutex);
    qos_lookups.insert(qos_guest);
  }
  *pqos = qos_guest;
  if (!consoles) {
    if (ev) {
      ev->Set(0, false);
    }
    return 0;
  }

  XELOGI("System link: title asked QoS of {} consoles, {} servers, {} probes",
         consoles, uint32_t(num_ins), uint32_t(probes));
  for (uint32_t i = 0; i < consoles; i++) {
    // Copied now: the title's arrays need not outlive the call.
    const uint32_t xnaddr_guest = xnaddrs_ptr[i];
    const uint32_t xnkid_guest = xnkids_ptr[i];
    if (!xnaddr_guest || !xnkid_guest) {
      std::lock_guard<std::mutex> lock(qos_mutex);
      qos->info[i].flags = XNET_XNQOSINFO_COMPLETE;
      qos->count_pending = qos->count_pending - 1;
      if (!qos->count_pending && ev) {
        ev->Set(0, false);
      }
      continue;
    }
    const uint32_t address = ntohl(
        kernel_memory()->TranslateVirtual<XNADDR*>(xnaddr_guest)->ina.s_addr);
    XNKID kid = *kernel_memory()->TranslateVirtual<XNKID*>(xnkid_guest);
    std::thread([=]() {
      const auto result = systemlink::QosProbe(address, kid.ab, probes);
      std::lock_guard<std::mutex> lock(qos_mutex);
      if (!qos_lookups.count(qos_guest)) {
        return;  // released meanwhile
      }
      auto qos = kernel_memory()->TranslateVirtual<XNQOS*>(qos_guest);
      XNQOSINFO& info = qos->info[i];
      uint8_t info_flags = XNET_XNQOSINFO_COMPLETE;
      if (result.contacted) {
        info_flags |= XNET_XNQOSINFO_TARGET_CONTACTED;
        if (result.disabled) {
          info_flags |= XNET_XNQOSINFO_TARGET_DISABLED;
        }
        if (!result.data.empty()) {
          info_flags |= XNET_XNQOSINFO_DATA_RECEIVED;
          const uint32_t len =
              std::min<uint32_t>(uint32_t(result.data.size()), kDataMax);
          std::memcpy(kernel_memory()->TranslateVirtual(info.data_ptr),
                      result.data.data(), len);
          info.data_len = static_cast<uint16_t>(len);
        }
        info.rtt_min_in_msecs = result.rtt_min_ms;
        info.rtt_med_in_msecs = result.rtt_med_ms;
        info.up_bits_per_sec = kBitsPerSec;
        info.down_bits_per_sec = kBitsPerSec;
      }
      info.probes_xmit = result.probes_sent;
      info.probes_recv = result.probes_received;
      info.flags = info_flags;
      const uint32_t pending = qos->count_pending - 1;
      qos->count_pending = pending;
      if (!pending && ev) {
        ev->Set(0, false);
      }
    }).detach();
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetQosLookup, kNetworking, kImplemented);

dword_result_t NetDll_inet_addr_entry(lpstring_t addr_ptr) {
  if (!addr_ptr) {
    return -1;
  }

  uint32_t addr = inet_addr(addr_ptr);
  // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-inet_addr#return-value
  // Based on console research it seems like x360 uses old version of inet_addr
  // In case of empty string it return 0 instead of -1
  if (addr == -1 && !addr_ptr.value().length()) {
    return 0;
  }

  return xe::byte_swap(addr);
}
DECLARE_XAM_EXPORT1(NetDll_inet_addr, kNetworking, kImplemented);

dword_result_t NetDll_socket_entry(dword_t caller, dword_t af, dword_t type,
                                   dword_t protocol) {
  XELOGI("NetDll_socket: af={}, type={}, protocol={}", af.value(), type.value(),
         protocol.value());

  if (!cvars::network_enabled) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENETDOWN));
    XELOGI("NetDll_socket: refused, network_enabled is off");
    return -1;
  }

  auto socket = object_ref<XSocket>(new XSocket(kernel_state()));
  X_STATUS result = socket->Initialize(XSocket::AddressFamily((uint32_t)af),
                                       XSocket::Type((uint32_t)type),
                                       XSocket::Protocol((uint32_t)protocol));

  if (XFAILED(result)) {
    socket->ReleaseHandle();

    XThread::SetLastError(socket->GetLastWSAError());
    XELOGE("NetDll_socket: failed with error {:08X}",
           socket->GetLastWSAError());
    return -1;
  }

  return socket->handle();
}
DECLARE_XAM_EXPORT1(NetDll_socket, kNetworking, kImplemented);

dword_result_t NetDll_closesocket_entry(dword_t caller, dword_t socket_handle) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  socket->Close();
  socket->ReleaseHandle();
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_closesocket, kNetworking, kImplemented);

int_result_t NetDll_shutdown_entry(dword_t caller, dword_t socket_handle,
                                   int_t how) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  auto ret = socket->Shutdown(how);
  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_shutdown, kNetworking, kImplemented);

dword_result_t NetDll_setsockopt_entry(dword_t caller, dword_t socket_handle,
                                       dword_t level, dword_t optname,
                                       lpvoid_t optval_ptr, dword_t optlen) {
  XELOGI("NetDll_setsockopt: socket={:08X}, level={:04X}, optname={:04X}",
         socket_handle.value(), level.value(), optname.value());

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  if (!optval_ptr) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAEFAULT));
    return -1;
  }
  X_STATUS status = socket->SetOption(level, optname, optval_ptr, optlen);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  int result = XSUCCEEDED(status) ? 0 : -1;
  XELOGI("NetDll_setsockopt: {}, returning {}",
         XSUCCEEDED(status) ? "success" : "failed", result);
  return result;
}
DECLARE_XAM_EXPORT1(NetDll_setsockopt, kNetworking, kImplemented);

dword_result_t NetDll_getsockopt_entry(dword_t caller, dword_t socket_handle,
                                       dword_t level, dword_t optname,
                                       lpvoid_t optval_ptr, lpdword_t optlen) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  if (!optval_ptr || !optlen) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAEFAULT));
    return -1;
  }
  uint32_t native_len = *optlen;
  X_STATUS status = socket->GetOption(level, optname, optval_ptr, &native_len);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    return -1;
  }
  *optlen = native_len;
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_getsockopt, kNetworking, kImplemented);

dword_result_t NetDll_ioctlsocket_entry(dword_t caller, dword_t socket_handle,
                                        dword_t cmd, lpvoid_t arg_ptr) {
  XELOGI("NetDll_ioctlsocket: socket={:08X}, cmd={:08X}", socket_handle.value(),
         cmd.value());

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  X_STATUS status = socket->IOControl(cmd, arg_ptr);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    XELOGE("NetDll_ioctlsocket: failed with error {:08X}",
           socket->GetLastWSAError());
    return -1;
  }

  // TODO
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_ioctlsocket, kNetworking, kImplemented);

dword_result_t NetDll_bind_entry(dword_t caller, dword_t socket_handle,
                                 pointer_t<XSOCKADDR_IN> name,
                                 dword_t namelen) {
  XELOGI("NetDll_bind: socket={:08X}, namelen={}", socket_handle.value(),
         namelen.value());

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  N_XSOCKADDR_IN native_name(name);
  XELOGI(
      "NetDll_bind: calling socket->Bind, family={}, port={:04X}, addr={:08X}",
      native_name.sin_family, uint16_t(native_name.sin_port),
      uint32_t(native_name.sin_addr));
  X_STATUS status = socket->Bind(&native_name, namelen);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    XELOGE("NetDll_bind: failed with error {:08X}", socket->GetLastWSAError());
    return -1;
  }

  XELOGI("NetDll_bind: success, port={}", socket->bound_port());
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_bind, kNetworking, kImplemented);

dword_result_t NetDll_connect_entry(dword_t caller, dword_t socket_handle,
                                    pointer_t<XSOCKADDR> name,
                                    dword_t namelen) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  N_XSOCKADDR native_name(name);
  X_STATUS status = socket->Connect(&native_name, namelen);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    return -1;
  }

  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_connect, kNetworking, kImplemented);

dword_result_t NetDll_listen_entry(dword_t caller, dword_t socket_handle,
                                   int_t backlog) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  X_STATUS status = socket->Listen(backlog);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    return -1;
  }

  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_listen, kNetworking, kImplemented);

dword_result_t NetDll_accept_entry(dword_t caller, dword_t socket_handle,
                                   pointer_t<XSOCKADDR> addr_ptr,
                                   lpdword_t addrlen_ptr) {
  // The address is optional but one asked for needs room.
  if (addr_ptr && (!addrlen_ptr || *addrlen_ptr < sizeof(XSOCKADDR))) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAEFAULT));
    return -1;
  }

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  N_XSOCKADDR native_addr;
  int native_len = sizeof(native_addr);
  auto new_socket =
      socket->Accept(addr_ptr ? &native_addr : nullptr, &native_len);
  if (new_socket) {
    if (addr_ptr) {
      addr_ptr->address_family = native_addr.address_family;
      std::memcpy(addr_ptr->sa_data, native_addr.sa_data,
                  sizeof(native_addr.sa_data));
      *addrlen_ptr = native_len;
    }
    return new_socket->handle();
  }
  XThread::SetLastError(socket->GetLastWSAError());
  return -1;
}
DECLARE_XAM_EXPORT1(NetDll_accept, kNetworking, kImplemented);

struct x_fd_set {
  xe::be<uint32_t> fd_count;
  xe::be<uint32_t> fd_array[64];
};

struct host_set {
  uint32_t count;
  object_ref<XSocket> sockets[64];

  // Whether select can take a native handle: still open (another guest thread
  // can close a socket while select waits, leaving -1) and, on POSIX, below
  // FD_SETSIZE - bionic's FORTIFY checks abort FD_SET and FD_ISSET otherwise.
  static bool IsSelectable(uint64_t handle) {
    if (handle == static_cast<uint64_t>(-1)) {
      return false;
    }
#ifndef XE_PLATFORM_WIN32
    if (handle >= FD_SETSIZE) {
      return false;
    }
#endif
    return true;
  }

  // POSIX select takes the highest descriptor plus one. The guest's nfds is
  // for Winsock, which ignores it, and titles pass 0 or anything.
  int NativeNfds() const {
    int nfds = 0;
    for (uint32_t i = 0; i < this->count; ++i) {
      const uint64_t handle = this->sockets[i]->native_handle();
      if (IsSelectable(handle)) {
        nfds = std::max(nfds, int(handle) + 1);
      }
    }
    return nfds;
  }

  // False when a handle names no socket, which Winsock fails with
  // WSAENOTSOCK. The guest count is clamped to the 64 the set holds.
  bool Load(const x_fd_set* guest_set) {
    assert_true(guest_set->fd_count <= 64);
    this->count = std::min(uint32_t(guest_set->fd_count), uint32_t(64));
    for (uint32_t i = 0; i < this->count; ++i) {
      auto socket_handle = static_cast<X_HANDLE>(guest_set->fd_array[i]);
      if (socket_handle == -1) {
        this->count = i;
        break;
      }
      // Convert from Xenia -> native
      auto socket =
          kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
      if (!socket) {
        this->count = i;
        return false;
      }
      this->sockets[i] = socket;
    }
    return true;
  }

  void Store(x_fd_set* guest_set) {
    guest_set->fd_count = 0;
    for (uint32_t i = 0; i < this->count; ++i) {
      auto socket = this->sockets[i];
      guest_set->fd_array[guest_set->fd_count++] = socket->handle();
    }
  }

  void Store(fd_set* native_set) {
    FD_ZERO(native_set);
    for (uint32_t i = 0; i < this->count; ++i) {
      const auto handle = this->sockets[i]->native_handle();
      if (IsSelectable(handle)) {
        FD_SET(handle, native_set);
      }
    }
  }

  void UpdateFrom(fd_set* native_set) {
    uint32_t new_count = 0;
    for (uint32_t i = 0; i < this->count; ++i) {
      auto socket = this->sockets[i];
      const auto handle = socket->native_handle();
      if (IsSelectable(handle) && FD_ISSET(handle, native_set)) {
        this->sockets[new_count++] = socket;
      }
    }
    this->count = new_count;
  }
};

int_result_t NetDll_select_entry(dword_t caller, dword_t nfds,
                                 pointer_t<x_fd_set> readfds,
                                 pointer_t<x_fd_set> writefds,
                                 pointer_t<x_fd_set> exceptfds,
                                 lpvoid_t timeout_ptr) {
  XELOGD("NetDll_select: nfds={}, timeout={:08X}", nfds.value(),
         timeout_ptr.guest_address());

  host_set host_readfds = {0};
  fd_set native_readfds = {0};
  if (readfds) {
    if (!host_readfds.Load(readfds)) {
      XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
      return -1;
    }
    host_readfds.Store(&native_readfds);
  }
  host_set host_writefds = {0};
  fd_set native_writefds = {0};
  if (writefds) {
    if (!host_writefds.Load(writefds)) {
      XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
      return -1;
    }
    host_writefds.Store(&native_writefds);
  }
  host_set host_exceptfds = {0};
  fd_set native_exceptfds = {0};
  if (exceptfds) {
    if (!host_exceptfds.Load(exceptfds)) {
      XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
      return -1;
    }
    host_exceptfds.Store(&native_exceptfds);
  }
  timeval* timeout_in = nullptr;
  timeval timeout;
  if (timeout_ptr) {
    timeout = {static_cast<int32_t>(timeout_ptr.as_array<int32_t>()[0]),
               static_cast<int32_t>(timeout_ptr.as_array<int32_t>()[1])};
    Clock::ScaleGuestDurationTimeval(
        reinterpret_cast<int32_t*>(&timeout.tv_sec),
        reinterpret_cast<int32_t*>(&timeout.tv_usec));
    timeout_in = &timeout;
  }
#ifdef XE_PLATFORM_WIN32
  const int native_nfds = int(nfds);
#else
  const int native_nfds = std::max(
      {host_readfds.NativeNfds(), host_writefds.NativeNfds(),
       host_exceptfds.NativeNfds()});
#endif
  int ret;
  if (!GuestScheduler::CurrentThreadOffloadsBlockingCalls()) {
    ret = select(native_nfds, readfds ? &native_readfds : nullptr,
                 writefds ? &native_writefds : nullptr,
                 exceptfds ? &native_exceptfds : nullptr, timeout_in);
  } else {
    // A blocking select would stall the other guest threads on this host
    // thread. Poll instead and park between polls until the timeout.
    auto* scheduler = kernel_state()->guest_scheduler();
    uint64_t deadline_ms = 0;
    if (timeout_in) {
      // Rounded up so it never waits less than asked.
      deadline_ms = Clock::QueryHostUptimeMillis() +
                    uint64_t(timeout.tv_sec) * 1000 +
                    (timeout.tv_usec + 999) / 1000;
    }
    while (true) {
      timeval poll_timeout = {0, 0};
      ret = select(native_nfds, readfds ? &native_readfds : nullptr,
                   writefds ? &native_writefds : nullptr,
                   exceptfds ? &native_exceptfds : nullptr, &poll_timeout);
      if (ret != 0 ||
          (timeout_in && Clock::QueryHostUptimeMillis() >= deadline_ms)) {
        break;
      }
      // A select with nothing ready empties the sets. Refill them first.
      if (readfds) {
        host_readfds.Store(&native_readfds);
      }
      if (writefds) {
        host_writefds.Store(&native_writefds);
      }
      if (exceptfds) {
        host_exceptfds.Store(&native_exceptfds);
      }
      scheduler->BlockCurrentThread(deadline_ms);
    }
  }
  if (ret == -1) {
    // The sets are undefined after a failure: none of the sockets is ready.
#ifdef XE_PLATFORM_WIN32
    const bool bad_socket = WSAGetLastError() == WSAENOTSOCK;
#else
    const bool bad_socket = errno == EBADF;
#endif
    XThread::SetLastError(uint32_t(bad_socket ? X_WSAError::X_WSAENOTSOCK
                                              : X_WSAError::X_WSAEINVAL));
    host_readfds.count = 0;
    host_writefds.count = 0;
    host_exceptfds.count = 0;
  }
  if (readfds) {
    host_readfds.UpdateFrom(&native_readfds);
    host_readfds.Store(readfds);
  }
  if (writefds) {
    host_writefds.UpdateFrom(&native_writefds);
    host_writefds.Store(writefds);
  }
  if (exceptfds) {
    host_exceptfds.UpdateFrom(&native_exceptfds);
    host_exceptfds.Store(exceptfds);
  }

  // TODO(gibbed): modify ret to be what's actually copied to the guest fd_sets?
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_select, kNetworking, kImplemented);

dword_result_t NetDll_recv_entry(dword_t caller, dword_t socket_handle,
                                 lpvoid_t buf_ptr, dword_t buf_len,
                                 dword_t flags) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  const int ret = socket->Recv(buf_ptr, buf_len, flags);
  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_recv, kNetworking, kImplemented);

dword_result_t NetDll_recvfrom_entry(dword_t caller, dword_t socket_handle,
                                     lpvoid_t buf_ptr, dword_t buf_len,
                                     dword_t flags,
                                     pointer_t<XSOCKADDR_IN> from_ptr,
                                     lpdword_t fromlen_ptr) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  N_XSOCKADDR_IN native_from;
  if (from_ptr) {
    native_from = *from_ptr;
  }
  uint32_t native_fromlen = fromlen_ptr ? fromlen_ptr.value() : 0;
  int ret = socket->RecvFrom(buf_ptr, buf_len, flags, &native_from,
                             fromlen_ptr ? &native_fromlen : 0);

  if (from_ptr) {
    from_ptr->sin_family = native_from.sin_family;
    from_ptr->sin_port = native_from.sin_port;
    from_ptr->sin_addr = native_from.sin_addr;
    std::memset(from_ptr->x_sin_zero, 0, sizeof(from_ptr->x_sin_zero));
  }
  if (fromlen_ptr) {
    *fromlen_ptr = native_fromlen;
  }

  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
  } else if (ret > 0) {
    // Only log when we actually receive data
    XELOGD("NetDll_recvfrom: socket={:08X}, received {} bytes",
           socket_handle.value(), ret);
  }

  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_recvfrom, kNetworking, kImplemented);

dword_result_t NetDll_send_entry(dword_t caller, dword_t socket_handle,
                                 lpvoid_t buf_ptr, dword_t buf_len,
                                 dword_t flags) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  const int ret = socket->Send(buf_ptr, buf_len, flags);
  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_send, kNetworking, kImplemented);

dword_result_t NetDll_sendto_entry(dword_t caller, dword_t socket_handle,
                                   lpvoid_t buf_ptr, dword_t buf_len,
                                   dword_t flags,
                                   pointer_t<XSOCKADDR_IN> to_ptr,
                                   dword_t to_len) {
  XELOGD("NetDll_sendto: socket={:08X}, len={}", socket_handle.value(),
         buf_len.value());

  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  // A NULL destination sends to the connected peer.
  N_XSOCKADDR_IN native_to;
  N_XSOCKADDR_IN* to = nullptr;
  if (to_ptr) {
    native_to = *to_ptr;
    to = &native_to;
  }
  int ret = socket->SendTo(buf_ptr, buf_len, flags, to, to_len);
  if (ret == -1) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_sendto, kNetworking, kImplemented);

dword_result_t NetDll_WSAEventSelect_entry(dword_t caller,
                                           dword_t socket_handle,
                                           dword_t event_handle,
                                           dword_t network_events) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }

  object_ref<XEvent> ev;
  if (event_handle) {
    ev = kernel_state()->object_table()->LookupObject<XEvent>(event_handle);
    if (!ev) {
      XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
      return -1;
    }
  }

  int ret = socket->WSAEventSelect(std::move(ev), network_events);
  if (ret < 0) {
    XThread::SetLastError(socket->GetLastWSAError());
  }
  return ret;
}
DECLARE_XAM_EXPORT1(NetDll_WSAEventSelect, kNetworking, kImplemented);

dword_result_t NetDll___WSAFDIsSet_entry(dword_t socket_handle,
                                         pointer_t<x_fd_set> fd_set) {
  const uint8_t max_fd_count =
      std::min((uint32_t)fd_set->fd_count, uint32_t(64));
  for (uint8_t i = 0; i < max_fd_count; i++) {
    if (fd_set->fd_array[i] == socket_handle) {
      return 1;
    }
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll___WSAFDIsSet, kNetworking, kImplemented);

void NetDll_WSASetLastError_entry(dword_t error_code) {
  XThread::SetLastError(error_code);
}
DECLARE_XAM_EXPORT1(NetDll_WSASetLastError, kNetworking, kImplemented);

// getsockname and getpeername, which write the guest's sockaddr_in.
static int GetSocketName(uint32_t socket_handle, bool peer,
                         pointer_t<XSOCKADDR_IN> name_ptr, lpdword_t len_ptr) {
  auto socket =
      kernel_state()->object_table()->LookupObject<XSocket>(socket_handle);
  if (!socket) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAENOTSOCK));
    return -1;
  }
  if (!name_ptr || !len_ptr || *len_ptr < sizeof(XSOCKADDR_IN)) {
    XThread::SetLastError(uint32_t(X_WSAError::X_WSAEFAULT));
    return -1;
  }

  N_XSOCKADDR_IN name;
  X_STATUS status =
      peer ? socket->GetPeerName(&name) : socket->GetSockName(&name);
  if (XFAILED(status)) {
    XThread::SetLastError(socket->GetLastWSAError());
    return -1;
  }

  name_ptr->sin_family = name.sin_family;
  name_ptr->sin_port = name.sin_port;
  name_ptr->sin_addr = name.sin_addr;
  std::memset(name_ptr->x_sin_zero, 0, sizeof(name_ptr->x_sin_zero));
  *len_ptr = sizeof(XSOCKADDR_IN);
  return 0;
}

dword_result_t NetDll_getsockname_entry(dword_t caller, dword_t socket_handle,
                                        pointer_t<XSOCKADDR_IN> name_ptr,
                                        lpdword_t len_ptr) {
  return GetSocketName(socket_handle, false, name_ptr, len_ptr);
}
DECLARE_XAM_EXPORT1(NetDll_getsockname, kNetworking, kImplemented);

dword_result_t NetDll_getpeername_entry(dword_t caller, dword_t socket_handle,
                                        pointer_t<XSOCKADDR_IN> name_ptr,
                                        lpdword_t len_ptr) {
  return GetSocketName(socket_handle, true, name_ptr, len_ptr);
}
DECLARE_XAM_EXPORT1(NetDll_getpeername, kNetworking, kImplemented);

dword_result_t NetDll_XNetCreateKey_entry(dword_t caller,
                                          pointer_t<XNKID> key_id,
                                          pointer_t<XNKEY> exchange_key) {
  if (!systemlink::Enabled()) {
    kernel_memory()->Fill(key_id.guest_address(), 8, 0xBE);
    kernel_memory()->Fill(exchange_key.guest_address(), 16, 0xBE);
    return 0;
  }
  // A random one, of the system link kind: a host's session is told apart
  // from the next by it.
  XNKID kid;
  XNKEY key;
  RandomBytes(kid.ab, sizeof(kid.ab));
  RandomBytes(key.ab, sizeof(key.ab));
  kid.ab[0] &= 0x1F;
  *key_id = kid;
  *exchange_key = key;
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetCreateKey, kNetworking, kImplemented);

dword_result_t NetDll_XNetRegisterKey_entry(dword_t caller,
                                            pointer_t<XNKID> key_id,
                                            pointer_t<XNKEY> exchange_key) {
  if (key_id) {
    std::lock_guard<std::mutex> lock(xnaddr_mutex);
    registered_key = *key_id;
    key_registered = true;
    if (systemlink::Enabled()) {
      XELOGI("System link: title registered session key {:02X}{:02X}..",
             key_id->ab[0], key_id->ab[1]);
    }
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetRegisterKey, kNetworking, kImplemented);

dword_result_t NetDll_XNetUnregisterKey_entry(dword_t caller,
                                              pointer_t<XNKID> key_id) {
  std::lock_guard<std::mutex> lock(xnaddr_mutex);
  if (key_id && key_registered &&
      !std::memcmp(key_id->ab, registered_key.ab, sizeof(registered_key.ab))) {
    registered_key = {};
    key_registered = false;
  }
  return 0;
}
DECLARE_XAM_EXPORT1(NetDll_XNetUnregisterKey, kNetworking, kStub);

}  // namespace xam
}  // namespace kernel
}  // namespace xe

DECLARE_XAM_EMPTY_REGISTER_EXPORTS(Net);
