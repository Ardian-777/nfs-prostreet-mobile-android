/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/xsocket.h"

#include <atomic>
#include <cstring>
#include <future>
#include <optional>
#include <string_view>
#include <thread>

#include "xenia/base/clock.h"
#include "xenia/base/logging.h"
#include "xenia/base/memory.h"
#include "xenia/base/platform.h"
#include "xenia/base/threading.h"
#include "xenia/kernel/guest_scheduler.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/systemlink.h"
#include "xenia/kernel/xam/xam_module.h"
#include "xenia/kernel/xevent.h"
#include "xenia/kernel/xsocket_options.h"
#include "xenia/kernel/xthread.h"

namespace xe {
namespace kernel {

namespace {

// Drives async_wait completions on a worker thread for the process lifetime.
class IoContextRunner {
 public:
  IoContextRunner() : work_(asio::make_work_guard(io_context_)) {
    thread_ = std::thread([this]() {
      xe::threading::set_name("Xenia Socket I/O");
      io_context_.run();
    });
  }
  ~IoContextRunner() {
    work_.reset();
    io_context_.stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }
  asio::io_context& get() { return io_context_; }

 private:
  asio::io_context io_context_;
  asio::executor_work_guard<asio::io_context::executor_type> work_;
  std::thread thread_;
};

}  // namespace

static std::atomic<bool> io_context_started = false;

// Shared io_context for all sockets
static asio::io_context& GetIoContext() {
  static IoContextRunner runner;
  io_context_started = true;
  return runner.get();
}

void DrainSocketIo() {
  if (!io_context_started) {
    return;
  }
  // The I/O thread runs handlers in the order they were queued, but a wait
  // the reactor completed along with another may be queued after this is, so
  // this goes around the queue twice.
  std::promise<void> drained;
  asio::io_context& io_context = GetIoContext();
  asio::post(io_context, [&io_context, &drained]() {
    asio::post(io_context, [&drained]() { drained.set_value(); });
  });
  drained.get_future().wait();
}

// Translate socket options to native
// Note:
// SO_DONTLINGER = ~SO_LINGER
// SO_EXCLUSIVEADDRUSE = ~SO_REUSEADDR
// TODO: Check SO_DONTLINGER and SO_EXCLUSIVEADDRUSE usage on linux
const std::map<uint32_t, int> supported_socket_options = {
    {0x0004, SO_REUSEADDR}, {0x0020, SO_BROADCAST}, {0x0080, SO_LINGER},
    {0x1001, SO_SNDBUF},    {0x1002, SO_RCVBUF},    {0x1005, SO_SNDTIMEO},
    {0x1006, SO_RCVTIMEO},  {~0x0080u, ~SO_LINGER}, {~0x0004u, ~SO_REUSEADDR}};

// Translate socket TCP options to native
const std::map<uint32_t, int> supported_tcp_options = {{0x0001, TCP_NODELAY}};

// Whether an option above is a BOOL or an int, which the guest gives
// big-endian and the host takes as its own int.
static bool IsIntSocketOption(uint32_t level, uint32_t optname) {
  if (level == 0xFFFF) {
    return optname == 0x0004 || optname == 0x0020 || optname == 0x1001 ||
           optname == 0x1002;
  }
  return level == 0x6 && optname == 0x0001;
}

// The fewest bytes a value of such an option, or of SO_SNDTIMEO and
// SO_RCVTIMEO, takes: a BOOL may be one byte, an int or a DWORD is four, and
// a shorter value fails with WSAEFAULT.
static uint32_t SocketOptionMinLength(uint32_t level, uint32_t optname) {
  return level == 0xFFFF && (optname == 0x1001 || optname == 0x1002 ||
                             optname == 0x1005 || optname == 0x1006)
             ? 4
             : 1;
}

// Translate ioctl commands to native
const std::map<uint32_t, uint32_t> supported_controls = {
    {0x8004667E, FIONBIO}, {0x4004667F, FIONREAD}};

// Translate socket levels to native
const std::map<uint32_t, int> supported_levels = {{0xFFFF, SOL_SOCKET},
                                                  {0x6, IPPROTO_TCP}};

// asio error_code -> Winsock WSAE* code. Guests look up by Winsock value;
// returning raw POSIX errno makes recoverable errors look fatal (e.g. COD4
// MP treats unrecognized recvfrom error as a hard init failure).
uint32_t AsioErrorToWSAError(const asio::error_code& ec) {
  if (!ec) {
    return 0;
  }
  if (ec == asio::error::would_block || ec == asio::error::try_again) {
    return 10035;  // WSAEWOULDBLOCK
  }
  if (ec == asio::error::in_progress) {
    return 10036;  // WSAEINPROGRESS
  }
  if (ec == asio::error::already_started) {
    return 10037;  // WSAEALREADY
  }
  if (ec == asio::error::not_socket) {
    return 10038;  // WSAENOTSOCK
  }
  if (ec == asio::error::message_size) {
    return 10040;  // WSAEMSGSIZE
  }
  if (ec == asio::error::no_protocol_option) {
    return 10042;  // WSAENOPROTOOPT
  }
  if (ec == asio::error::address_family_not_supported) {
    return 10047;  // WSAEAFNOSUPPORT
  }
  if (ec == asio::error::address_in_use) {
    return 10048;  // WSAEADDRINUSE
  }
  if (ec == asio::error::network_down) {
    return 10050;  // WSAENETDOWN
  }
  if (ec == asio::error::network_unreachable) {
    return 10051;  // WSAENETUNREACH
  }
  if (ec == asio::error::network_reset) {
    return 10052;  // WSAENETRESET
  }
  if (ec == asio::error::connection_aborted) {
    return 10053;  // WSAECONNABORTED
  }
  if (ec == asio::error::connection_reset) {
    return 10054;  // WSAECONNRESET
  }
  if (ec == asio::error::no_buffer_space) {
    return 10055;  // WSAENOBUFS
  }
  if (ec == asio::error::already_connected) {
    return 10056;  // WSAEISCONN
  }
  if (ec == asio::error::not_connected) {
    return 10057;  // WSAENOTCONN
  }
  if (ec == asio::error::shut_down) {
    return 10058;  // WSAESHUTDOWN
  }
  if (ec == asio::error::timed_out) {
    return 10060;  // WSAETIMEDOUT
  }
  if (ec == asio::error::connection_refused) {
    return 10061;  // WSAECONNREFUSED
  }
  if (ec == asio::error::host_unreachable) {
    return 10065;  // WSAEHOSTUNREACH
  }
  if (ec == asio::error::access_denied) {
    return 10013;  // WSAEACCES
  }
  if (ec == asio::error::fault) {
    return 10014;  // WSAEFAULT
  }
  if (ec == asio::error::invalid_argument) {
    return 10022;  // WSAEINVAL
  }
  if (ec == asio::error::operation_aborted) {
    return 995;  // WSA_OPERATION_ABORTED
  }
  if (ec == asio::error::interrupted) {
    return 10004;  // WSAEINTR
  }
  return static_cast<uint32_t>(ec.value());
}

// The Winsock code of the last call that failed on a host socket.
static uint32_t HostSocketError() {
#if XE_PLATFORM_WIN32
  const int host_error = WSAGetLastError();
#else
  const int host_error = errno;
#endif  // XE_PLATFORM_WIN32
  return AsioErrorToWSAError(
      asio::error_code(host_error, asio::error::get_system_category()));
}

XSocket::XSocket(KernelState* kernel_state)
    : XObject(kernel_state, kObjectType) {}

XSocket::XSocket(KernelState* kernel_state, asio::ip::tcp::socket socket)
    : XObject(kernel_state, kObjectType), tcp_socket_(std::move(socket)) {
  af_ = AddressFamily::X_AF_INET;
  type_ = Type::X_SOCK_STREAM;
  proto_ = Protocol::X_IPPROTO_TCP;
}

XSocket::~XSocket() { Close(); }

uint64_t XSocket::native_handle() {
  if (tcp_socket_ && tcp_socket_->is_open()) {
    return static_cast<uint64_t>(tcp_socket_->native_handle());
  }
  if (udp_socket_ && udp_socket_->is_open()) {
    return static_cast<uint64_t>(udp_socket_->native_handle());
  }
  if (acceptor_ && acceptor_->is_open()) {
    return static_cast<uint64_t>(acceptor_->native_handle());
  }
  return static_cast<uint64_t>(-1);
}

X_STATUS XSocket::Initialize(AddressFamily af, Type type, Protocol proto) {
  af_ = af;
  type_ = type;
  proto_ = proto;

  if (proto == Protocol::X_IPPROTO_VDP) {
    // VDP is a layer on top of UDP.
    proto = Protocol::X_IPPROTO_UDP;
  }

  asio::error_code ec;

  if (type == Type::X_SOCK_STREAM) {
    // Use an acceptor for TCP — it supports bind, listen, and accept natively.
    // If Connect() is called later, we transition to a tcp_socket at that
    // point.
    acceptor_.emplace(GetIoContext());
    acceptor_->open(asio::ip::tcp::v4(), ec);
  } else if (type == Type::X_SOCK_DGRAM) {
    udp_socket_.emplace(GetIoContext());
    udp_socket_->open(asio::ip::udp::v4(), ec);
  } else {
    return X_STATUS_INVALID_PARAMETER;
  }

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return X_STATUS_UNSUCCESSFUL;
  }

