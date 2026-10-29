#include "guest_waits_android.h"
#include <cpu/ppc_context.h>
#include <os/android/sampling_profiler_android.h>
#include <os/logger.h>

#include <algorithm>
#include <atomic>
#include <ctime>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
    using os::android::waits::Kind;
    using os::android::waits::TimeoutPath;

    constexpr size_t KindCount = size_t(Kind::Count);
    constexpr const char* KindNames[KindCount] = { "event", "sem", "multi", "cs", "sleep", "yield", "io", "audio", "thread" };

    std::atomic<uint64_t> g_kindNs[KindCount]{};

    // Wait time per guest caller. Written by the guest main thread only; slots are never freed, a
    // full table (or no caller) counts as "rest".
    constexpr size_t SiteCount = 1024;
    constexpr size_t MaxProbes = 16;

    struct Site
    {
        std::atomic<uint32_t> caller{ 0 };
        std::atomic<uint8_t> kind{ 0 };
        std::atomic<uint64_t> ns{ 0 };
        std::atomic<uint32_t> waits{ 0 };
    };

    Site g_sites[SiteCount];
    std::atomic<uint64_t> g_otherNs{ 0 };
    std::atomic<uint32_t> g_otherWaits{ 0 };

    Site* FindSite(uint32_t caller, Kind kind)
    {
        if (caller == 0)
            return nullptr;

        size_t index = ((caller >> 2) * 2654435761u) & (SiteCount - 1);
        for (size_t probe = 0; probe < MaxProbes; probe++, index = (index + 1) & (SiteCount - 1))
        {
            Site& site = g_sites[index];
            const uint32_t current = site.caller.load(std::memory_order_relaxed);
            if (current == caller)
                return &site;

            if (current == 0)
            {
                site.kind.store(uint8_t(kind), std::memory_order_relaxed);
                site.caller.store(caller, std::memory_order_relaxed);
                return &site;
            }
        }

        return nullptr;
    }

    // The first signal of the object the main thread is blocked on (BeginObjectWait resets it).
    std::atomic<int64_t> g_signalNs{ 0 };
    std::atomic<int32_t> g_signalTid{ 0 };

    // Object waits per signalling thread; slot 0 holds the waits without a recorded signal.
    constexpr size_t WakerCount = 64;

    struct Waker
    {
        std::atomic<int32_t> tid{ 0 };
        std::atomic<uint64_t> toSignalNs{ 0 };
        std::atomic<uint64_t> wakeNs{ 0 };
        std::atomic<uint64_t> maxWakeNs{ 0 };
        std::atomic<uint32_t> waits{ 0 };
    };

    Waker g_wakers[WakerCount];

    Waker& FindWaker(int32_t tid)
    {
        if (tid <= 0)
            return g_wakers[0];

        for (size_t i = 1; i < WakerCount; i++)
        {
            const int32_t current = g_wakers[i].tid.load(std::memory_order_relaxed);
            if (current == tid)
                return g_wakers[i];

            if (current == 0)
            {
                g_wakers[i].tid.store(tid, std::memory_order_relaxed);
                return g_wakers[i];
            }
        }

        return g_wakers[0];
    }

    std::string ThreadName(int32_t tid)
    {
        std::ifstream file(fmt::format("/proc/self/task/{}/comm", tid));
        std::string name;
        if (!std::getline(file, name) || name.empty())
            name = "exited";
        return fmt::format("{}:{}", name, tid);
    }

    constexpr size_t TimeoutPathCount = size_t(TimeoutPath::Count);
    std::atomic<uint32_t> g_timeoutCalls[TimeoutPathCount]{};
    std::atomic<uint32_t> g_timeoutTimedOut[TimeoutPathCount]{};
}

// The game's WaitForSingleObject: it tail-calls the WaitForSingleObjectEx wrapper, which overwrites
// the link register before NtWaitForSingleObjectEx, so the real caller is taken here.
PPC_FUNC_IMPL(__imp__sub_82537DC0);
PPC_FUNC(sub_82537DC0)
{
    const uint32_t previous = os::android::waits::t_waitCaller;
    os::android::waits::t_waitCaller = uint32_t(ctx.lr);
    __imp__sub_82537DC0(ctx, base);
    os::android::waits::t_waitCaller = previous;
}

