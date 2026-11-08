// Guest frame stats (xenia/base/frame_stats.h): the published FPS and frame time read 0
// once a second passes without a guest frame, instead of the last rate forever.
// Source events also carry their own cadence and publication identity to ZeroFG.

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "xenia/base/frame_stats.h"

namespace {

int failures = 0;

void expect(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "FAILED: %s\n", what);
    ++failures;
  }
}

void frames(int count) {
  for (int i = 0; i < count; ++i) {
    xe::RecordGuestPresent();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}

void source_publications() {
  constexpr int kEvents = 31;
  constexpr uint64_t kBackpressureNs = 3000000;
  const auto before = xe::GetSourceBoundarySnapshot();
  expect(before.totals.source_issue_total == 0 && before.issue_periods.count == 0,
         "guest FPS events do not fabricate Source issues");

  auto& evidence = xe::GetSourceContractEvidence();
  std::array<uint64_t, size_t(xe::SourceContractFault::kCount)> faults_before{};
  for (size_t i = 0; i < faults_before.size(); ++i) {
    faults_before[i] = evidence.counts[i].load(std::memory_order_acquire);
  }

  uint64_t previous_publication = 0;
  for (int i = 0; i < kEvents; ++i) {
    {
      xe::SourceBoundaryInflightScope issue_scope(
          xe::SourceBoundaryInflightScope::Kind::kIssue);
      xe::RecordGuestPresent();
      xe::RecordSourcePresent();
      const auto publication = xe::PrepareSourcePublicationTelemetry();
      expect(publication.issue_identity_valid,
             "a Source publication has its producer's issue identity");
      expect(publication.source_id == before.totals.source_issue_total + i + 1,
             "each Source event advances the publication's issue ID once");
      expect(publication.publication_id > previous_publication,
             "Source publication IDs increase");
      expect(publication.issue_time_ns != 0 &&
                 publication.publish_time_ns >= publication.issue_time_ns,
             "publication time follows a recorded Source issue");
      expect(publication.issue_thread_tag == publication.publish_thread_tag,
             "Source issue and publication retain the same producer");
      xe::CommitSourcePublicationTelemetry(publication, false);
      previous_publication = publication.publication_id;
    }
    if (i == 0) {
      // This wait belongs only to the interval ending at the next Source issue.
      xe::AddSourceBackpressureWaitNs(kBackpressureNs);
    }
    if (i + 1 < kEvents) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }

  const auto after = xe::GetSourceBoundarySnapshot();
  expect(after.totals.source_issue_total - before.totals.source_issue_total ==
             kEvents,
         "31 Source events produce 31 issues");
  expect(after.totals.source_published_total -
                 before.totals.source_published_total == kEvents,
         "31 Source events produce 31 committed publications");
  expect(after.issue_periods.consistent && after.issue_periods.count == kEvents - 1,
         "Source issues expose all 30 cadence intervals in a consistent snapshot");
  expect(after.publish_periods.consistent &&
             after.publish_periods.count == kEvents - 1,
         "Source publications expose all 30 cadence intervals");
  expect(after.issue_periods.ending_sequence -
                 before.issue_periods.ending_sequence == kEvents - 1,
         "each Source interval advances the cadence sequence once");
  uint64_t attributed_backpressure_ns = 0;
  for (size_t i = 0; i < after.issue_periods.count; ++i) {
    expect(after.issue_periods.intervals_ns[i] != 0,
           "Source cadence intervals are nonzero");
    expect(after.issue_periods.bp_wait_ns[i] == (i == 0 ? kBackpressureNs : 0),
           "backpressure is attributed to the next Source interval only");
    attributed_backpressure_ns += after.issue_periods.bp_wait_ns[i];
  }
  expect(attributed_backpressure_ns == kBackpressureNs,
         "one backpressure wait is counted exactly once");
  for (size_t i = 0; i < faults_before.size(); ++i) {
    expect(evidence.counts[i].load(std::memory_order_acquire) == faults_before[i],
           "valid Source publications add no contract faults");
  }
}

}  // namespace

int main() {
  using clock = std::chrono::steady_clock;
  float instant = -1, average = -1, fps = -1;
  xe::GetFrameStats(instant, average, fps);
  expect(instant == 0 && average == 0 && fps == 0, "nothing before the first frame");

  frames(12);
  const auto now = clock::now();
  xe::GetFrameStatsAt(now, instant, average, fps);
  std::printf("  running: %.1f FPS, %.1f ms\n", fps, instant);
  expect(fps > 5 && fps < 200 && instant > 0, "a running game reads its rate");
  xe::GetFrameStatsAt(now + std::chrono::milliseconds(900), instant, average, fps);
  expect(fps > 0, "within the window the last rate stands");
  xe::GetFrameStatsAt(now + std::chrono::milliseconds(1100), instant, average, fps);
  expect(instant == 0 && average == 0 && fps == 0, "a second without frames reads 0");

  // A real stall, then the game resumes.
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  xe::GetFrameStats(instant, average, fps);
  expect(fps == 0 && instant == 0, "stalled: 0, not the rate before");
  frames(10);
  xe::GetFrameStats(instant, average, fps);
  std::printf("  resumed: %.1f FPS, %.1f ms\n", fps, instant);
  expect(fps > 5 && instant > 0, "frames again: a rate again");

  source_publications();

  if (failures) {
    std::fprintf(stderr, "frame_stats_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("frame stats: passed\n");
  return 0;
}