  // Lets a relaunched title listen on a port its closed connections still
  // hold in TIME_WAIT.
  if (type == Type::X_SOCK_STREAM) {
    int reuse = 1;
    setsockopt(static_cast<int>(native_handle()), SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));
  }

  // System link titles broadcast without always asking for it.
  if (type == Type::X_SOCK_DGRAM && systemlink::Enabled()) {
    udp_socket_->set_option(asio::socket_base::broadcast(true), ec);
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Close() {
  asio::error_code ec;

  if (tcp_socket_) {
    if (tcp_socket_->is_open()) {
      tcp_socket_->shutdown(asio::socket_base::shutdown_both, ec);
      tcp_socket_->close(ec);
    }
    tcp_socket_.reset();
  }

  if (udp_socket_) {
    if (udp_socket_->is_open()) {
      udp_socket_->close(ec);
    }
    udp_socket_.reset();
  }

  if (acceptor_) {
    if (acceptor_->is_open()) {
      acceptor_->close(ec);
    }
    acceptor_.reset();
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::GetOption(uint32_t level, uint32_t optname, void* optval_ptr,
                            uint32_t* optlen) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    last_error_ = AsioErrorToWSAError(asio::error::not_socket);
    return X_STATUS_INVALID_HANDLE;
  }

  if ((IsIntSocketOption(level, optname) ||
       (level == 0xFFFF && (optname == 0x1005 || optname == 0x1006))) &&
      *optlen < SocketOptionMinLength(level, optname)) {
    last_error_ = uint32_t(X_WSAError::X_WSAEFAULT);
    return X_STATUS_UNSUCCESSFUL;
  }

  // SetOption keeps SO_SNDTIMEO and SO_RCVTIMEO off the host socket.
  if (level == 0xFFFF && (optname == 0x1005 || optname == 0x1006)) {
    StoreSocketOptionValue(
        optval_ptr, *optlen,
        optname == 0x1006 ? recv_timeout_ms_ : send_timeout_ms_);
    return X_STATUS_SUCCESS;
  }

  // Map Xbox socket levels to native
  int native_level = level;
  if (supported_levels.contains(level)) {
    native_level = supported_levels.at(level);
  }

  // Map Xbox socket options to native
  int native_optname = optname;
  if (level == 0xFFFF && supported_socket_options.contains(optname)) {
    native_optname = supported_socket_options.at(optname);
  } else if (level == IPPROTO_TCP && supported_tcp_options.contains(optname)) {
    native_optname = supported_tcp_options.at(optname);
  }

  asio::error_code ec;
  socklen_t native_optlen = static_cast<socklen_t>(*optlen);

  int native_handle_val = static_cast<int>(native_handle());
  if (native_handle_val == -1) {
    last_error_ = AsioErrorToWSAError(asio::error::not_socket);
    return X_STATUS_INVALID_HANDLE;
  }

  if (IsIntSocketOption(level, optname)) {
    int int_value = 0;
    socklen_t int_length = sizeof(int_value);
    if (getsockopt(native_handle_val, native_level, native_optname, &int_value,
                   &int_length) < 0) {
      last_error_ = HostSocketError();
      return X_STATUS_UNSUCCESSFUL;
    }
    StoreSocketOptionValue(optval_ptr, *optlen, uint32_t(int_value));
    return X_STATUS_SUCCESS;
  }

  int ret = getsockopt(native_handle_val, native_level, native_optname,
                       static_cast<char*>(optval_ptr), &native_optlen);
  if (ret < 0) {
    last_error_ = HostSocketError();
    return X_STATUS_UNSUCCESSFUL;
  }

  *optlen = static_cast<uint32_t>(native_optlen);
  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::SetOption(uint32_t level, uint32_t optname, void* optval_ptr,
                            uint32_t optlen) {
  if (level == 0xFFFF && (optname == 0x5801 || optname == 0x5802)) {
    // Disable socket encryption
    secure_ = false;
    return X_STATUS_SUCCESS;
  }

  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    last_error_ = AsioErrorToWSAError(asio::error::not_socket);
    return X_STATUS_INVALID_HANDLE;
  }

  // SO_EXCLUSIVEADDRUSE. Bind already refuses a port another socket holds.
  // Windows refuses the option next to the SO_REUSEADDR of stream sockets and
  // other hosts have no such option.
  if (level == 0xFFFF && optname == ~0x0004u) {
    return X_STATUS_SUCCESS;
  }

  if ((IsIntSocketOption(level, optname) ||
       (level == 0xFFFF && (optname == 0x1005 || optname == 0x1006))) &&
      optlen < SocketOptionMinLength(level, optname)) {
    last_error_ = uint32_t(X_WSAError::X_WSAEFAULT);
    return X_STATUS_UNSUCCESSFUL;
  }

  // SO_SNDTIMEO / SO_RCVTIMEO in milliseconds. Kept for the cooperative retry
  // loop, which never sees the host option take effect, and kept off the host:
  // a POSIX host wants a timeval and refuses the guest's DWORD.
  if (level == 0xFFFF && (optname == 0x1005 || optname == 0x1006)) {
    uint32_t timeout_ms = SocketOptionValue(optval_ptr, optlen);
    if (optname == 0x1006) {
      recv_timeout_ms_ = timeout_ms;
    } else {
      send_timeout_ms_ = timeout_ms;
    }
    return X_STATUS_SUCCESS;
  }

  // Map Xbox socket levels to native
  int native_level = level;
  if (supported_levels.contains(level)) {
    native_level = supported_levels.at(level);
  }

  // Map Xbox socket options to native
  int native_optname = optname;
  if (level == 0xFFFF && supported_socket_options.contains(optname)) {
    native_optname = supported_socket_options.at(optname);
  } else if (level == IPPROTO_TCP && supported_tcp_options.contains(optname)) {
    native_optname = supported_tcp_options.at(optname);
  }

  int native_handle_val = static_cast<int>(native_handle());
  if (native_handle_val == -1) {
    last_error_ = AsioErrorToWSAError(asio::error::not_socket);
    return X_STATUS_INVALID_HANDLE;
  }

  // A host other than Windows refuses a one-byte BOOL.
  int int_value = 0;
  const void* native_optval = optval_ptr;
  socklen_t native_optlen = optlen;
  if (IsIntSocketOption(level, optname)) {
    int_value = int(SocketOptionValue(optval_ptr, optlen));
    native_optval = &int_value;
    native_optlen = sizeof(int_value);
  }

  int ret = setsockopt(native_handle_val, native_level, native_optname,
                       static_cast<const char*>(native_optval), native_optlen);
  if (ret < 0) {
    last_error_ = HostSocketError();
    XELOGE("XSocket::SetOption: failed with error {:08X}", last_error_);
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::IOControl(uint32_t cmd, uint8_t* arg_ptr) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    return X_STATUS_INVALID_HANDLE;
  }

  asio::error_code ec;

  // FIONBIO - set non-blocking mode
  if (cmd == 0x8004667E) {
    uint32_t value = *reinterpret_cast<uint32_t*>(arg_ptr);
    bool non_blocking = (value != 0);

    if (acceptor_) {
      acceptor_->non_blocking(non_blocking, ec);
    } else if (tcp_socket_) {
      tcp_socket_->non_blocking(non_blocking, ec);
    } else if (udp_socket_) {
      udp_socket_->non_blocking(non_blocking, ec);
    }

    if (ec) {
      last_error_ = AsioErrorToWSAError(ec);
      return X_STATUS_UNSUCCESSFUL;
    }
    guest_non_blocking_ = non_blocking;
    return X_STATUS_SUCCESS;
  }

  // FIONREAD - get bytes available
  if (cmd == 0x4004667F) {
    size_t available = 0;
    if (tcp_socket_) {
      available = tcp_socket_->available(ec);
    } else if (udp_socket_) {
      available = udp_socket_->available(ec);
    }

    if (ec) {
      last_error_ = AsioErrorToWSAError(ec);
      return X_STATUS_UNSUCCESSFUL;
    }

    xe::store_and_swap<uint32_t>(arg_ptr, static_cast<uint32_t>(available));
    return X_STATUS_SUCCESS;
  }

  XELOGE("XSocket::IOControl: unsupported command {:08X}", cmd);
  return X_STATUS_INVALID_PARAMETER;
}

void XSocket::SetHostNonBlocking(bool enable) {
  asio::error_code ec;
  if (acceptor_) {
    acceptor_->non_blocking(enable, ec);
  } else if (tcp_socket_) {
    tcp_socket_->non_blocking(enable, ec);
  } else if (udp_socket_) {
    udp_socket_->non_blocking(enable, ec);
  }
}

void XSocket::RunCooperatively(asio::error_code& ec, RetryMode mode,
                               const std::function<void()>& attempt) {
  XThread* self =
      GuestScheduler::enabled() ? XThread::GetCurrentFiberThread() : nullptr;
  if (!self || guest_non_blocking_) {
    attempt();
    return;
  }

  // Enforced here because the host option only applies in blocking mode. No
  // socket timeout bounds a connect or accept.
  uint32_t timeout_ms = 0;
  if (mode == RetryMode::kReceive) {
    timeout_ms = recv_timeout_ms_;
  } else if (mode == RetryMode::kSend) {
    timeout_ms = send_timeout_ms_;
  }
  uint64_t deadline_ms =
      timeout_ms ? Clock::QueryHostUptimeMillis() + timeout_ms : 0;

  if (cooperative_io_depth_.fetch_add(1) == 0) {
    SetHostNonBlocking(true);
  }
  auto* scheduler = self->kernel_state()->guest_scheduler();
  for (bool retried = false;; retried = true) {
    attempt();
    bool retry = ec == asio::error::would_block || ec == asio::error::try_again;
    if (mode == RetryMode::kConnect) {
      // A retried connect reports success by refusing to connect twice. A
      // socket connected before the call fails with WSAEISCONN.
      if (ec == asio::error::already_connected && retried) {
        ec.clear();
        break;
      }
      retry = retry || ec == asio::error::in_progress ||
              ec == asio::error::already_started;
    }
    if (!retry) {
      break;
    }
    if (deadline_ms && Clock::QueryHostUptimeMillis() >= deadline_ms) {
      // What a blocking socket reports when its SO_*TIMEO expires.
      ec = asio::error::timed_out;
      break;
    }
    scheduler->BlockCurrentThread();
  }
  if (cooperative_io_depth_.fetch_sub(1) == 1) {
    SetHostNonBlocking(false);
  }
}

X_STATUS XSocket::Connect(N_XSOCKADDR* name, int name_len) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    return X_STATUS_INVALID_HANDLE;
  }