void os::android::waits::SetMainThread()
{
    t_mainThread = os::android::perf::IsCaptureRequested();
}

uint32_t os::android::waits::GuestCaller()
{
    if (t_waitCaller != 0)
        return t_waitCaller;

    const PPCContext* context = g_ppcContext;
    return context != nullptr ? uint32_t(context->lr) : 0;
}

int64_t os::android::waits::NowNs()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return int64_t(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

void os::android::waits::BeginObjectWait(const void* object)
{
    g_signalNs.store(0, std::memory_order_relaxed);
    g_signalTid.store(0, std::memory_order_relaxed);
    g_waitObject.store(object, std::memory_order_release);
}

void os::android::waits::SignalSlow(const void* object)
{
    int64_t expected = 0;
    const int64_t now = NowNs();
    if (g_signalNs.compare_exchange_strong(expected, now, std::memory_order_relaxed))
        g_signalTid.store(int32_t(gettid()), std::memory_order_relaxed);
}

void os::android::waits::Record(Kind kind, uint32_t caller, int64_t startNs, const void* object)
{
    const int64_t end = NowNs();
    const uint64_t ns = uint64_t(std::max<int64_t>(end - startNs, 0));
    g_kindNs[size_t(kind)].fetch_add(ns, std::memory_order_relaxed);

    if (Site* site = FindSite(caller, kind))
    {
        site->ns.fetch_add(ns, std::memory_order_relaxed);
        site->waits.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
        g_otherNs.fetch_add(ns, std::memory_order_relaxed);
        g_otherWaits.fetch_add(1, std::memory_order_relaxed);
    }

    if (object == nullptr)
        return;

    g_waitObject.store(nullptr, std::memory_order_relaxed);
    const int64_t signalNs = g_signalNs.load(std::memory_order_relaxed);
    const bool signalled = signalNs >= startNs && signalNs <= end;
    Waker& waker = FindWaker(signalled ? g_signalTid.load(std::memory_order_relaxed) : 0);
    const uint64_t toSignal = signalled ? uint64_t(signalNs - startNs) : ns;
    const uint64_t wake = signalled ? uint64_t(end - signalNs) : 0;
    waker.toSignalNs.fetch_add(toSignal, std::memory_order_relaxed);
    waker.wakeNs.fetch_add(wake, std::memory_order_relaxed);
    if (wake > waker.maxWakeNs.load(std::memory_order_relaxed))
        waker.maxWakeNs.store(wake, std::memory_order_relaxed);
    waker.waits.fetch_add(1, std::memory_order_relaxed);
}

void os::android::waits::CountTimeout(TimeoutPath path, bool timedOut)
{
    g_timeoutCalls[size_t(path)].fetch_add(1, std::memory_order_relaxed);
    if (timedOut)
        g_timeoutTimedOut[size_t(path)].fetch_add(1, std::memory_order_relaxed);
}

std::string os::android::waits::TakeFrameSummary(uint32_t frames)
{
    const double perFrame = frames ? 1.0 / (1e6 * frames) : 0.0;
    std::string text;
    for (size_t i = 0; i < KindCount; i++)
        text += fmt::format("{}{}:{:.2f}", i ? " " : "", KindNames[i], double(g_kindNs[i].exchange(0, std::memory_order_relaxed)) * perFrame);

    return text;
}

std::string os::android::waits::TakeTopCallers(uint32_t frames, size_t count)
{
    struct Entry
    {
        uint32_t caller;
        uint8_t kind;
        uint64_t ns;
        uint32_t waits;
    };

    std::vector<Entry> entries;
    for (Site& site : g_sites)
    {
        const uint32_t caller = site.caller.load(std::memory_order_relaxed);
        if (caller == 0)
            continue;

        const uint64_t ns = site.ns.exchange(0, std::memory_order_relaxed);
        const uint32_t waits = site.waits.exchange(0, std::memory_order_relaxed);
        if (ns != 0)
            entries.push_back({ caller, site.kind.load(std::memory_order_relaxed), ns, waits });
    }

    const uint64_t otherNs = g_otherNs.exchange(0, std::memory_order_relaxed);
    const uint32_t otherWaits = g_otherWaits.exchange(0, std::memory_order_relaxed);
    if (entries.empty() && otherNs == 0)
        return {};

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.ns > b.ns; });

    const double perFrame = frames ? 1.0 / (1e6 * frames) : 0.0;
    std::string text;
    uint64_t restNs = otherNs;
    uint32_t restWaits = otherWaits;
    for (size_t i = 0; i < entries.size(); i++)
    {
        const Entry& entry = entries[i];
        if (i >= count)
        {
            restNs += entry.ns;
            restWaits += entry.waits;
            continue;
        }

        text += fmt::format("{}0x{:08X} {} {:.2f} ms/frame ({} waits)", text.empty() ? "" : ", ",
            entry.caller, KindNames[std::min<size_t>(entry.kind, KindCount - 1)], double(entry.ns) * perFrame, entry.waits);
    }

    if (restNs != 0)
        text += fmt::format("{}rest {:.2f} ms/frame ({} waits)", text.empty() ? "" : ", ", double(restNs) * perFrame, restWaits);

    return text;
}

