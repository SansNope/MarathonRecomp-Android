#pragma once

namespace os::android
{
    // App-private internal files directory (e.g. /data/user/0/<pkg>/files).
    // Empty if SDL/JNI is not ready yet, so never call this from a static initializer.
    const std::filesystem::path& GetInternalFilesDir();

    // App-specific external files directory (e.g. /storage/emulated/0/Android/data/<pkg>/files).
    // Reachable from a PC over USB (MTP) without root and needs no runtime permissions.
    // Empty if unavailable; never call from a static initializer.
    const std::filesystem::path& GetExternalFilesDir();

    // App-specific media directory (e.g. /storage/emulated/0/Android/media/<pkg>).
    // Unlike Android/data, on-device file managers can browse it on Android 11+,
    // so users can drop driver files there without a PC. Empty if unavailable.
    const std::filesystem::path& GetExternalMediaDir();

    // Root for game files, config and saves. Asks the activity for getGameStoragePath()
    // (the launcher's AppStorage policy) over JNI and falls back to
    // GetExternalFilesDir()/MarathonRecomp. Fixed for the game session;
    // an unusable directory is reported to the user and terminates the process.
    const std::filesystem::path& GetDataRoot();
}