  auto* addr_in = reinterpret_cast<N_XSOCKADDR_IN*>(name);
  asio::ip::address_v4 addr(addr_in->sin_addr);
  uint16_t port = addr_in->sin_port;
  // A console's low port is where its bind put it (Bind).
  if (systemlink::IsConsole(addr.to_uint())) {
    port = systemlink::HostPort(port);
  }

  asio::error_code ec;

  if (acceptor_) {
    // Transition from acceptor to tcp_socket for client connection.
    // The acceptor was created in Initialize() before we knew whether this
    // socket would listen or connect.
    acceptor_->close(ec);
    acceptor_.reset();

    tcp_socket_.emplace(GetIoContext());
    tcp_socket_->open(asio::ip::tcp::v4(), ec);
    if (ec) {
      last_error_ = AsioErrorToWSAError(ec);
      return X_STATUS_UNSUCCESSFUL;
    }

    asio::ip::tcp::endpoint endpoint(addr, port);
    RunCooperatively(ec, RetryMode::kConnect,
                     [&]() { tcp_socket_->connect(endpoint, ec); });
  } else if (tcp_socket_) {
    asio::ip::tcp::endpoint endpoint(addr, port);
    RunCooperatively(ec, RetryMode::kConnect,
                     [&]() { tcp_socket_->connect(endpoint, ec); });
  } else if (udp_socket_) {
    asio::ip::udp::endpoint endpoint(addr, port);
    udp_socket_->connect(endpoint, ec);
  }

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Bind(N_XSOCKADDR_IN* name, int name_len) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    return X_STATUS_INVALID_HANDLE;
  }

  // On Linux and Windows (when running under Wine), ports < 1024 require root
  // privileges. Remap to port + 10000 to avoid privilege issues.
  // Note: N_XSOCKADDR_IN uses xe::be<> which auto-converts to/from host endian.
  const uint16_t original_port = static_cast<uint16_t>(name->sin_port);
  const uint16_t new_port = systemlink::HostPort(original_port);
  port_shifted_ = new_port != original_port;
  if (port_shifted_) {
    name->sin_port = new_port;
    XELOGW("XSocket::Bind: port {} requires privileges, remapping to port {}",
           original_port, new_port);
  }

  asio::ip::address_v4 addr(name->sin_addr);
  uint16_t port = name->sin_port;

  // Reject duplicate active binds (Xbox 360 behaviour).  SO_REUSEADDR is set
  // on stream sockets for TIME_WAIT reclaiming but games expect a second bind
  // to the same port to fail with WSAEADDRINUSE.  Query the kernel's live
  // object table so the check is naturally correct across in-process relaunches
  // (Shutdown destroys the old object table).
  if (port != 0) {
    auto sockets =
        kernel_state()->object_table()->GetObjectsByType<XSocket>(kObjectType);
    for (auto& s : sockets) {
      if (s.get() != this && s->bound_ && s->bound_port_ == port &&
          s->type_ == type_) {
        last_error_ = 10048;  // WSAEADDRINUSE
        return X_STATUS_UNSUCCESSFUL;
      }
    }
  }

  asio::error_code ec;

  if (acceptor_) {
    asio::ip::tcp::endpoint endpoint(addr, port);
    acceptor_->bind(endpoint, ec);
  } else if (tcp_socket_) {
    asio::ip::tcp::endpoint endpoint(addr, port);
    tcp_socket_->bind(endpoint, ec);
  } else if (udp_socket_) {
    asio::ip::udp::endpoint endpoint(addr, port);
    udp_socket_->bind(endpoint, ec);
  }

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return X_STATUS_UNSUCCESSFUL;
  }

  bound_ = true;

  // Get the actual bound port (important when binding to port 0)
  if (acceptor_) {
    bound_port_ = acceptor_->local_endpoint(ec).port();
  } else if (tcp_socket_) {
    bound_port_ = tcp_socket_->local_endpoint(ec).port();
  } else if (udp_socket_) {
    bound_port_ = udp_socket_->local_endpoint(ec).port();
  }

  if (ec) {
    bound_port_ = port;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Listen(int backlog) {
  if (!acceptor_) {
    return X_STATUS_INVALID_HANDLE;
  }

  asio::error_code ec;

  acceptor_->listen(backlog, ec);
  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

namespace {
// Host byte order.
struct AcceptedPeer {
  uint32_t address;
  uint16_t port;
};
}  // namespace

// Writes the accepted peer as a guest sockaddr_in or clears it without one.
// Host sockaddr_in layouts differ (macOS starts with sin_len).
static void AcceptedName(N_XSOCKADDR* name, int* name_len,
                         std::optional<AcceptedPeer> peer) {
  if (name) {
    std::memset(name, 0, sizeof(*name));
  }
  if (!peer) {
    *name_len = 0;
    return;
  }
  if (name) {
    const xe::be<uint16_t> port = peer->port;
    const xe::be<uint32_t> address = peer->address;
    name->address_family = AF_INET;
    std::memcpy(name->sa_data, &port, sizeof(port));
    std::memcpy(name->sa_data + sizeof(port), &address, sizeof(address));
  }
  *name_len = sizeof(N_XSOCKADDR);
}

object_ref<XSocket> XSocket::Accept(N_XSOCKADDR* name, int* name_len) {
  if (!acceptor_) {
    // WSAEOPNOTSUPP for a datagram socket, WSAEINVAL before listen().
    last_error_ = udp_socket_ ? 10045 : uint32_t(X_WSAError::X_WSAEINVAL);
    return nullptr;
  }

  asio::error_code ec;

  // Accept a new connection
  asio::ip::tcp::socket new_socket(GetIoContext());
  asio::ip::tcp::endpoint peer_endpoint;
  RunCooperatively(ec, RetryMode::kAccept,
                   [&]() { acceptor_->accept(new_socket, peer_endpoint, ec); });

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    AcceptedName(name, name_len, std::nullopt);
    return nullptr;
  }
  const uint32_t peer_address = peer_endpoint.address().to_v4().to_uint();
  AcceptedName(name, name_len,
               AcceptedPeer{peer_address,
                            systemlink::IsConsole(peer_address)
                                ? systemlink::GuestPort(peer_endpoint.port())
                                : peer_endpoint.port()});

  // Create a kernel object to represent the new socket
  auto socket =
      object_ref<XSocket>(new XSocket(kernel_state_, std::move(new_socket)));
  socket->bound_ = true;
  // An accepted socket starts in the listener's non-blocking mode.
  if (guest_non_blocking_) {
    socket->SetHostNonBlocking(true);
    socket->guest_non_blocking_ = true;
  }

  return socket;
}

int XSocket::Shutdown(int how) {
  if (!tcp_socket_ && !udp_socket_) {
    last_error_ = 10057;  // WSAENOTCONN
    return -1;
  }

  asio::error_code ec;
  asio::socket_base::shutdown_type shutdown_type;

  switch (how) {
    case 0:
      shutdown_type = asio::socket_base::shutdown_receive;
      break;
    case 1:
      shutdown_type = asio::socket_base::shutdown_send;
      break;
    case 2:
    default:
      shutdown_type = asio::socket_base::shutdown_both;
      break;
  }

  if (tcp_socket_) {
    tcp_socket_->shutdown(shutdown_type, ec);
  }
  // UDP sockets don't support shutdown (connectionless protocol)

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return -1;
  }

  return 0;
}

