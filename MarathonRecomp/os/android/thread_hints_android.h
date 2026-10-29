#pragma once

#include <string>
#include <sys/types.h>

// Scheduler hints for the threads on the frame's critical path, controlled by files in
// <game root>; files may hold "key=value" lines:
//
// Affinity, decided from the core topology: pins the worker threads and the guest main and render
// threads to the cores outside
// the little cluster (capacity below 50% of the strongest), only when such little cores exist and
// at least 3 other cores remain; otherwise off. Re-applied after returning to the foreground and
// checked every 2 seconds.
//   thread_affinity.disabled  turns it off
//   thread_affinity.enabled   optional overrides:
//     cpus=4-7  (forces this set and skips the topology decision)
//     workers=FrustumCulling,UpdateResource,FrameSynchroniz,ModelStandard,ManageParticle,HavokRunner,DocMarathon  (thread name prefixes)
//     main_render=0  (keeps the guest main and render threads unpinned; pinned by default)
//   thread_affinity_main_strongest.enabled  experiment (a launcher switch, off by default): the
//     guest main thread only on the strongest cores (equal top capacity, at least 2 of them, e.g.
//     6-7 on a Tensor G2); the other pinned threads keep the set above.
//
// Per-thread uclamp_min, opt-in. Android common kernels reject any uclamp change from a process
// without CAP_SYS_NICE (EPERM), so it only works on non-GKI kernels:
//   thread_uclamp.enabled
//     main=512, render=512, worker=512  (0..1024)
//     workers=FrustumCulling,UpdateResource,FrameSynchroniz
namespace os::android::thread_hints
{
    // The thread running the guest entry point (unnamed SDL thread).
    void ApplyToMainThread();

    // Called with the full (untruncated) name whenever the current thread is named.
    void ApplyToCurrentThread(const char* name);

    // Once per frame from the presenting thread: every 2 seconds restores the mask of pinned
    // threads that lost it (a cpuset change after the return to the foreground resets it).
    void CheckAffinity();

    // The uclamp_min/uclamp_max a thread runs with (set by the power HAL for ADPF sessions), e.g.
    // "480/1024"; "n/a" when it cannot be read.
    std::string ThreadUclamp(pid_t tid);
}
