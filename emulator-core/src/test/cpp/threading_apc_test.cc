// Native test of Thread::QueueUserCallback (the host side of guest APCs and
// NtAlertThread): callbacks queued at the same time to different threads must
// each run on their own target thread, during an alertable wait, and never on
// another thread or with no target at all. Also multi-object waits given the
// same handle twice, which the cooperative scheduler polls with a 0 ms timeout.
//
// Built for Android (the fork's XE_PLATFORM_xendroid path) as a static x86_64
// executable by tools/test-native-logic.sh, with the logging calls stubbed.

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "xenia/base/logging.h"
#include "xenia/base/threading.h"

namespace xe::logging {
bool ShouldLog(LogLevel, uint32_t) { return false; }
namespace internal {
std::pair<char*, size_t> GetThreadBuffer() {
  static thread_local char buffer[1024];
  return {buffer, sizeof(buffer)};
}
void AppendLogLine(LogLevel, const char, size_t) {}
}  // namespace internal
}  // namespace xe::logging

using namespace std::chrono_literals;
using xe::threading::Thread;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "FAILED: %s\n", what);
    ++failures;
  }
}

// One callback, on its own thread, during an alertable sleep.
void single_callback_runs_on_its_thread() {
  std::atomic<bool> stop{false};
  std::atomic<int> ran{0};
  std::atomic<Thread*> where{nullptr};
  Thread::CreationParameters params = {};
  auto thread = Thread::Create(params, [&] {
    while (!stop.load()) xe::threading::AlertableSleep(1ms);
  });
  Thread* target = thread.get();
  for (int i = 0; i < 200 && ran.load() == 0; ++i) {
    thread->QueueUserCallback([&] {
      where.store(Thread::GetCurrentThread());
      ran.fetch_add(1);
    });
    std::this_thread::sleep_for(2ms);
  }
  stop.store(true);
  check(xe::threading::Wait(thread.get(), false, 2s) ==
            xe::threading::WaitResult::kSuccess,
        "the alerted thread did not finish");
  check(ran.load() > 0, "the callback never ran during an alertable sleep");
  check(where.load() == target, "the callback ran on another thread");
}

// Several queuers alerting several targets at once: each callback must find
// itself on the thread it was queued to.
void concurrent_callbacks_stay_on_their_targets() {
  constexpr int kTargets = 4;
  constexpr int kQueuers = 3;
  constexpr int kRounds = 3000;
  std::atomic<bool> stop{false};
  std::atomic<int> ran[kTargets] = {};
  std::atomic<int> wrong{0};
  std::vector<std::unique_ptr<Thread>> targets;
  Thread::CreationParameters params = {};
  for (int i = 0; i < kTargets; ++i) {
    targets.push_back(Thread::Create(params, [&] {
      while (!stop.load()) xe::threading::AlertableSleep(500us);
    }));
  }
  std::vector<std::thread> queuers;
  for (int q = 0; q < kQueuers; ++q) {
    queuers.emplace_back([&, q] {
      for (int round = 0; round < kRounds; ++round) {
        int j = (round + q) % kTargets;
        Thread* target = targets[j].get();
        target->QueueUserCallback([&, j, target] {
          if (Thread::GetCurrentThread() != target) wrong.fetch_add(1);
          ran[j].fetch_add(1);
        });
      }
    });
  }
  for (auto& queuer : queuers) queuer.join();
  std::this_thread::sleep_for(20ms);
  stop.store(true);
  for (auto& target : targets) {
    check(xe::threading::Wait(target.get(), false, 2s) ==
              xe::threading::WaitResult::kSuccess,
          "an alerted thread did not finish");
  }
  int total = 0;
  for (auto& count : ran) total += count.load();
  std::printf("concurrent alerts: %d callbacks ran, %d on the wrong thread\n",
              total, wrong.load());
  check(wrong.load() == 0, "a callback ran on a thread it was not queued to");
  check(total > 0, "no callback ran");
}

// A handle passed twice: a wait-any still times out, then succeeds on the
// first slot, and a wait-all fails as on Windows. Each used to spin forever.
void repeated_handle_waits_end() {
  using xe::threading::WaitResult;
  auto event = xe::threading::Event::CreateManualResetEvent(false);
  auto other = xe::threading::Event::CreateManualResetEvent(false);
  auto any = xe::threading::WaitAny({event.get(), other.get(), event.get()},
                                    false, 0ms);
  check(any.first == WaitResult::kTimeout,
        "a wait-any on a repeated handle did not time out");
  event->Set();
  any = xe::threading::WaitAny({event.get(), other.get(), event.get()}, false,
                               0ms);
  check(any.first == WaitResult::kSuccess && any.second == 0,
        "a wait-any on a repeated handle did not end on its first slot");
  check(xe::threading::WaitAll({event.get(), other.get(), event.get()}, false,
                               0ms) == WaitResult::kFailed,
        "a wait-all on a repeated handle did not fail");
}

}  // namespace

int main() {
  single_callback_runs_on_its_thread();
  concurrent_callbacks_stay_on_their_targets();
  repeated_handle_waits_end();
  if (failures) {
    std::fprintf(stderr, "threading_apc_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("threading_apc_test: ok\n");
  return 0;
}