int XSocket::Recv(uint8_t* buf, uint32_t buf_len, uint32_t flags) {
  if (!tcp_socket_ && !udp_socket_) {
    last_error_ = 10057;  // WSAENOTCONN
    return -1;
  }

  asio::error_code ec;
  size_t bytes_received = 0;

  if (udp_socket_) {
    RunCooperatively(ec, RetryMode::kReceive, [&]() {
      bytes_received =
          udp_socket_->receive(asio::buffer(buf, buf_len), flags, ec);
    });
  } else if (tcp_socket_) {
    RunCooperatively(ec, RetryMode::kReceive, [&]() {
      bytes_received =
          tcp_socket_->receive(asio::buffer(buf, buf_len), flags, ec);
    });
  }

  // A stream peer that closed the connection reads as 0 bytes.
  if (ec == asio::error::eof) {
    return 0;
  }
  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return -1;
  }

  return static_cast<int>(bytes_received);
}

int XSocket::RecvFrom(uint8_t* buf, uint32_t buf_len, uint32_t flags,
                      N_XSOCKADDR_IN* from, uint32_t* from_len) {
  if (!udp_socket_ && !tcp_socket_) {
    last_error_ = 10057;  // WSAENOTCONN
    return -1;
  }

  asio::error_code ec;
  size_t bytes_received = 0;

  if (udp_socket_) {
    asio::ip::udp::endpoint sender_endpoint;
    RunCooperatively(ec, RetryMode::kReceive, [&]() {
      for (;;) {
        bytes_received = udp_socket_->receive_from(asio::buffer(buf, buf_len),
                                                   sender_endpoint, 0, ec);
        if (ec || !IsOwnBroadcast(sender_endpoint, buf, bytes_received)) {
          break;
        }
        // A console never hears its own broadcast; the next datagram, or
        // would_block when there is none.
      }
    });

    if (!ec) {
      const uint32_t sender = sender_endpoint.address().to_v4().to_uint();
      systemlink::Heard(sender);
      if (systemlink::Enabled()) {
        systemlink::Trace(false, sender, sender_endpoint.port(), bound_port_,
                          buf, bytes_received);
      }
    }
    if (!ec && from) {
      const uint32_t sender = sender_endpoint.address().to_v4().to_uint();
      from->sin_family = AF_INET;
      from->sin_addr = sender;
      from->sin_port = systemlink::IsConsole(sender)
                           ? systemlink::GuestPort(sender_endpoint.port())
                           : sender_endpoint.port();
      std::memset(from->x_sin_zero, 0, sizeof(from->x_sin_zero));
    }
    if (from_len) {
      *from_len = sizeof(N_XSOCKADDR_IN);
    }
  } else if (tcp_socket_) {
    RunCooperatively(ec, RetryMode::kReceive, [&]() {
      bytes_received =
          tcp_socket_->receive(asio::buffer(buf, buf_len), flags, ec);
    });
  }

  // A stream peer that closed the connection reads as 0 bytes.
  if (ec == asio::error::eof) {
    return 0;
  }
  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return -1;
  }

  return static_cast<int>(bytes_received);
}

