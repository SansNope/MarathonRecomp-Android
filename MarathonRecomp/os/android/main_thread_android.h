#pragma once

#include <thread>

// Static initialisers run on the Java thread that loads libmain.so, but the game runs on SDLThread,
// which calls main() and then the guest entry point. Code meant for "the main thread" (pumping SDL
// events, which also delivers the pause and resume of the activity) compares against the id
// recorded at the start of main().
namespace os::android
{
    inline std::thread::id g_mainThreadId;

    inline void SetMainThread()
    {
        g_mainThreadId = std::this_thread::get_id();
    }

    inline bool IsMainThread()
    {
        return std::this_thread::get_id() == g_mainThreadId;
    }
}