std::string os::android::waits::TakeWakers(uint32_t frames, size_t count)
{
    struct Entry
    {
        int32_t tid;
        uint64_t toSignal;
        uint64_t wake;
        uint64_t maxWake;
        uint32_t waits;
    };

    std::vector<Entry> entries;
    for (size_t i = 0; i < WakerCount; i++)
    {
        Waker& waker = g_wakers[i];
        const int32_t tid = waker.tid.load(std::memory_order_relaxed);
        if (i != 0 && tid == 0)
            continue;

        const uint32_t waits = waker.waits.exchange(0, std::memory_order_relaxed);
        const uint64_t toSignal = waker.toSignalNs.exchange(0, std::memory_order_relaxed);
        const uint64_t wake = waker.wakeNs.exchange(0, std::memory_order_relaxed);
        const uint64_t maxWake = waker.maxWakeNs.exchange(0, std::memory_order_relaxed);
        if (waits != 0)
            entries.push_back({ i == 0 ? 0 : tid, toSignal, wake, maxWake, waits });
    }

    if (entries.empty())
        return {};

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.toSignal + a.wake > b.toSignal + b.wake; });

    const double perFrame = frames ? 1.0 / (1e6 * frames) : 0.0;
    std::string text;
    for (size_t i = 0; i < entries.size() && i < count; i++)
    {
        const Entry& entry = entries[i];
        text += fmt::format("{}{} {:.2f} ms/frame (to signal {:.2f}, wake {:.2f}, max wake {:.1f} ms; {} waits)",
            text.empty() ? "" : ", ", entry.tid ? ThreadName(entry.tid) : std::string("none"),
            double(entry.toSignal + entry.wake) * perFrame, double(entry.toSignal) * perFrame, double(entry.wake) * perFrame,
            double(entry.maxWake) / 1e6, entry.waits);
    }

    return text;
}

std::string os::android::waits::TakeTimeouts()
{
    constexpr const char* Names[TimeoutPathCount] = { "event", "sem", "thread", "kewait_single" };
    std::string text;
    for (size_t i = 0; i < TimeoutPathCount; i++)
    {
        const uint32_t calls = g_timeoutCalls[i].exchange(0, std::memory_order_relaxed);
        const uint32_t timedOut = g_timeoutTimedOut[i].exchange(0, std::memory_order_relaxed);
        if (calls != 0)
            text += fmt::format("{}{}={} ({} timed out)", text.empty() ? "" : " ", Names[i], calls, timedOut);
    }

    return text;
}