int XSocket::Send(const uint8_t* buf, uint32_t buf_len, uint32_t flags) {
  if (!tcp_socket_ && !udp_socket_) {
    last_error_ = 10057;  // WSAENOTCONN
    return -1;
  }

  asio::error_code ec;
  size_t bytes_sent = 0;

  if (udp_socket_) {
    RunCooperatively(ec, RetryMode::kSend, [&]() {
      bytes_sent = udp_socket_->send(asio::buffer(buf, buf_len), flags, ec);
    });
  } else if (tcp_socket_) {
    RunCooperatively(ec, RetryMode::kSend, [&]() {
      bytes_sent = tcp_socket_->send(asio::buffer(buf, buf_len), flags, ec);
    });
  }

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return -1;
  }

  return static_cast<int>(bytes_sent);
}

int XSocket::SendTo(uint8_t* buf, uint32_t buf_len, uint32_t flags,
                    N_XSOCKADDR_IN* to, uint32_t to_len) {
  if (!udp_socket_ && !tcp_socket_) {
    last_error_ = 10057;  // WSAENOTCONN
    return -1;
  }

  asio::error_code ec;
  size_t bytes_sent = 0;

  if (udp_socket_) {
    if (to) {
      asio::ip::address_v4 addr(to->sin_addr);
      uint16_t port = to->sin_port;
      if (systemlink::IsConsole(addr.to_uint())) {
        port = systemlink::HostPort(port);
      }
      const bool broadcast =
          systemlink::Enabled() && systemlink::IsBroadcast(addr.to_uint());
      if (broadcast) {
        addr = asio::ip::address_v4(systemlink::BroadcastAddress());
      }
      asio::ip::udp::endpoint endpoint(addr, port);

      if (broadcast) {
        NoteBroadcast(buf, buf_len);
      }
      RunCooperatively(ec, RetryMode::kSend, [&]() {
        bytes_sent = udp_socket_->send_to(asio::buffer(buf, buf_len), endpoint,
                                          flags, ec);
      });

      if (systemlink::Enabled()) {
        if (ec && ec != asio::error::would_block) {
          static std::atomic<int> failures_logged = 0;
          if (failures_logged++ < 16) {
            XELOGW("System link: sending to {} port {} failed: {}",
                   systemlink::Describe(addr.to_uint()), port, ec.message());
          }
        } else if (!ec) {
          systemlink::Trace(true, addr.to_uint(), port, bound_port_, buf,
                            bytes_sent);
        }
      }

      // And to the consoles the local network's broadcasts don't reach. Their
      // failures are theirs: the title sent its broadcast.
      if (broadcast && !ec) {
        for (uint32_t peer : systemlink::BroadcastPeers()) {
          asio::error_code peer_ec;
          udp_socket_->send_to(
              asio::buffer(buf, buf_len),
              asio::ip::udp::endpoint(asio::ip::address_v4(peer), port), flags,
              peer_ec);
        }
      }
    } else {
      // Send to connected endpoint
      RunCooperatively(ec, RetryMode::kSend, [&]() {
        bytes_sent = udp_socket_->send(asio::buffer(buf, buf_len), flags, ec);
      });
      if (systemlink::Enabled()) {
        asio::error_code peer_ec;
        const auto peer = udp_socket_->remote_endpoint(peer_ec);
        if (!ec && !peer_ec) {
          systemlink::Trace(true, peer.address().to_v4().to_uint(),
                            peer.port(), bound_port_, buf, bytes_sent);
        } else if (ec && ec != asio::error::would_block) {
          XELOGW("System link: sending on a connected socket failed: {}",
                 ec.message());
        }
      }
    }
  } else if (tcp_socket_) {
    RunCooperatively(ec, RetryMode::kSend, [&]() {
      bytes_sent = tcp_socket_->send(asio::buffer(buf, buf_len), flags, ec);
    });
  }

  if (ec) {
    last_error_ = AsioErrorToWSAError(ec);
    return -1;
  }

  return static_cast<int>(bytes_sent);
}

