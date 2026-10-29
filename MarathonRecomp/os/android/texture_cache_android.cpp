#include "texture_cache_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <xxhash.h>

namespace
{
    // Bump when the transcoder or the upload layout changes: old entries then stop matching.
    constexpr uint32_t CacheVersion = 1;
    constexpr uint32_t Magic = 0x4354524D; // "MRTC"
    constexpr uint64_t MaxCacheBytes = 1ull << 30;

    struct Header
    {
        uint32_t magic;
        uint32_t version;
        uint64_t payloadSize;
        uint64_t payloadHash;
    };

    std::atomic<uint64_t> g_cacheBytes{ 0 };
    std::atomic<uint32_t> g_hits{ 0 }, g_misses{ 0 }, g_writes{ 0 }, g_damaged{ 0 }, g_transcodes{ 0 };
    std::atomic<uint64_t> g_hitBytes{ 0 }, g_hitNs{ 0 }, g_writeBytes{ 0 }, g_transcodeBytes{ 0 }, g_transcodeNs{ 0 };
    std::atomic<bool> g_full{ false };

    int64_t NowNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    const std::filesystem::path& Directory()
    {
        static const std::filesystem::path s_directory = []
        {
            std::filesystem::path directory = os::android::GetInternalFilesDir();
            if (directory.empty())
                return directory;

            directory /= "texture_cache";
            std::error_code error;
            std::filesystem::create_directories(directory, error);

            uint64_t bytes = 0;
            uint32_t files = 0;
            for (const auto& entry : std::filesystem::directory_iterator(directory, error))
            {
                if (entry.is_regular_file(error))
                {
                    bytes += entry.file_size(error);
                    ++files;
                }
            }

            g_cacheBytes = bytes;
            LOGFN("Texture cache: {} ({} files, {:.1f} MB of {} MB).", directory.string(), files, double(bytes) / 1048576.0, MaxCacheBytes >> 20);
            return directory;
        }();

        return s_directory;
    }

    std::filesystem::path EntryPath(const os::android::texture_cache::Key& key)
    {
        return Directory() / fmt::format("{:016x}{:016x}.etc", key.high, key.low);
    }
}

bool os::android::texture_cache::IsEnabled()
{
    static const bool s_enabled = []
    {
        std::error_code error;
        const bool disabled = std::filesystem::exists(os::android::GetDataRoot() / "texture_cache.disabled", error);
        if (disabled)
            LOGN("Texture cache disabled by texture_cache.disabled.");
        else
            LOGN("Texture cache enabled for transcoded BC textures.");
        return !disabled;
    }();

    return s_enabled && !Directory().empty();
}

os::android::texture_cache::Key os::android::texture_cache::MakeKey(const void* source, size_t sourceSize, uint32_t targetFormat, size_t payloadSize)
{
    const uint64_t seed = (uint64_t(CacheVersion) << 48) ^ (uint64_t(targetFormat) << 32) ^ uint64_t(payloadSize);
    const XXH128_hash_t hash = XXH3_128bits_withSeed(source, sourceSize, seed);
    return { hash.low64, hash.high64 };
}

bool os::android::texture_cache::Read(const Key& key, uint8_t* out, size_t size)
{
    const int64_t start = NowNs();
    FILE* file = fopen(EntryPath(key).c_str(), "rb");
    if (file == nullptr)
    {
        g_misses++;
        return false;
    }

    Header header{};
    thread_local std::vector<uint8_t> s_payload;
    bool valid = fread(&header, sizeof(header), 1, file) == 1 && header.magic == Magic && header.version == CacheVersion &&
        header.payloadSize == size;
    if (valid)
    {
        s_payload.resize(size);
        valid = fread(s_payload.data(), 1, size, file) == size && XXH3_64bits(s_payload.data(), size) == header.payloadHash;
    }
    fclose(file);

    if (!valid)
    {
        g_damaged++;
        g_misses++;
        return false;
    }

    memcpy(out, s_payload.data(), size);
    g_hits++;
    g_hitBytes += size;
    g_hitNs += uint64_t(NowNs() - start);
    return true;
}

void os::android::texture_cache::Write(const Key& key, const uint8_t* data, size_t size)
{
    if (g_cacheBytes.load() + size + sizeof(Header) > MaxCacheBytes)
    {
        if (!g_full.exchange(true))
            LOGFN("Texture cache: full ({} MB), new textures are no longer stored.", MaxCacheBytes >> 20);
        return;
    }

    const std::filesystem::path path = EntryPath(key);
    const std::filesystem::path temporary = path.string() + fmt::format(".{}.tmp", gettid());
    FILE* file = fopen(temporary.c_str(), "wb");
    if (file == nullptr)
        return;

    const Header header{ Magic, CacheVersion, size, XXH3_64bits(data, size) };
    const bool written = fwrite(&header, sizeof(header), 1, file) == 1 && fwrite(data, 1, size, file) == size;
    const bool closed = fclose(file) == 0;

    std::error_code error;
    if (!written || !closed)
    {
        std::filesystem::remove(temporary, error);
        return;
    }

    std::filesystem::rename(temporary, path, error);
    if (error)
    {
        std::filesystem::remove(temporary, error);
        return;
    }

    g_cacheBytes += size + sizeof(Header);
    g_writes++;
    g_writeBytes += size;
}

void os::android::texture_cache::CountTranscode(int64_t ns, size_t bytes)
{
    g_transcodes++;
    g_transcodeNs += uint64_t(std::max<int64_t>(ns, 0));
    g_transcodeBytes += bytes;
}

std::string os::android::texture_cache::TakeStats()
{
    const uint32_t hits = g_hits.exchange(0), misses = g_misses.exchange(0), writes = g_writes.exchange(0);
    const uint32_t damaged = g_damaged.exchange(0), transcodes = g_transcodes.exchange(0);
    const uint64_t hitBytes = g_hitBytes.exchange(0), hitNs = g_hitNs.exchange(0), writeBytes = g_writeBytes.exchange(0);
    const uint64_t transcodeBytes = g_transcodeBytes.exchange(0), transcodeNs = g_transcodeNs.exchange(0);
    if (hits == 0 && misses == 0 && transcodes == 0)
        return {};

    return fmt::format("transcoded {} textures ({:.1f} MB) in {:.0f} ms; cache {}: {} hits ({:.1f} MB read in {:.0f} ms), {} misses{}, {} stored ({:.1f} MB), {:.1f} MB in total",
        transcodes, double(transcodeBytes) / 1048576.0, double(transcodeNs) / 1e6, IsEnabled() ? "on" : "off",
        hits, double(hitBytes) / 1048576.0, double(hitNs) / 1e6, misses, damaged ? fmt::format(" ({} damaged)", damaged) : std::string(),
        writes, double(writeBytes) / 1048576.0, double(g_cacheBytes.load()) / 1048576.0);
}
