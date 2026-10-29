#pragma once

// Where the guest main thread blocks between frames, for the [perf] lines (perf_capture.enabled,
// a launcher switch). The thread's wall time per frame exceeds its CPU time by several ms on a
// Pixel 7 Pro; those idle gaps keep the governor's frequency low, so the kernel objects it waits
// on (and file reads, the XMA lock, thread joins) are timed only on the guest main thread and only
// on their blocking path, attributed to the guest caller (the link register of the import call,
// or the caller of the game's WaitForSingleObject, whose wrapper hides it).
// For events and semaphores the thread that signals the object is recorded too, splitting each
// wait into the time until the signal and the wake-up delay after it.
// GUEST_WAIT_SCOPE(Kind) times the rest of the enclosing block; GUEST_WAIT_SCOPE_ON also names
// the object waited on, GUEST_WAIT_SIGNAL(object) marks it signalled, GUEST_WAIT_TIMEOUT counts
// waits with a finite timeout (on all threads). All compile to nothing outside Android.
#ifdef __ANDROID__

#include <atomic>
#include <cstdint>
#include <string>

namespace os::android::waits
{
    enum class Kind : uint8_t
    {
        Event,
        Semaphore,
        Multiple,
        CriticalSection,
        Sleep,
        Yield,
        Io,
        Audio,
        Thread,
        Count
    };

    // Waits with a finite, non-zero timeout, which poll until their deadline since perf11 (before,
    // see docs/PERFORMANCE-AUDIT-2026-10-29.md, R1); counted to see whether the game uses them.
    enum class TimeoutPath : uint8_t
    {
        Event,
        Semaphore,
        Thread,
        KeWaitSingle,   // KeWaitForSingleObject with any object
        Count
    };

    inline thread_local bool t_mainThread = false;

    // The caller of the game's WaitForSingleObject (sub_82537DC0) while it runs.
    inline thread_local uint32_t t_waitCaller = 0;

    // The object the guest main thread is blocked on.
    inline std::atomic<const void*> g_waitObject{ nullptr };

    // Called on the thread that runs the guest entry point, before it starts.
    void SetMainThread();

    // The guest return address of the current import call, 0 outside guest code.
    uint32_t GuestCaller();

    void BeginObjectWait(const void* object);
    void Record(Kind kind, uint32_t caller, int64_t startNs, const void* object);
    void SignalSlow(const void* object);
    void CountTimeout(TimeoutPath path, bool timedOut);

    int64_t NowNs();

    inline void NotifySignal(const void* object)
    {
        if (g_waitObject.load(std::memory_order_relaxed) == object)
            SignalSlow(object);
    }

    class Scope
    {
    public:
        Scope(Kind kind, uint32_t caller, const void* object = nullptr)
        {
            if (!t_mainThread)
                return;
            m_kind = kind;
            m_caller = caller;
            m_object = object;
            if (object != nullptr)
                BeginObjectWait(object);
            m_start = NowNs();
        }

        ~Scope()
        {
            if (m_start != 0)
                Record(m_kind, m_caller, m_start, m_object);
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        int64_t m_start = 0;
        const void* m_object = nullptr;
        uint32_t m_caller = 0;
        Kind m_kind = Kind::Event;
    };

    // Wait time per kind since the previous call, in ms per frame, e.g.
    // "event:1.20 sem:0.00 multi:0.00 cs:0.31 sleep:0.00 yield:0.02 io:0.00 audio:0.00 thread:0.00".
    std::string TakeFrameSummary(uint32_t frames);

    // The guest callers with the most wait time since the previous call (at most `count`), e.g.
    // "0x825B1A28 cs 2.31 ms/frame (812 waits), ..."; empty when nothing waited.
    std::string TakeTopCallers(uint32_t frames, size_t count);

    // The threads that signalled the objects the main thread waited on, e.g.
    // "FrustumCullingT:1234 2.10 ms/frame (to signal 1.80, wake 0.30, max wake 2.1 ms; 1200 waits)";
    // "none" collects waits that ended without a recorded signal.
    std::string TakeWakers(uint32_t frames, size_t count);

    // Finite-timeout waits on all threads since the previous call; empty when there were none.
    std::string TakeTimeouts();
}

#define GUEST_WAIT_SCOPE(kind) ::os::android::waits::Scope guestWaitScope_(::os::android::waits::Kind::kind, \
    ::os::android::waits::t_mainThread ? ::os::android::waits::GuestCaller() : 0)

#define GUEST_WAIT_SCOPE_ON(kind, object) ::os::android::waits::Scope guestWaitScope_(::os::android::waits::Kind::kind, \
    ::os::android::waits::t_mainThread ? ::os::android::waits::GuestCaller() : 0, object)

#define GUEST_WAIT_SIGNAL(object) ::os::android::waits::NotifySignal(object)

#define GUEST_WAIT_TIMEOUT(path, timedOut) ::os::android::waits::CountTimeout(::os::android::waits::TimeoutPath::path, timedOut)

#else

#define GUEST_WAIT_SCOPE(kind)
#define GUEST_WAIT_SCOPE_ON(kind, object)
#define GUEST_WAIT_SIGNAL(object)
#define GUEST_WAIT_TIMEOUT(path, timedOut)

#endif