// Winsock FD_* flags split into asio wait_read / wait_write groups.
namespace {
constexpr uint32_t kFD_READ = 1;
constexpr uint32_t kFD_WRITE = 2;
constexpr uint32_t kFD_OOB = 4;
constexpr uint32_t kFD_ACCEPT = 8;
constexpr uint32_t kFD_CONNECT = 16;
constexpr uint32_t kFD_CLOSE = 32;
constexpr uint32_t kReadEvents = kFD_READ | kFD_ACCEPT | kFD_CLOSE | kFD_OOB;
constexpr uint32_t kWriteEvents = kFD_WRITE | kFD_CONNECT;
}  // namespace

int XSocket::WSAEventSelect(object_ref<XEvent> event, uint32_t flags) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    last_error_ = uint32_t(X_WSAError::X_WSAENOTSOCK);
    return -1;
  }

  asio::error_code ec;

  // WSAEventSelect implicitly sets the socket to non-blocking.
  if (acceptor_) {
    acceptor_->non_blocking(true, ec);
  } else if (tcp_socket_) {
    tcp_socket_->non_blocking(true, ec);
  } else if (udp_socket_) {
    udp_socket_->non_blocking(true, ec);
  }
  guest_non_blocking_ = true;

  std::lock_guard<std::mutex> lock(select_mutex_);

  // Cancel pending waits from any prior selection.
  if (selected_event_) {
    asio::error_code cancel_ec;
    if (acceptor_) {
      acceptor_->cancel(cancel_ec);
    } else if (tcp_socket_) {
      tcp_socket_->cancel(cancel_ec);
    } else if (udp_socket_) {
      udp_socket_->cancel(cancel_ec);
    }
  }

  selected_event_ = std::move(event);
  selected_event_flags_ = flags;

  if (flags == 0 || !selected_event_) {
    return 0;
  }

  const bool want_read = (flags & kReadEvents) != 0;
  const bool want_write = (flags & kWriteEvents) != 0;

  // Capture strong refs so the socket and event outlive any pending wait.
  auto handler = [self = retain_object(this),
                  ev = selected_event_](const asio::error_code& wait_ec) {
    if (wait_ec) {
      return;  // cancelled or socket closed
    }
    ev->Set(0, false);
  };

  if (want_read) {
    if (acceptor_) {
      acceptor_->async_wait(asio::socket_base::wait_read, handler);
    } else if (tcp_socket_) {
      tcp_socket_->async_wait(asio::socket_base::wait_read, handler);
    } else if (udp_socket_) {
      udp_socket_->async_wait(asio::socket_base::wait_read, handler);
    }
  }
  if (want_write) {
    if (acceptor_) {
      acceptor_->async_wait(asio::socket_base::wait_write, handler);
    } else if (tcp_socket_) {
      tcp_socket_->async_wait(asio::socket_base::wait_write, handler);
    } else if (udp_socket_) {
      udp_socket_->async_wait(asio::socket_base::wait_write, handler);
    }
  }

  return 0;
}

