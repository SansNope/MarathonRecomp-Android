#pragma once

#include <string>
#include <sys/types.h>

// Diagnostics for testers' logs, active with perf_sampling.enabled (a launcher switch, off by
// default). Every 20 seconds log.txt gets:
//   [prof]    where the sampled threads spend CPU time: a sample per 1 ms of thread CPU time (in
//             practice per scheduler tick, weighted by the timer overruns),
//             counted per exported function of libmain.so (recompiled functions are exported) or
//             per library, with the most common callers;
//   [threads] CPU and run-queue wait per thread (/proc/self/task/*/schedstat);
//   [cpufreq] current and maximum frequency per core, thermal status and headroom.
// Nothing about scheduling, rendering or game settings is changed.
namespace os::android::profiler
{
    // Starts sampling the calling thread (at most 4 threads).
    void SampleCurrentThread(const char* label);
}

namespace os::android::perf
{
    // perf_capture.enabled exists (a launcher switch, off by default).
    bool IsCaptureRequested();

    // Current MHz of each cluster (cores grouped by their maximum frequency, lowest first), e.g.
    // "930/910/500"; an instant reading, not an average. "n/a" when cpufreq is not readable.
    std::string ClusterClocks();

    // The core a thread of this process last ran on (stat field 39), -1 when unknown.
    int ThreadCore(pid_t tid);
}
