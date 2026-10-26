#pragma once

// Scheduler hints for the threads on the frame's critical path, controlled by files in
// <game root>; files may hold "key=value" lines:
//
// Worker affinity, decided from the core topology: pins the worker threads to the cores outside
// the little cluster (capacity below 50% of the strongest), only when such little cores exist and
// at least 3 other cores remain; otherwise off. Re-applied after returning to the foreground.
//   thread_affinity.disabled  turns it off
//   thread_affinity.enabled   optional overrides:
//     cpus=4-7  (forces this set and skips the topology decision)
//     workers=FrustumCulling,UpdateResource,FrameSynchroniz  (thread name prefixes)
//
// Per-thread uclamp_min, opt-in (rejected with EPERM on some firmware):
//   thread_uclamp.enabled
//     main=512, render=512, worker=512  (0..1024)
//     workers=FrustumCulling,UpdateResource,FrameSynchroniz
namespace os::android::thread_hints
{
    // The thread running the guest entry point (unnamed SDL thread).
    void ApplyToMainThread();

    // Called with the full (untruncated) name whenever the current thread is named.
    void ApplyToCurrentThread(const char* name);
}
