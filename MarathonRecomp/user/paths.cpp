#include "paths.h"
#include <os/process.h>

std::filesystem::path g_executableRoot = os::process::GetExecutableRoot();
#ifndef __ANDROID__
std::filesystem::path g_userPath = BuildUserPath();
#endif

bool CheckPortable()
{
    return std::filesystem::exists(g_executableRoot / "portable.txt");
}

std::filesystem::path BuildUserPath()
{
    if (CheckPortable())
        return g_executableRoot;

    std::filesystem::path userPath;

#if defined(_WIN32)
    PWSTR knownPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &knownPath) == S_OK)
        userPath = std::filesystem::path{ knownPath } / USER_DIRECTORY;

    CoTaskMemFree(knownPath);
#elif defined(__ANDROID__)
    // An app UID has no meaningful $HOME/passwd entry; keep config and saves
    // next to the game files in app-specific storage (AppStorage.configFile() in the launcher).
    userPath = os::android::GetDataRoot() / ".config" / USER_DIRECTORY;
#elif defined(__linux__) || defined(__APPLE__)
    const char* homeDir = getenv("HOME");
#if defined(__linux__)
    if (homeDir == nullptr)
    {
        homeDir = getpwuid(getuid())->pw_dir;
    }
#endif

    if (homeDir != nullptr)
    {
        // Prefer to store in the .config directory if it exists. Use the home directory otherwise.
        std::filesystem::path homePath = homeDir;
#if defined(__linux__)
        const char* configDir = getenv("XDG_CONFIG_HOME");
        std::filesystem::path configPath = "";
        if (configDir == nullptr)
        {
            configPath = homePath / ".config";
        } else
        {
            configPath = configDir;
        }

#else
        std::filesystem::path configPath = homePath / "Library" / "Application Support";
#endif
        if (std::filesystem::exists(configPath))
            userPath = configPath / USER_DIRECTORY;
        else
            userPath = homePath / ("." USER_DIRECTORY);
    }
#else
    static_assert(false, "GetUserPath() not implemented for this platform.");
#endif

    return userPath;
}

const std::filesystem::path& GetUserPath()
{
#ifdef __ANDROID__
    // Lazy: the path is resolved through SDL/JNI, which isn't available yet
    // when the static initializers of libmain.so run.
    static std::filesystem::path g_userPath = BuildUserPath();
#endif
    return g_userPath;
}