bool XSocket::QueuePacket(uint32_t src_ip, uint16_t src_port,
                          const uint8_t* buf, size_t len) {
  packet* pkt = reinterpret_cast<packet*>(new uint8_t[sizeof(packet) + len]);
  pkt->src_ip = src_ip;
  pkt->src_port = src_port;

  pkt->data_len = static_cast<uint16_t>(len);
  std::memcpy(pkt->data, buf, len);

  std::lock_guard<std::mutex> lock(incoming_packet_mutex_);
  incoming_packets_.push(reinterpret_cast<uint8_t*>(pkt));

  // TODO: Limit on number of incoming packets?
  return true;
}

X_STATUS XSocket::QueryName(bool peer, N_XSOCKADDR_IN* name) {
  auto handle = native_handle();
  if (handle == static_cast<uint64_t>(-1)) {
    last_error_ = AsioErrorToWSAError(asio::error::not_socket);
    return X_STATUS_INVALID_HANDLE;
  }

  sockaddr_in host_name = {};
  socklen_t len = sizeof(host_name);
  auto host_address = reinterpret_cast<sockaddr*>(&host_name);
  int result = peer ? getpeername(static_cast<int>(handle), host_address, &len)
                    : getsockname(static_cast<int>(handle), host_address, &len);
  if (result == -1) {
    last_error_ = HostSocketError();
    return X_STATUS_UNSUCCESSFUL;
  }

  const uint32_t address = ntohl(host_name.sin_addr.s_addr);
  uint16_t port = ntohs(host_name.sin_port);
  // The title sees the port it asked for (Bind), and a console's as it asked.
  if (peer ? systemlink::IsConsole(address) : port_shifted_) {
    port = systemlink::GuestPort(port);
  }
  name->sin_family = host_name.sin_family;
  name->sin_port = port;
  name->sin_addr = address;
  std::memset(name->x_sin_zero, 0, sizeof(name->x_sin_zero));
  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::GetSockName(N_XSOCKADDR_IN* name) {
  return QueryName(false, name);
}

X_STATUS XSocket::GetPeerName(N_XSOCKADDR_IN* name) {
  return QueryName(true, name);
}

static uint64_t BroadcastHash(const uint8_t* buf, size_t len) {
  return std::hash<std::string_view>()(
             std::string_view(reinterpret_cast<const char*>(buf), len)) ^
         len;
}

void XSocket::NoteBroadcast(const uint8_t* buf, size_t len) {
  std::lock_guard<std::mutex> lock(broadcasts_mutex_);
  broadcasts_sent_[broadcasts_next_++ % broadcasts_sent_.size()] =
      BroadcastHash(buf, len);
}

bool XSocket::IsOwnBroadcast(const asio::ip::udp::endpoint& sender,
                             const uint8_t* buf, size_t len) {
  // What this socket broadcast comes back from this device and its own port,
  // as it was sent. A datagram the title sent itself is not dropped.
  if (!systemlink::Enabled() || sender.port() != bound_port_ ||
      !systemlink::IsLocalAddress(sender.address().to_v4().to_uint())) {
    return false;
  }
  const uint64_t hash = BroadcastHash(buf, len);
  std::lock_guard<std::mutex> lock(broadcasts_mutex_);
  for (uint64_t& sent : broadcasts_sent_) {
    if (sent == hash) {
      sent = 0;
      return true;
    }
  }
  return false;
}

uint32_t XSocket::GetLastWSAError() const { return last_error_; }

}  // namespace kernel
}  // namespace xe
