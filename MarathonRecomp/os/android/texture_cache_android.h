#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Disk cache of BC -> ETC2/EAC transcoded textures (on by default; texture_cache.disabled, a
// launcher switch, turns it off). It is only used when BC textures are transcoded, so devices with
// BC support never touch it. Without BC support every BC texture is transcoded on the CPU at each load (on a
// Pixel 7 Pro 9 + 5.5% of the guest thread's samples while loading); the transcoded upload data
// depends only on the source DDS and the target format, so it is stored under a hash of both in
// <internal files>/texture_cache and read back on later loads. Entries carry their size and a
// hash of their contents and are ignored when either does not match; the cache stops growing at
// 1 GB. Statistics (also without the cache) go to [texture-cache] lines with perf_capture.enabled.
namespace os::android::texture_cache
{
    struct Key
    {
        uint64_t low = 0;
        uint64_t high = 0;
    };

    bool IsEnabled();

    Key MakeKey(const void* source, size_t sourceSize, uint32_t targetFormat, size_t payloadSize);

    // Fills `out` with `size` bytes; false on a miss or a damaged entry.
    bool Read(const Key& key, uint8_t* out, size_t size);

    void Write(const Key& key, const uint8_t* data, size_t size);

    // A texture transcoded on the CPU, for the statistics.
    void CountTranscode(int64_t ns, size_t bytes);

    // Hits, misses, transcoding and cache size since the previous call; empty when nothing was
    // loaded.
    std::string TakeStats();
}
