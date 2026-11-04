/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_SYSTEMLINK_H_
#define XENIA_KERNEL_SYSTEMLINK_H_

#include <cstdint>
#include <string>
#include <vector>

namespace xe {
namespace kernel {
namespace systemlink {

// System link: consoles on one local network finding each other through
// broadcasts, as 360s on a LAN do. Titles already talk through host sockets
// (XSocket), so what this adds is what the network around them needs:
//
// - an address: the XNADDR a title hands its peers carries this device's IPv4
//   address and an Ethernet address of its own (XNetGetTitleXnAddr);
// - broadcasts: one to 255.255.255.255 goes to the local network's broadcast
//   address (some Android networks drop the all-ones one) and, as unicasts, to
//   every console named in systemlink_peers and to the consoles heard from
//   outside the local network: the far end of a VPN such as ZeroTier or
//   Tailscale, which carries no broadcasts;
// - ports: a socket bound below 1024 takes port + 10000 on the host (the app
//   has no privilege for the low ones), so traffic between consoles is
//   translated both ways and titles still see the ports they asked for;
// - its own broadcasts, which come back to the socket that sent them, are
//   dropped, as a console never hears itself.
//
// Addresses are IPv4 in host byte order.

// Whether system link is on (systemlink cvar).
bool Enabled();

// This console's address on the network ("systemlink_address", else the first
// Wi-Fi or Ethernet interface's); 0 when there is none.
uint32_t LocalAddress();
// The local network's broadcast address; 255.255.255.255 when not known.
uint32_t BroadcastAddress();
// Whether a title's destination is a broadcast (the all-ones address or the
// local network's).
bool IsBroadcast(uint32_t address);
// Whether address is this console's own on the network.
bool IsLocalAddress(uint32_t address);

// This console's Ethernet address, for its XNADDR.
void MacAddress(uint8_t mac[6]);

// The consoles a broadcast also goes to, as unicasts: the named peers and
// those heard from outside the local network lately.
std::vector<uint32_t> BroadcastPeers();
// A datagram from address arrived.
void Heard(uint32_t address);

// Whether address is a console's (this one included, loopback too): its
// ports are translated. Servers on the Internet are not.
bool IsConsole(uint32_t address);

// A port the title asked for, as the host has it, and back.
uint16_t HostPort(uint16_t guest_port);
uint16_t GuestPort(uint16_t host_port);

// An address for the log, without the address itself (shared logs hide
// addresses): what it is to this console, and its last byte.
std::string Describe(uint32_t address);
// A datagram a title sent to or received from another console, port as the
// host has it: logged with its first bytes, the first ones for each address,
// port and socket.
void Trace(bool sent, uint32_t address, uint16_t port, uint16_t local_port,
           const uint8_t* data, size_t size);

// QoS: before joining a session a title probes its host (XNetQosLookup), and
// the host answers with what it published (XNetQosListen): whether it takes
// players and a few bytes of its own. Probes go over UDP to kQosPort, where
// every console with system link on answers for the sessions it publishes.
// Sessions are told apart by their XNKID.
constexpr uint16_t kQosPort = 13074;

// What this console answers for session kid: whether it takes players, its
// data, or nothing (released).
void QosSetEnabled(const uint8_t kid[8], bool enabled);
void QosSetData(const uint8_t kid[8], const uint8_t* data, size_t size);
void QosRelease(const uint8_t kid[8]);

struct QosResult {
  bool contacted = false;
  bool disabled = false;
  std::vector<uint8_t> data;
  uint16_t probes_sent = 0;
  uint16_t probes_received = 0;
  uint16_t rtt_min_ms = 0;
  uint16_t rtt_med_ms = 0;
};
// Probes the console at address about session kid; blocks for up to about
// half a second a probe, at most 8 probes.
QosResult QosProbe(uint32_t address, const uint8_t kid[8], unsigned probes);

}  // namespace systemlink
}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_SYSTEMLINK_H_
