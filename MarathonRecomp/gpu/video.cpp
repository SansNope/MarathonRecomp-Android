#include "video.h"

#include "imgui/imgui_common.h"
#include "imgui/imgui_snapshot.h"
#include "imgui/imgui_font_builder.h"

#include <app.h>
#include <bc_diff.h>
#include <cpu/guest_thread.h>
#include <cstdint>
#include <cstdio>
#include <decompressor.h>
#include <kernel/function.h>
#include <kernel/heap.h>
#include <hid/hid.h>
#include <kernel/memory.h>
#include <kernel/xdbf.h>
#include <plume_render_interface.h>
#include <res/bc_diff/button_bc_diff.bin.h>
#include <res/font/im_font_atlas.dds.h>
#include <shader/shader_cache.h>
#include <Marathon.h>
#include <ui/achievement_menu.h>
#include <ui/achievement_overlay.h>
#include <ui/button_window.h>
#include <ui/fader.h>
#include <ui/imgui_utils.h>
#include <ui/installer_wizard.h>
#include <ui/message_window.h>
#include <ui/options_menu.h>
#include <ui/game_window.h>
#include <ui/black_bar.h>
#include <patches/aspect_ratio_patches.h>
#include <patches/hook_event.h>
#include <patches/loading_patches.h>
#include <user/config.h>
#include <sdl_listener.h>
#include <xxHashMap.h>
#include <os/process.h>
#include <os/thread.h>
#ifdef __ANDROID__
#define BCDEC_IMPLEMENTATION
#include <android/native_window.h>
#include <jni.h>
#include <bcdec.h>
#include <ProcessRGB.hpp>
#include <os/android/vulkan_driver_android.h>
#include <os/android/logger_android.h>
#include <os/android/storage_android.h>
#include <os/android/frame_diagnostics.h>
#include <os/android/sampling_profiler_android.h>
#include <os/android/guest_waits_android.h>
#include <os/android/texture_cache_android.h>
#include <os/android/overlay_android.h>
#include <os/android/performance_hint_android.h>
#include <os/android/thread_hints_android.h>
#include <ui/touch_controls.h>
#include <dlfcn.h>
#include <time.h>
#include <sched.h>
#endif

#if defined(ASYNC_PSO_DEBUG) || defined(PSO_CACHING)
#include <user/paths.h>
#include <magic_enum/magic_enum.hpp>
#elif defined(__ANDROID__)
#include <magic_enum/magic_enum.hpp>
#endif

#define MARATHON_RECOMP
#include "../../tools/XenosRecomp/XenosRecomp/shader_common.h"

#ifdef MARATHON_RECOMP_D3D12
#include "gpu/shader/hlsl/conditional_survey_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/copy_vs.hlsl.dxil.h"
#include "gpu/shader/hlsl/copy_color_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/copy_depth_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_vs.hlsl.dxil.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/movie_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/gamma_correction_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/imgui_ps.hlsl.dxil.h"
#include "gpu/shader/hlsl/imgui_vs.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_color_2x.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_color_4x.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_color_8x.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_2x.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_4x.hlsl.dxil.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_8x.hlsl.dxil.h"
#endif

#ifdef MARATHON_RECOMP_METAL
#include "gpu/shader/msl/conditional_survey_ps.metal.metallib.h"
#include "gpu/shader/msl/copy_vs.metal.metallib.h"
#include "gpu/shader/msl/copy_color_ps.metal.metallib.h"
#include "gpu/shader/msl/copy_depth_ps.metal.metallib.h"
#include "gpu/shader/msl/enhanced_burnout_blur_vs.metal.metallib.h"
#include "gpu/shader/msl/enhanced_burnout_blur_ps.metal.metallib.h"
#include "gpu/shader/msl/movie_ps.metal.metallib.h"
#include "gpu/shader/msl/gamma_correction_ps.metal.metallib.h"
#include "gpu/shader/msl/imgui_ps.metal.metallib.h"
#include "gpu/shader/msl/imgui_vs.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_color_2x.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_color_4x.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_color_8x.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_depth_2x.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_depth_4x.metal.metallib.h"
#include "gpu/shader/msl/resolve_msaa_depth_8x.metal.metallib.h"
#endif

#include "gpu/shader/hlsl/conditional_survey_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/copy_vs.hlsl.spirv.h"
#include "gpu/shader/hlsl/copy_color_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/copy_depth_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_vs.hlsl.spirv.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/movie_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/gamma_correction_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/imgui_ps.hlsl.spirv.h"
#include "gpu/shader/hlsl/imgui_vs.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_2x.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_4x.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_8x.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_2x.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_4x.hlsl.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_8x.hlsl.spirv.h"
#ifdef __ANDROID__
#include "gpu/shader/hlsl/conditional_survey_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/copy_vs.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/copy_color_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/copy_depth_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_vs.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/movie_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/imgui_ps.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/imgui_vs.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_2x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_4x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_color_8x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_2x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_4x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/resolve_msaa_depth_8x.hlsl.fixed.spirv.h"
#include "gpu/shader/hlsl/conditional_survey_ps.hlsl.buffer.spirv.h"
#include "gpu/shader/hlsl/enhanced_burnout_blur_vs.hlsl.buffer.spirv.h"
#endif

#ifdef _WIN32
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace plume
{
#ifdef MARATHON_RECOMP_D3D12
    extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
#endif
#ifdef MARATHON_RECOMP_METAL
extern std::unique_ptr<RenderInterface> CreateMetalInterface();
#endif
    extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
}

using namespace plume;

#pragma pack(push, 1)
struct PipelineState
{
    GuestShader* vertexShader = nullptr;
    GuestShader* pixelShader = nullptr;
    GuestVertexDeclaration* vertexDeclaration = nullptr;
    bool zEnable = true;
    bool zWriteEnable = true;
    bool stencilEnable = false;
    bool stencilTwoSided = false;
    RenderBlend srcBlend = RenderBlend::ONE;
    RenderBlend destBlend = RenderBlend::ZERO;
    RenderCullMode cullMode = RenderCullMode::NONE;
    RenderFrontFace frontFace = RenderFrontFace::CLOCKWISE;
    RenderComparisonFunction zFunc = RenderComparisonFunction::LESS;
    RenderComparisonFunction stencilFunc = RenderComparisonFunction::ALWAYS;
    RenderStencilOp stencilFail = RenderStencilOp::KEEP;
    RenderStencilOp stencilZFail = RenderStencilOp::KEEP;
    RenderStencilOp stencilPass = RenderStencilOp::KEEP;
    RenderComparisonFunction stencilFuncCCW = RenderComparisonFunction::ALWAYS;
    RenderStencilOp stencilFailCCW = RenderStencilOp::KEEP;
    RenderStencilOp stencilZFailCCW = RenderStencilOp::KEEP;
    RenderStencilOp stencilPassCCW = RenderStencilOp::KEEP;
    uint32_t stencilMask = 0xFFFFFFFF;
    uint32_t stencilWriteMask = 0xFFFFFFFF;
    uint32_t stencilRef = 0;
    bool alphaBlendEnable = false;
    RenderBlendOperation blendOp = RenderBlendOperation::ADD;
    float slopeScaledDepthBias = 0.0f;
    int32_t depthBias = 0;
    RenderBlend srcBlendAlpha = RenderBlend::ONE;
    RenderBlend destBlendAlpha = RenderBlend::ZERO;
    RenderBlendOperation blendOpAlpha = RenderBlendOperation::ADD;
    uint32_t colorWriteEnable = uint32_t(RenderColorWriteEnable::ALL);
    RenderPrimitiveTopology primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
    uint8_t vertexStrides[16]{};
    RenderFormat renderTargetFormat{};
    RenderFormat depthStencilFormat{};
    RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
    bool enableAlphaToCoverage = false;
    bool enableConditionalSurvey = false;
    uint32_t specConstants = 0;
};
#pragma pack(pop)

// Logs a built-in pipeline the driver rejected (Android diagnostics; no-op elsewhere).
static void CheckBuiltInPipeline(const RenderPipeline* pipeline, const char* name);

// Drops the vertex input renumbering of a destroyed shader (see g_compactVertexLocations).
static void ForgetVertexLocationRemap(const RenderShader* shader);

struct UploadAllocation
{
    const RenderBuffer* buffer;
    uint64_t offset;
    uint8_t* memory;
    uint64_t deviceAddress;
};

struct SharedConstants
{
    uint32_t texture2DIndices[16]{};
    uint32_t texture2DArrayIndices[16]{};
    uint32_t textureCubeIndices[16]{};
    uint32_t samplerIndices[16]{};
    uint32_t booleans{};
    uint32_t swappedTexcoords{};
    uint32_t swappedNormals{};
    uint32_t swappedBinormals{};
    uint32_t swappedTangents{};
    uint32_t swappedBlendWeights{};
    float halfPixelOffsetX{};
    float halfPixelOffsetY{};
    float clipPlane[4]{};
    bool clipPlaneEnabled{};
    float alphaThreshold{};
    uint32_t conditionalSurveyIndex{};
    uint32_t conditionalRenderingIndex{};
};

// Depth bias values here are only used when the render device has 
// dynamic depth bias capability enabled. Otherwise, they get unused
// and the values get assigned in the pipeline state instead.

static GuestSurface* g_renderTarget;
static GuestSurface* g_depthStencil;
static RenderFormat g_depthStencilFormat = RenderFormat::D32_FLOAT_S8_UINT;
static RenderFramebuffer* g_framebuffer;
static RenderViewport g_viewport(0.0f, 0.0f, 1280.0f, 720.0f);
static PipelineState g_pipelineState;
static int32_t g_depthBias;
static float g_slopeScaledDepthBias;
static uint32_t g_vertexShaderConstants[0x400];
static uint32_t g_pixelShaderConstants[0x380];
static SharedConstants g_sharedConstants;
static GuestTexture* g_textures[16];
static RenderSamplerDesc g_samplerDescs[16];
#ifdef __ANDROID__
static RenderFormat g_pointSamplerFormats[std::size(g_samplerDescs)]{};
static bool g_pointFilterR32 = false;
static bool g_pointFilterDepthStencil = false;
#endif
static bool g_scissorTestEnable = false;
static RenderRect g_scissorRect;
static RenderVertexBufferView g_vertexBufferViews[16];
static RenderInputSlot g_inputSlots[16];
static RenderIndexBufferView g_indexBufferView({}, 0, RenderFormat::R16_UINT);

struct DirtyStates
{
    bool renderTargetAndDepthStencil;
    bool viewport;
    bool pipelineState;
    bool depthBias;
    bool sharedConstants;
    bool scissorRect;
    bool vertexShaderConstants;
    bool vertexShaderConstantAddress;
    uint8_t vertexStreamFirst;
    uint8_t vertexStreamLast;
    bool indices;
    bool pixelShaderConstants;

    DirtyStates(bool value)
        : renderTargetAndDepthStencil(value)
        , viewport(value)
        , pipelineState(value)
        , depthBias(value)
        , sharedConstants(value)
        , scissorRect(value)
        , vertexShaderConstants(value)
        , vertexShaderConstantAddress(value)
        , vertexStreamFirst(value ? 0 : 255)
        , vertexStreamLast(value ? 15 : 0)
        , indices(value)
        , pixelShaderConstants(value)
    {
    }
};

static DirtyStates g_dirtyStates(true);

template<typename T>
static void SetDirtyValue(bool& dirtyState, T& dest, const T& src)
{
    if (dest != src)
    {
        dest = src;
        dirtyState = true;
    }
}

static constexpr size_t PROFILER_VALUE_COUNT = 256;
static size_t g_profilerValueIndex;

struct Profiler
{
    std::atomic<double> value;
    double values[PROFILER_VALUE_COUNT];
    std::chrono::steady_clock::time_point start;

    void Begin()
    {
        start = std::chrono::steady_clock::now();
    }

    void End()
    {
        value = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    void Set(double v)
    {
        value = v;
    }

    void Reset()
    {
        End();
        Begin();
    }

    double UpdateAndReturnAverage()
    {
        values[g_profilerValueIndex] = value;
        return std::accumulate(values, values + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
    }
};

static double g_applicationValues[PROFILER_VALUE_COUNT];
static Profiler g_gpuFrameProfiler;
static Profiler g_presentProfiler;
static Profiler g_frameFenceProfiler;
static Profiler g_presentWaitProfiler;
static Profiler g_swapChainAcquireProfiler;

enum GpuPassLabel : uint8_t
{
    GpuPassGame,
    GpuPassImGui,
    GpuPassGamma,
    GpuPassColorCopy,
    GpuPassDepthCopy
};

#ifdef __ANDROID__
struct FixedDescriptorFrameStats
{
    uint32_t lookups = 0;
    uint32_t binds = 0;
    uint32_t preparedDraws = 0;
};

struct DrawConstantFrameStats
{
    uint64_t uploadedBytes = 0;
    uint32_t repeatedUpdates = 0;
    uint32_t pairedSnapshots = 0;
    uint32_t pushWrites = 0;
};

struct ImGuiFrameStats
{
    uint32_t openedPasses = 0;
    uint32_t barrierPassBreaks = 0;
    uint32_t geometryBinds = 0;
    bool drew = false;
};

// Pipeline compilations, logged as [pso] with the frame diagnostics: the render thread compiles a
// missing pipeline while the frame waits (a hitch when it happens during gameplay); the compiler
// threads compile during loading and precompilation.
static thread_local bool g_isRenderThread = false;
static std::atomic<uint32_t> g_psoGameplayCount{ 0 }, g_psoOtherCount{ 0 };
static std::atomic<uint64_t> g_psoGameplayMicroseconds{ 0 }, g_psoOtherMicroseconds{ 0 }, g_psoGameplayMaxMicroseconds{ 0 };
static std::atomic<uint32_t> g_pipelinesSinceCacheSave{ 0 };
// GPU time per physical render pass, from a timestamp Plume writes before each pass begins (with
// perf_capture.enabled). Passes are grouped by their framebuffer (size, color attachments, depth);
// logged as [gpu-passes] about every 10 seconds with the frame's draw, pass and barrier counts.
struct GpuPassDiagnostics
{
    struct Target
    {
        uint64_t key = 0;
        double ms = 0.0;
        uint32_t passes = 0;
    };

    std::mutex mutex;
    std::vector<Target> targets;
    uint32_t frames = 0;
    double totalMs = 0.0, beforeFirstPassMs = 0.0;
    double depthCopyMs = 0.0;
    double totalRenderCpuMs = 0.0, maxRenderCpuMs = 0.0;
    uint32_t renderCpuFrames = 0;
    uint64_t depthCopyPasses = 0, depthCopyDiscardPasses = 0, depthTransferCopies = 0, colorTransferCopies = 0;
    uint64_t clearsMergedAfterDepthCopy = 0;
    uint64_t directCsmSliceSkips = 0;
    uint64_t draws = 0, passes = 0, barrierCalls = 0, barrierPassBreaks = 0, splitPasses = 0, clearLoadPasses = 0, truncatedFrames = 0;
    uint64_t imGuiFrames = 0, imGuiOpenedPasses = 0, imGuiBarrierPassBreaks = 0, imGuiGeometryBinds = 0;
    uint64_t constantBytes = 0, repeatedConstantUpdates = 0, constantPushWrites = 0, pairedSnapshots = 0;
    uint64_t fixedDescriptorLookups = 0, fixedDescriptorBinds = 0, fixedDescriptorDraws = 0;
    // Every third summary (~30 s) the next frame's passes are listed one by one.
    uint32_t summaries = 0;
    bool frameListRequested = false;
    std::string frameList;

    static const char* Label(uint8_t label)
    {
        switch (label)
        {
        case GpuPassImGui: return "imgui";
        case GpuPassGamma: return "gamma";
        case GpuPassColorCopy: return "copy-color";
        case GpuPassDepthCopy: return "copy-depth";
        default: return "game";
        }
    }

    static char EndReason(RenderPassEndReason reason)
    {
        switch (reason)
        {
        case RenderPassEndReason::Barrier: return 'b';
        case RenderPassEndReason::Framebuffer: return 'f';
        case RenderPassEndReason::End: return 'e';
        case RenderPassEndReason::Open: return '?';
        default: return 'o';
        }
    }

    static uint64_t Key(const RenderPassTimestampInfo& info)
    {
        return (uint64_t(info.width) << 32) | (uint64_t(info.height) << 8) | (uint64_t(info.colorAttachmentCount) << 1) | (info.depthAttachment ? 1 : 0);
    }

    // The pass timestamps are in nanoseconds. The separate frame query supplies the last boundary.
    void AddFrame(const std::vector<RenderPassTimestampInfo>& infos, const uint64_t* timestamps, uint64_t frameStart, uint64_t frameEnd,
        const RenderCommandListStats& stats, const ImGuiFrameStats& imGui,
        const DrawConstantFrameStats& constants, const FixedDescriptorFrameStats& fixedDescriptors,
        double renderCpuMs, uint32_t frameDirectSliceSkips)
    {
        const bool truncated = stats.renderPasses > infos.size();
        const auto getPassMilliseconds = [&](size_t index)
        {
            if (truncated && index + 1 == infos.size())
                return 0.0; // The end of this pass was outside the recorded range.

            const uint64_t end = index + 1 < infos.size() ? timestamps[index + 1] : frameEnd;
            return end >= timestamps[index] ? double(end - timestamps[index]) / 1e6 : 0.0;
        };

        std::lock_guard lock(mutex);
        ++frames;
        if (renderCpuMs >= 0.0)
        {
            totalRenderCpuMs += renderCpuMs;
            maxRenderCpuMs = std::max(maxRenderCpuMs, renderCpuMs);
            ++renderCpuFrames;
        }
        if (imGui.drew)
        {
            ++imGuiFrames;
            imGuiOpenedPasses += imGui.openedPasses;
            imGuiBarrierPassBreaks += imGui.barrierPassBreaks;
            imGuiGeometryBinds += imGui.geometryBinds;
        }
        totalMs += double(frameEnd - frameStart) / 1e6;
        if (!infos.empty() && timestamps[0] >= frameStart)
            beforeFirstPassMs += double(timestamps[0] - frameStart) / 1e6;

        for (size_t i = 0; i < infos.size(); i++)
        {
            const double ms = getPassMilliseconds(i);
            const uint64_t key = Key(infos[i]);
            auto target = std::find_if(targets.begin(), targets.end(), [&](const Target& t) { return t.key == key; });
            if (target == targets.end())
                target = targets.insert(targets.end(), Target{ key });
            target->ms += ms;
            target->passes++;
            if (infos[i].label == GpuPassDepthCopy)
            {
                depthCopyMs += ms;
                ++depthCopyPasses;
                depthCopyDiscardPasses += infos[i].discardedDepthAtStart;
            }
        }

        draws += stats.draws;
        passes += stats.renderPasses;
        barrierCalls += stats.barrierCalls;
        barrierPassBreaks += stats.barrierPassBreaks;
        splitPasses += stats.splitPasses;
        clearLoadPasses += stats.clearLoadPasses;
        depthTransferCopies += stats.depthTransferCopies;
        colorTransferCopies += stats.colorTransferCopies;
        clearsMergedAfterDepthCopy += stats.clearsMergedAfterDepthCopy;
        directCsmSliceSkips += frameDirectSliceSkips;
        truncatedFrames += truncated ? 1 : 0;
        constantBytes += constants.uploadedBytes;
        pairedSnapshots += constants.pairedSnapshots;
        repeatedConstantUpdates += constants.repeatedUpdates;
        constantPushWrites += constants.pushWrites;
        fixedDescriptorLookups += fixedDescriptors.lookups;
        fixedDescriptorBinds += fixedDescriptors.binds;
        fixedDescriptorDraws += fixedDescriptors.preparedDraws;

        if (frameListRequested && !infos.empty())
        {
            frameListRequested = false;
            frameList = fmt::format("{} passes ({} recorded), {:.2f} ms GPU:", stats.renderPasses, infos.size(), double(frameEnd - frameStart) / 1e6);
            for (size_t i = 0; i < infos.size(); i++)
            {
                const auto& info = infos[i];
                const double ms = getPassMilliseconds(i);
                frameList += fmt::format(" #{} {}x{} c{}{} {} d{}{}{}{}{} end={} {:.2f};", i, info.width, info.height, info.colorAttachmentCount,
                    info.depthAttachment ? "+d" : "", Label(info.label), info.draws, info.clearedAtStart ? " clr" : "", info.split ? " split" : "",
                    info.discardedColorAtStart ? " discard-color" : "", info.discardedDepthAtStart ? " discard-depth" : "", EndReason(info.endReason), ms);
            }
        }
    }

    // The pass list of a frame requested by TakeSummary; empty when none is ready.
    std::string TakeFrameList()
    {
        std::lock_guard lock(mutex);
        return std::exchange(frameList, std::string());
    }

    std::string TakeSummary(size_t count)
    {
        std::lock_guard lock(mutex);
        if (frames == 0)
            return {};

        std::sort(targets.begin(), targets.end(), [](const Target& a, const Target& b) { return a.ms > b.ms; });
        const double n = double(frames);
        if (++summaries % 3 == 1)
            frameListRequested = true;

        std::string text = fmt::format("{} frames: per frame {:.2f} ms GPU, {:.1f} passes ({:.1f} split by a barrier on the same target, {:.1f} cleared on load), {:.0f} draws, {:.1f} barrier calls ({:.1f} ending a pass); before the first pass {:.2f} ms{}; by target:",
            frames, totalMs / n, double(passes) / n, double(splitPasses) / n, double(clearLoadPasses) / n, double(draws) / n, double(barrierCalls) / n, double(barrierPassBreaks) / n, beforeFirstPassMs / n,
            truncatedFrames ? fmt::format(", {} frames had more passes than recorded", truncatedFrames) : std::string());
        for (size_t i = 0; i < targets.size() && i < count; i++)
        {
            const Target& target = targets[i];
            text += fmt::format(" {}{}x{} c{}{} {:.2f} ms ({:.1f} passes)", i ? ", " : "", uint32_t(target.key >> 32), uint32_t((target.key >> 8) & 0xFFFFFF),
                uint32_t((target.key >> 1) & 0x7F), (target.key & 1) ? "+d" : "", target.ms / n, double(target.passes) / n);
        }

        if (renderCpuFrames != 0)
            text += fmt::format(". Render thread CPU {:.2f} ms/frame (max {:.2f}, {} frames)",
                totalRenderCpuMs / double(renderCpuFrames), maxRenderCpuMs, renderCpuFrames);

        if (depthCopyPasses != 0)
        {
            text += fmt::format(". copy-depth {:.2f} ms/frame ({:.1f} passes/frame, {:.1f} discard-depth/frame)",
                depthCopyMs / n, double(depthCopyPasses) / n, double(depthCopyDiscardPasses) / n);
        }
        if (depthTransferCopies != 0)
            text += fmt::format(". Depth transfers {:.1f}/frame", double(depthTransferCopies) / n);
        if (directCsmSliceSkips != 0)
            text += fmt::format(". Direct CSM slice redirects {:.1f}/frame", double(directCsmSliceSkips) / n);
        if (clearsMergedAfterDepthCopy != 0)
            text += fmt::format(". Clears merged after depth copies {:.2f}/frame", double(clearsMergedAfterDepthCopy) / n);
        if (colorTransferCopies != 0)
            text += fmt::format(". Color transfers {:.1f}/frame", double(colorTransferCopies) / n);

        if (imGuiFrames != 0)
        {
            const double imGuiCount = double(imGuiFrames);
            text += fmt::format(". ImGui over {} frames: {:.2f} new passes, {:.2f} barrier breaks, {:.2f} geometry binds/frame",
                imGuiFrames, double(imGuiOpenedPasses) / imGuiCount, double(imGuiBarrierPassBreaks) / imGuiCount,
                double(imGuiGeometryBinds) / imGuiCount);
        }

        if (constantBytes != 0)
        {
            text += fmt::format(". Draw constants per frame: {:.1f} KiB uploaded, {:.1f} repeated updates skipped, {:.1f} push writes",
                double(constantBytes) / (1024.0 * n), double(repeatedConstantUpdates) / n, double(constantPushWrites) / n);
        }

        if (pairedSnapshots != 0)
            text += fmt::format(". Paired draw constant snapshots {:.1f}/frame", double(pairedSnapshots) / n);

        if (fixedDescriptorDraws != 0)
        {
            const double drawCount = double(fixedDescriptorDraws);
            text += fmt::format(". Fixed draw descriptors: {:.2f} lookups/draw, {:.2f} binds/draw",
                double(fixedDescriptorLookups) / drawCount, double(fixedDescriptorBinds) / drawCount);
        }

        targets.clear();
        frames = 0;
        fixedDescriptorLookups = fixedDescriptorBinds = fixedDescriptorDraws = 0;
        constantBytes = repeatedConstantUpdates = constantPushWrites = pairedSnapshots = 0;
        imGuiFrames = imGuiOpenedPasses = imGuiBarrierPassBreaks = imGuiGeometryBinds = 0;
        totalMs = beforeFirstPassMs = depthCopyMs = 0.0;
        totalRenderCpuMs = maxRenderCpuMs = 0.0;
        renderCpuFrames = 0;
        depthCopyPasses = depthCopyDiscardPasses = depthTransferCopies = colorTransferCopies = clearsMergedAfterDepthCopy = directCsmSliceSkips = 0;
        draws = passes = barrierCalls = barrierPassBreaks = splitPasses = clearLoadPasses = truncatedFrames = 0;
        return text;
    }
};

static GpuPassDiagnostics g_gpuPassDiagnostics;

// Diagnostics. No settings, GPU work, affinity or pacing are changed.
struct TextureUploadDiagnostics
{
    std::mutex mutex;
    uint64_t textures = 0;
    uint64_t reusedBuffers = 0;
    uint64_t regions = 0;
    uint64_t copyCalls = 0;
    uint64_t submissions = 0;

    void Add(bool reused, size_t regionCount, bool batched)
    {
        std::lock_guard lock(mutex);
        ++textures;
        reusedBuffers += reused;
        regions += regionCount;
        copyCalls += batched ? 1 : regionCount;
    }

    void AddSubmission()
    {
        std::lock_guard lock(mutex);
        ++submissions;
    }

    std::string TakeSummary()
    {
        std::lock_guard lock(mutex);
        if (textures == 0)
            return {};

        auto text = fmt::format("{} DDS textures, {} staging buffer reuses, {} regions in {} copy commands ({} queue submissions)",
            textures, reusedBuffers, regions, copyCalls, submissions);
        textures = reusedBuffers = regions = copyCalls = submissions = 0;
        return text;
    }
};

static TextureUploadDiagnostics g_textureUploadDiagnostics;

// <game root>/perf_capture.enabled (a launcher switch, off by default) writes a summary every two
// seconds to log.txt.
struct AndroidFrameDiagnostics
{
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastEnd{}, windowStart{}, entry{};
    double lastCpu = 0.0, entryCpu = 0.0;
    uint32_t count = 0;
    double frame = 0.0, cpu = 0.0, guest = 0.0, guestCpu = 0.0;
    double renderWait = 0.0, present = 0.0, fence = 0.0, acquire = 0.0, pacing = 0.0;
    double presentWait = 0.0, updateWall = 0.0, updateCpu = 0.0;
    double pendingUpdateWall = 0.0, pendingUpdateCpu = 0.0, updateStartCpu = 0.0;
    // Game time (App::s_deltaTime per document update) against wall time: below 1 the game runs slow.
    double gameTime = 0.0, gameDeltaMin = 0.0, gameDeltaMax = 0.0;
    uint32_t gameUpdates = 0;
    Clock::time_point updateStart{};
    double gpu = 0.0, gpuMax = 0.0;
    // Frames and [perf] windows since the last [waits] line (every 5 windows, ~10 s).
    uint32_t waitFrames = 0, waitWindows = 0;
    bool enabled = false;

    static double CpuMilliseconds()
    {
        timespec ts{};
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
        return double(ts.tv_sec) * 1000.0 + double(ts.tv_nsec) / 1000000.0;
    }

    static double Milliseconds(Clock::time_point begin, Clock::time_point end)
    {
        return std::chrono::duration<double, std::milli>(end - begin).count();
    }

    void Begin()
    {
        enabled = os::android::perf::IsCaptureRequested();
        if (!enabled)
            return;
        entry = Clock::now();
        entryCpu = CpuMilliseconds();
    }

    void End(double waitRenderMs, double presentMs, double fenceMs, double pacingMs)
    {
        if (!enabled)
            return;
        const auto end = Clock::now();
        const double endCpu = CpuMilliseconds();
        if (lastEnd.time_since_epoch().count() != 0)
        {
            ++count;
            frame += Milliseconds(lastEnd, end);
            cpu += endCpu - lastCpu;
            guest += Milliseconds(lastEnd, entry);
            guestCpu += entryCpu - lastCpu;
            presentWait += g_presentWaitProfiler.value.load();
            updateWall += pendingUpdateWall;
            updateCpu += pendingUpdateCpu;
            renderWait += waitRenderMs;
            present += presentMs;
            fence += fenceMs;
            pacing += pacingMs;
            acquire += g_swapChainAcquireProfiler.value.load();
            const double gpuMs = g_gpuFrameProfiler.value.load();
            gpu += gpuMs;
            gpuMax = std::max(gpuMax, gpuMs);
        }
        else
        {
            windowStart = end;
            LOGN("[perf] enabled; GPU uses existing frame timestamps; other values are CPU wall/thread times");
        }
        lastEnd = end;
        lastCpu = endCpu;
        pendingUpdateWall = pendingUpdateCpu = 0.0;
        if (count && end - windowStart >= std::chrono::seconds(2))
        {
            const double n = double(count);
            LOGFN("[perf] frames={} fps={:.2f} frame_ms={:.3f} main_cpu_ms={:.3f} guest_wall_ms={:.3f} guest_cpu_ms={:.3f} render_wait_ms={:.3f} present_ms={:.3f} fence_ms={:.3f} acquire_ms={:.3f} pacing_ms={:.3f} gpu_ms={:.3f} gpu_max_ms={:.3f} cpu={} render_cpu={} mhz={} scale={:.3f} present_wait_ms={:.3f} doc_update_wall_ms={:.3f} doc_update_cpu_ms={:.3f} fps_cap={} vframe={} game_speed={:.3f} game_dt_ms={:.2f}..{:.2f} adpf={} uclamp={} render_uclamp={} waits_ms={}",
                count, 1000.0 * n / frame, frame / n, cpu / n, guest / n, guestCpu / n,
                renderWait / n, present / n, fence / n, acquire / n, pacing / n, gpu / n,
                gpuMax, sched_getcpu(), os::android::perf::ThreadCore(os::android::hint::GetThread(os::android::hint::Role::Render)),
                os::android::perf::ClusterClocks(), float(Config::ResolutionScale), presentWait / n, updateWall / n, updateCpu / n,
                int32_t(Config::FPS), Config::FPS != 60 ? 1 : 0, 1000.0 * gameTime / frame, 1000.0 * gameDeltaMin, 1000.0 * gameDeltaMax, os::android::hint::GetStatus(),
                os::android::thread_hints::ThreadUclamp(os::android::hint::GetThread(os::android::hint::Role::Guest)),
                os::android::thread_hints::ThreadUclamp(os::android::hint::GetThread(os::android::hint::Role::Render)),
                os::android::waits::TakeFrameSummary(count));
            waitFrames += count;
            if (++waitWindows >= 5)
            {
                const std::string callers = os::android::waits::TakeTopCallers(waitFrames, 8);
                if (!callers.empty())
                    LOGFN("[waits] guest main thread, {} frames: {}", waitFrames, callers);
                const std::string wakers = os::android::waits::TakeWakers(waitFrames, 6);
                if (!wakers.empty())
                    LOGFN("[wakers] guest main thread, {} frames, by signalling thread: {}", waitFrames, wakers);
                const std::string timeouts = os::android::waits::TakeTimeouts();
                if (!timeouts.empty())
                    LOGFN("[waits] finite timeouts on all threads: {}", timeouts);
                const std::string gpuPasses = g_gpuPassDiagnostics.TakeSummary(8);
                if (!gpuPasses.empty())
                    LOGFN("[gpu-passes] {}", gpuPasses);
                const std::string gpuFrame = g_gpuPassDiagnostics.TakeFrameList();
                if (!gpuFrame.empty())
                    LOGFN("[gpu-frame] {}", gpuFrame);
                const std::string textureCache = os::android::texture_cache::TakeStats();
                if (!textureCache.empty())
                    LOGFN("[texture-cache] {}", textureCache);
                const std::string textureUploads = g_textureUploadDiagnostics.TakeSummary();
                if (!textureUploads.empty())
                    LOGFN("[texture-upload] {}", textureUploads);
                waitFrames = waitWindows = 0;
            }
            gameTime = gameDeltaMin = gameDeltaMax = 0.0;
            gameUpdates = 0;

            const uint32_t psoGameplay = g_psoGameplayCount.exchange(0);
            const uint32_t psoOther = g_psoOtherCount.exchange(0);
            const uint64_t psoGameplayUs = g_psoGameplayMicroseconds.exchange(0);
            const uint64_t psoOtherUs = g_psoOtherMicroseconds.exchange(0);
            const uint64_t psoGameplayMaxUs = g_psoGameplayMaxMicroseconds.exchange(0);
            if (psoGameplay != 0 || psoOther != 0)
            {
                LOGFN("[pso] render thread during gameplay: {} pipelines, {:.1f} ms (max {:.1f} ms); loading/precompilation: {} pipelines, {:.1f} ms",
                    psoGameplay, psoGameplayUs / 1000.0, psoGameplayMaxUs / 1000.0, psoOther, psoOtherUs / 1000.0);
            }
            count = 0;
            frame = cpu = guest = guestCpu = renderWait = present = fence = acquire = pacing = gpu = gpuMax = 0.0;
            presentWait = updateWall = updateCpu = 0.0;
            windowStart = end;
        }
    }
};

static thread_local AndroidFrameDiagnostics g_androidFrameDiagnostics;

// One [fps] line a minute in every log, with or without perf_capture.enabled, so a tester's log
// always shows the frame rate. Frames longer than a second (the app in the background, a long
// load) are counted but left out of the averages. Only the presenting thread touches it.
struct AndroidFrameRateSummary
{
    using Clock = std::chrono::steady_clock;
    Clock::time_point last{}, windowStart{};
    uint32_t frames = 0, slowFrames = 0, longFrames = 0;
    double frameSum = 0.0, frameMax = 0.0, gpuSum = 0.0, gpuMax = 0.0;

    void Frame()
    {
        const auto now = Clock::now();
        if (last.time_since_epoch().count() == 0)
        {
            last = windowStart = now;
            return;
        }

        const double ms = std::chrono::duration<double, std::milli>(now - last).count();
        last = now;
        if (ms > 1000.0)
        {
            ++longFrames;
        }
        else
        {
            ++frames;
            frameSum += ms;
            frameMax = std::max(frameMax, ms);
            slowFrames += ms > 100.0 ? 1 : 0;

            const double gpuMs = g_gpuFrameProfiler.value.load();
            gpuSum += gpuMs;
            gpuMax = std::max(gpuMax, gpuMs);
        }

        if (now - windowStart < std::chrono::seconds(60))
            return;

        if (frames != 0)
        {
            const double n = double(frames);
            const int32_t cap = Config::FPS;
            LOGFN("[fps] {:.0f} s: {} frames, {:.1f} FPS, frame {:.2f} ms (worst {:.1f}), {} over 100 ms, {} over 1 s; GPU {:.2f} ms (max {:.1f}); scale {:.2f}, cap {}.",
                std::chrono::duration<double>(now - windowStart).count(), frames, 1000.0 * n / frameSum, frameSum / n, frameMax,
                slowFrames, longFrames, gpuSum / n, gpuMax, float(Config::ResolutionScale),
                cap >= FPS_MAX ? std::string("off") : std::to_string(cap));
        }

        frames = slowFrames = longFrames = 0;
        frameSum = frameMax = gpuSum = gpuMax = 0.0;
        windowStart = now;
    }
};

static AndroidFrameRateSummary g_androidFrameRateSummary;

void os::android::perf::BeginDocUpdate()
{
    auto& diagnostics = g_androidFrameDiagnostics;
    // The first call can precede the first Present; skipping that sample is fine.
    if (!diagnostics.enabled)
        return;
    diagnostics.updateStart = AndroidFrameDiagnostics::Clock::now();
    diagnostics.updateStartCpu = AndroidFrameDiagnostics::CpuMilliseconds();
}

void os::android::perf::EndDocUpdate()
{
    auto& diagnostics = g_androidFrameDiagnostics;
    if (!diagnostics.enabled || diagnostics.updateStart.time_since_epoch().count() == 0)
        return;
    diagnostics.pendingUpdateWall += AndroidFrameDiagnostics::Milliseconds(diagnostics.updateStart, AndroidFrameDiagnostics::Clock::now());
    diagnostics.pendingUpdateCpu += AndroidFrameDiagnostics::CpuMilliseconds() - diagnostics.updateStartCpu;
    const double delta = App::s_deltaTime;
    diagnostics.gameTime += delta;
    diagnostics.gameDeltaMin = diagnostics.gameUpdates ? std::min(diagnostics.gameDeltaMin, delta) : delta;
    diagnostics.gameDeltaMax = diagnostics.gameUpdates ? std::max(diagnostics.gameDeltaMax, delta) : delta;
    diagnostics.gameUpdates++;
    diagnostics.updateStart = {};
}
#endif

static bool g_profilerVisible;
static bool g_profilerWasToggled;

#if !defined(MARATHON_RECOMP_D3D12) && !defined(MARATHON_RECOMP_METAL)
static constexpr Backend g_backend = Backend::VULKAN;
#else
static Backend g_backend;
#endif

static bool g_triangleStripWorkaround = false;

static std::unique_ptr<RenderInterface> g_interface;
static std::unique_ptr<RenderDevice> g_device;

static RenderDeviceCapabilities g_capabilities;

static constexpr size_t NUM_FRAMES = 2;
static constexpr size_t NUM_QUERIES = 2;

static uint32_t g_frame = 0;
static uint32_t g_nextFrame = 1;
static uint32_t g_renderFrame = 0;

static std::unique_ptr<RenderCommandQueue> g_queue;
static std::unique_ptr<RenderCommandList> g_commandLists[NUM_FRAMES];
static std::unique_ptr<RenderCommandFence> g_commandFences[NUM_FRAMES];
static std::unique_ptr<RenderQueryPool> g_queryPools[NUM_FRAMES];
#ifdef __ANDROID__
// Per-pass GPU timestamps (GpuPassDiagnostics), read from the completed frame's slot.
static constexpr uint32_t NUM_PASS_QUERIES = 160;
static std::unique_ptr<RenderQueryPool> g_passQueryPools[NUM_FRAMES];
static ImGuiFrameStats g_imGuiFrameStats[NUM_FRAMES];
static DrawConstantFrameStats g_drawConstantFrameStats[NUM_FRAMES];
static FixedDescriptorFrameStats g_fixedDescriptorFrameStats[NUM_FRAMES];
static uint32_t g_directCsmSliceSkips[NUM_FRAMES]{};
struct RenderThreadFrameTiming
{
    double beginCpuMs = -1.0;
    double renderCpuMs = -1.0;
};
static RenderThreadFrameTiming g_renderThreadFrameTimings[NUM_FRAMES];
#endif
static bool g_commandListStates[NUM_FRAMES];

static std::unique_ptr<RenderCommandList> g_waitCommandList;
static std::unique_ptr<RenderCommandFence> g_waitCommandFence;

static Mutex g_copyMutex;
static std::unique_ptr<RenderCommandQueue> g_copyQueue;
static std::unique_ptr<RenderCommandList> g_copyCommandList;
static std::unique_ptr<RenderCommandFence> g_copyCommandFence;

static Mutex g_discardMutex;
static std::unique_ptr<RenderCommandList> g_discardCommandList;
static std::unique_ptr<RenderCommandFence> g_discardCommandFence;

static std::unique_ptr<RenderSwapChain> g_swapChain;
static bool g_swapChainValid;

#ifdef __ANDROID__
// Android WSI surfaces only report RGBA-order formats (R8G8B8A8_UNORM/_SRGB), not BGRA.
// With B8G8R8A8_UNORM Plume finds no compatible surface format and the swap chain never
// gets a valid format (UnleashedRecomp-Android uses the same override).
static constexpr RenderFormat BACKBUFFER_FORMAT = RenderFormat::R8G8B8A8_UNORM;

static uint64_t g_androidSwapchainRetryAfterMs;
// Set when recreating the swap chain failed: the surface itself is then likely lost
// (VK_ERROR_SURFACE_LOST_KHR / NATIVE_WINDOW_IN_USE) even if the window pointer is unchanged.
static bool g_androidSurfaceNeedsRecreate;
#else
static constexpr RenderFormat BACKBUFFER_FORMAT = RenderFormat::B8G8R8A8_UNORM;
#endif

static std::unique_ptr<RenderCommandSemaphore> g_acquireSemaphores[NUM_FRAMES];
static std::vector<std::unique_ptr<RenderCommandSemaphore>> g_renderSemaphores;
static uint32_t g_backBufferIndex;
static std::unique_ptr<GuestSurface> g_backBufferHolder;
static GuestSurface* g_backBuffer;

// ResolutionScale only shrinks the game's scene render targets (framebuffer*, depth
// stencils); its back buffer and 2D/HUD layout stay at the full viewport size, and the
// game's own "Rasterize" pass stretches the scene into the back buffer. The scaled
// targets are created by Video::Resize, which needs the game's render target container,
// so a resize requested before it exists stays pending until it can be applied.
static bool g_resizePending;

static std::unique_ptr<RenderTexture> g_intermediaryBackBufferTexture;
static uint32_t g_intermediaryBackBufferTextureWidth;
static uint32_t g_intermediaryBackBufferTextureHeight;

#ifdef __ANDROID__
struct GammaPassConstants
{
    float gamma;
    uint32_t transpose;
    int32_t sourceOffset[2];
    int32_t sourceStep[2];
};

static_assert(sizeof(GammaPassConstants) == 24);
static_assert(offsetof(GammaPassConstants, sourceOffset) == 8);
static_assert(offsetof(GammaPassConstants, sourceStep) == 16);

static std::unique_ptr<RenderDescriptorSet> g_gammaSourceDescriptorSet;
static std::unique_ptr<RenderPipelineLayout> g_gammaPipelineLayout;
#else
static uint32_t g_intermediaryBackBufferTextureDescriptorIndex;
#endif
static std::unique_ptr<RenderPipeline> g_gammaCorrectionPipeline;

static std::unique_ptr<RenderDescriptorSet> g_textureDescriptorSet;
static std::unique_ptr<RenderDescriptorSet> g_samplerDescriptorSet;
static constexpr uint32_t FIXED_DESCRIPTOR_SLOTS = 16;
static bool g_useFixedDescriptors = false;
static bool g_reuseFixedDrawDescriptors = false;
static bool g_compactFixedDescriptors = false;
static bool g_useSoftwareClip = false;
static bool g_useBufferConstants = false;
static bool g_optimizeDrawConstants = false;
static bool g_batchDrawState = false;
static bool g_batchTextureUploads = false;
static bool g_batchMultiTextureUploads = false;
#ifdef __ANDROID__
static bool g_profileTextureUploads = false;
static bool g_capLoadingFps = true;
#endif
static uint64_t g_drawConstantAddresses[3];
static constexpr unsigned BUFFER_CONSTANTS_SHADER_VARIANT = 3;
static constexpr uint32_t SHADER_CONSTANT_ALIGNMENT = 0x100;
static uint32_t g_drawConstantOffsets[3];
static uint32_t g_drawConstantPageIndex;
static const RenderBuffer* g_drawConstantBuffer;
static bool g_stageGuestBuffers = false;
static bool g_discardDepthCopyLoad = false;
static bool g_useDepthTransfer = false;
static bool g_useColorTransfer = false;
static bool g_directCsmSlice = true;
static bool g_batchImGui = false;

struct DepthCopyDestination
{
    uint32_t textureAddress;
    uint32_t layer;
};

#ifdef __ANDROID__
static constexpr uint32_t MAX_DEPTH_COPY_TRACE_ENTRIES = 64;
static bool g_depthCopyTraceEnabled = false;
static bool g_mergeDepthCopyClears = false;
static uint32_t g_depthCopyTraceRemaining = 0;
static uint64_t g_depthCopyTraceFrame = 0;

struct DepthCopyTrace
{
    DepthCopyDestination plannedDestination;
    uint32_t draws = 0;
    uint32_t stencilDraws = 0;
    uint32_t depthClears = 0;
    uint32_t stencilClears = 0;
    bool depthClearedBeforeDraw = false;
    bool destinationBoundDuringDraw = false;
    bool colorAttachmentDuringDraw = false;

    void recordClear(bool depth, bool stencil)
    {
        depthClears += depth;
        stencilClears += stencil;
        depthClearedBeforeDraw |= depth && draws == 0;
    }
};

static DepthCopyTrace g_depthCopyTrace;
#endif
static bool g_hasConditionalSurvey = true;
static bool g_skipSurveyDraws = false;

static constexpr uint32_t CONDITIONAL_SURVEY_MAX = 64;
static std::unique_ptr<RenderBuffer> g_conditionalSurveyBuffer;
static std::unique_ptr<RenderDescriptorSet> g_conditionalSurveyDescriptorSet;

enum
{
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D,
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY,
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE,
    TEXTURE_DESCRIPTOR_NULL_COUNT
};

// Guest resource slots used by the fixed Android shaders. Shader rewriting and descriptor
// writes both enumerate these masks in ascending order.
static constexpr uint32_t COMPACT_FIXED_TEXTURE_SLOTS[TEXTURE_DESCRIPTOR_NULL_COUNT] = {
    (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 9) | (1u << 10) | (1u << 13),
    1u << 11,
    1u << 8,
};
static constexpr uint32_t COMPACT_FIXED_SAMPLER_SLOTS =
    (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 8) |
    (1u << 9) | (1u << 10) | (1u << 11) | (1u << 13);
static constexpr uint32_t ALL_FIXED_DESCRIPTOR_SLOTS = (1u << FIXED_DESCRIPTOR_SLOTS) - 1;

static uint32_t FixedTextureSlots(uint32_t viewType)
{
    return g_compactFixedDescriptors ? COMPACT_FIXED_TEXTURE_SLOTS[viewType] : ALL_FIXED_DESCRIPTOR_SLOTS;
}

struct TextureDescriptorAllocator
{
    Mutex mutex;
    uint32_t capacity = TEXTURE_DESCRIPTOR_NULL_COUNT;
    std::vector<uint32_t> freed;

    uint32_t allocate()
    {
        std::lock_guard lock(mutex);

        uint32_t value;
        if (!freed.empty())
        {
            value = freed.back();
            freed.pop_back();
        }
        else
        {
            value = capacity;
            ++capacity;
        }

        return value;
    }

    void free(uint32_t value)
    {
        assert(value != NULL);
        std::lock_guard lock(mutex);
        freed.push_back(value);
    }
};

static std::unique_ptr<RenderTexture> g_blankTextures[TEXTURE_DESCRIPTOR_NULL_COUNT];
static std::unique_ptr<RenderTextureView> g_blankTextureViews[TEXTURE_DESCRIPTOR_NULL_COUNT];

static TextureDescriptorAllocator g_textureDescriptorAllocator;

struct FixedTextureDescriptor
{
    const RenderTexture* texture = nullptr;
    const RenderTextureView* view = nullptr;
    RenderTextureLayout layout{};
    uint64_t revision = 0;
};

static std::mutex g_fixedTextureMutex;
static std::vector<FixedTextureDescriptor> g_fixedTextureDescriptors;
static std::atomic<uint64_t> g_fixedTextureRevision{ 0 };

static void SetTextureDescriptor(uint32_t index, const RenderTexture* texture, RenderTextureLayout layout,
    const RenderTextureView* view = nullptr)
{
    if (!g_useFixedDescriptors)
    {
        g_textureDescriptorSet->setTexture(index, texture, layout, view);
        return;
    }

    std::lock_guard lock(g_fixedTextureMutex);
    if (index >= g_fixedTextureDescriptors.size())
        g_fixedTextureDescriptors.resize(index + 1);

    const uint64_t revision = g_fixedTextureRevision.load(std::memory_order_relaxed) + 1;
    g_fixedTextureDescriptors[index] = { texture, view, layout, revision };
    g_fixedTextureRevision.store(revision, std::memory_order_release);
}

// Marathon's shaders read Xenos "3D" texture fetches through the Texture2DArray heap (set 1), and
// the game binds ordinary 2D textures there. Reading a 2D view through an arrayed image type is
// invalid in Vulkan: Turnip tolerates it, Mali returns nothing (untextured world). Every 2D texture
// therefore gets a second view typed as a 2D array, with its own descriptor for that heap.
static void CreateArrayTextureView(GuestTexture& texture, RenderTextureViewDesc viewDesc)
{
#ifdef __ANDROID__
    if (texture.texture == nullptr || texture.viewDimension != RenderTextureViewDimension::TEXTURE_2D)
        return;

    viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
    texture.arrayTextureView = texture.texture->createTextureView(viewDesc);
    texture.arrayDescriptorIndex = g_textureDescriptorAllocator.allocate();
    SetTextureDescriptor(texture.arrayDescriptorIndex, texture.texture, RenderTextureLayout::SHADER_READ, texture.arrayTextureView.get());
#else
    (void)texture;
    (void)viewDesc;
#endif
}

static void FreeArrayTextureView(GuestTexture& texture)
{
    if (texture.arrayDescriptorIndex != NULL)
    {
        SetTextureDescriptor(texture.arrayDescriptorIndex, nullptr, {});
        g_textureDescriptorAllocator.free(texture.arrayDescriptorIndex);
        texture.arrayDescriptorIndex = NULL;
    }
}

static std::unique_ptr<RenderPipelineLayout> g_pipelineLayout;
static xxHashMap<std::unique_ptr<RenderPipeline>> g_pipelines;

#ifdef ASYNC_PSO_DEBUG
static std::atomic<uint32_t> g_pipelinesCreatedInRenderThread;
static std::atomic<uint32_t> g_pipelinesCreatedAsynchronously;
static std::atomic<uint32_t> g_pipelinesDropped;
static std::atomic<uint32_t> g_pipelinesCurrentlyCompiling;
static std::string g_pipelineDebugText;
static Mutex g_debugMutex;
#endif

#ifdef PSO_CACHING
static xxHashMap<PipelineState> g_pipelineStatesToCache;
static Mutex g_pipelineCacheMutex;
#endif

static std::atomic<uint32_t> g_compilingPipelineTaskCount;
static std::atomic<uint32_t> g_pendingPipelineTaskCount;

enum class PipelineTaskType
{
    Null,
    PrecompilePipelines,
    RecompilePipelines
};

struct PipelineTask
{
    PipelineTaskType type{};
};

static Mutex g_pipelineTaskMutex;
static std::vector<PipelineTask> g_pipelineTaskQueue;

static std::thread::id g_mainThreadId = std::this_thread::get_id();

class LoadingPipelineCompilationEvent : public ContextHookEvent<Sonicteam::LoadingTask>
{
public:
    void Postfix(Sonicteam::LoadingTask* in_pThis) override
    {
        assert(std::this_thread::get_id() == g_mainThreadId);

        // Wait for pipeline compilations to finish.
        uint32_t value;
        while ((value = g_compilingPipelineTaskCount.load()) != 0)
        {
            // Pump SDL events to prevent the OS
            // from thinking the process is unresponsive.
            SDL_PumpEvents();
            SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

            g_compilingPipelineTaskCount.wait(value);
        }
    }
}
g_loadingPipelineCompilationEvent{};

static void EnqueuePipelineTask(PipelineTaskType type)
{
    // Precompiled pipelines deliberately do not increment
    // this counter to overlap the compilation with intro logos.
    if (type != PipelineTaskType::PrecompilePipelines)
        ++g_compilingPipelineTaskCount;

    {
        std::lock_guard lock(g_pipelineTaskMutex);
        g_pipelineTaskQueue.emplace_back(type);
    }

    if ((++g_pendingPipelineTaskCount) == 1)
        g_pendingPipelineTaskCount.notify_one();
}

static const PipelineState g_pipelineStateCache[] =
{
#include "cache/pipeline_state_cache.h"
};

#include "cache/vertex_element_cache.h"

static uint8_t* const g_vertexDeclarationCache[] =
{
#include "cache/vertex_declaration_cache.h"
};

static xxHashMap<std::pair<uint32_t, std::unique_ptr<RenderSampler>>> g_samplerStates;

static Mutex g_vertexDeclarationMutex;
static xxHashMap<GuestVertexDeclaration*> g_vertexDeclarations;

struct UploadBuffer
{
    static constexpr size_t SIZE = 16 * 1024 * 1024;

    std::unique_ptr<RenderBuffer> buffer;
    uint8_t* memory = nullptr;
    uint64_t deviceAddress = 0;
};

struct UploadAllocator
{
    std::vector<UploadBuffer> buffers;
    uint32_t index = 0;
    uint32_t offset = 0;

    void prepareAllocation(uint32_t size, uint32_t alignment)
    {
        assert(size <= UploadBuffer::SIZE);

        offset = (offset + alignment - 1) & ~(alignment - 1);

        if (offset + size > UploadBuffer::SIZE)
        {
            ++index;
            offset = 0;
        }
    }

    UploadAllocation allocate(uint32_t size, uint32_t alignment)
    {
        prepareAllocation(size, alignment);
        if (buffers.size() <= index)
            buffers.resize(index + 1);

        auto& buffer = buffers[index];
        if (buffer.buffer == nullptr)
        {
            const auto constantFlags = g_useBufferConstants ? RenderBufferFlag::STORAGE :
                RenderBufferFlag::CONSTANT | RenderBufferFlag::DEVICE_ADDRESSABLE;
            buffer.buffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(UploadBuffer::SIZE,
                RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX | constantFlags));
            buffer.memory = reinterpret_cast<uint8_t*>(buffer.buffer->map());
            if (!g_useBufferConstants)
                buffer.deviceAddress = buffer.buffer->getDeviceAddress();
        }
        
        auto ref = buffer.buffer->at(offset);
        offset += size;

        return { ref.ref, ref.offset, buffer.memory + ref.offset, buffer.deviceAddress + ref.offset };
    }

    template<bool TByteSwap, typename T>
    UploadAllocation allocate(const T* memory, uint32_t size, uint32_t alignment)
    {
        auto result = allocate(size, alignment);

        if constexpr (TByteSwap)
        {
            auto destination = reinterpret_cast<T*>(result.memory);

            for (size_t i = 0; i < size; i += sizeof(T))
            {
                *destination = ByteSwap(*memory);
                ++destination;
                ++memory;
            }
        }
        else
        {
            memcpy(result.memory, memory, size);
        }

        return result;
    }

    void reset()
    {
        index = 0;
        offset = 0;
    }
};

static UploadAllocator g_uploadAllocators[NUM_FRAMES];

struct IntermediaryUploadAllocator
{
    static constexpr size_t SIZE = 16 * 1024 * 1024;

    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    uint32_t index = 0;
    uint32_t offset = 0;

    uint8_t* allocate(uint32_t size)
    {
        assert(size <= SIZE);

        if (offset + size > SIZE)
        {
            ++index;
            offset = 0;
        }

        if (buffers.size() <= index)
            buffers.resize(index + 1);

        auto& buffer = buffers[index];
        if (buffer == nullptr)
            buffer = std::make_unique_for_overwrite<uint8_t[]>(SIZE);

        auto result = buffer.get() + offset;
        offset += ((size + 0xF) & ~0xF);

        return result;
    }

    uint8_t* allocate(const void* memory, uint32_t size)
    {
        auto result = allocate(size);
        memcpy(result, memory, size);
        return result;
    }

    void reset()
    {
        index = 0;
        offset = 0;
    }
};

static IntermediaryUploadAllocator g_intermediaryUploadAllocator;

static std::vector<GuestResource*> g_tempResources[NUM_FRAMES];
static std::vector<std::unique_ptr<RenderBuffer>> g_tempBuffers[NUM_FRAMES];

template<GuestPrimitiveType PrimitiveType>
struct PrimitiveIndexData
{
    std::vector<uint16_t> indexData;
    RenderBufferReference indexBuffer;
    uint32_t currentIndexCount = 0;

    uint32_t prepare(uint32_t guestPrimCount)
    {
        uint32_t primCount;
        uint32_t indexCountPerPrimitive;

        switch (PrimitiveType)
        {
        case D3DPT_TRIANGLEFAN:
            primCount = guestPrimCount - 2;
            indexCountPerPrimitive = 3; 
            break;
        case D3DPT_QUADLIST:
            primCount = guestPrimCount / 4;
            indexCountPerPrimitive = 6;
            break;
        default:
            assert(false && "Unknown primitive type.");
            break;
        }

        uint32_t indexCount = primCount * indexCountPerPrimitive;

        if (indexData.size() < indexCount)
        {
            const size_t oldPrimCount = indexData.size() / indexCountPerPrimitive;
            indexData.resize(indexCount);

            for (size_t i = oldPrimCount; i < primCount; i++)
            {
                switch (PrimitiveType)
                {
                case D3DPT_TRIANGLEFAN:
                {
                    indexData[i * 3 + 0] = 0;
                    indexData[i * 3 + 1] = static_cast<uint16_t>(i + 1);
                    indexData[i * 3 + 2] = static_cast<uint16_t>(i + 2);
                    break;
                }
                case D3DPT_QUADLIST:
                {
                    indexData[i * 6 + 0] = static_cast<uint16_t>(i * 4 + 0);
                    indexData[i * 6 + 1] = static_cast<uint16_t>(i * 4 + 1);
                    indexData[i * 6 + 2] = static_cast<uint16_t>(i * 4 + 2);

                    indexData[i * 6 + 3] = static_cast<uint16_t>(i * 4 + 0);
                    indexData[i * 6 + 4] = static_cast<uint16_t>(i * 4 + 2);
                    indexData[i * 6 + 5] = static_cast<uint16_t>(i * 4 + 3);
                    break;
                }
                default:
                    assert(false && "Unknown primitive type.");
                    break;
                }
            }
        }

        if (indexBuffer == NULL || currentIndexCount < indexCount)
        {
            auto allocation = g_uploadAllocators[g_frame].allocate<false>(indexData.data(), indexCount * 2, 2);
            indexBuffer = allocation.buffer->at(allocation.offset);
            currentIndexCount = indexCount;
        }

        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, indexBuffer);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, indexCount * 2);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, RenderFormat::R16_UINT);

        return indexCount;
    }

    void reset()
    {
        indexBuffer = {};
        currentIndexCount = 0;
    }
};

static PrimitiveIndexData<D3DPT_TRIANGLEFAN> g_triangleFanIndexData;
static PrimitiveIndexData<D3DPT_QUADLIST> g_quadIndexData;

// Host render target storage for guest surfaces, identified by EDRAM placement.
struct SurfaceVariant
{
    uint32_t base = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t guestFormat = 0;
    RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
    RenderFormat format = RenderFormat::UNKNOWN;
    std::unique_ptr<RenderTexture> textureHolder;
    std::unique_ptr<RenderTextureView> textureView;
    uint32_t descriptorIndex = 0;
    // Barrier state is shared by all views, as they render to the same texture.
    RenderTextureLayout layout = RenderTextureLayout::UNKNOWN;
    // Framebuffers are cached here instead of on the guest surface so they don't
    // outlive the host texture when views are destroyed.
    ankerl::unordered_dense::map<const RenderTexture*, std::unique_ptr<RenderFramebuffer>> framebuffers;
    uint32_t refCount = 0;

    uint32_t deferredClearFlags = 0;
    float deferredClearColor[4]{};
    float deferredClearZ = 1.0f;
    uint32_t deferredClearStencil = 0;
};

static Mutex g_surfaceVariantMutex;
static std::vector<std::unique_ptr<SurfaceVariant>> g_surfaceVariants;

static ankerl::unordered_dense::set<GuestSurface*> g_pendingSurfaceCopies;
static ankerl::unordered_dense::set<GuestSurface*> g_pendingResolves;

static bool CanRenderToSlice(const GuestSurface* surface, const GuestTexture* texture)
{
    return g_directCsmSlice &&
        texture->type == ResourceType::ArrayTexture &&
        RenderFormatIsDepth(surface->format) &&
        surface->sampleCount == RenderSampleCount::COUNT_1 &&
        surface->format == texture->format &&
        surface->width == texture->width &&
        surface->height == texture->height;
}

static void SetSliceRedirect(GuestSurface* surface, GuestTexture* texture, uint32_t slice)
{
    surface->sliceTexture = texture;
    surface->slice = slice;
    surface->texture = texture->texture;
    texture->sliceSurfaces.emplace(surface);
    g_dirtyStates.renderTargetAndDepthStencil = true;
}

static void ClearSliceRedirect(GuestSurface* surface)
{
    surface->sliceTexture = nullptr;
    surface->texture = surface->variant != nullptr ? surface->variant->textureHolder.get() : surface->textureHolder.get();
    g_dirtyStates.renderTargetAndDepthStencil = true;
}

static void ReleaseSurfaceVariant(SurfaceVariant* variant)
{
    std::lock_guard lock(g_surfaceVariantMutex);

    assert(variant->refCount != 0);
    if (--variant->refCount != 0)
        return;

    SetTextureDescriptor(variant->descriptorIndex, nullptr, {});
    g_textureDescriptorAllocator.free(variant->descriptorIndex);

    for (auto& other : g_surfaceVariants)
    {
        if (other.get() != variant)
            other->framebuffers.erase(variant->textureHolder.get());
    }

    g_framebuffer = nullptr;

    std::erase_if(g_surfaceVariants, [variant](auto& entry) { return entry.get() == variant; });
}

static void PurgeSurfaceVariantFramebuffers()
{
    std::lock_guard lock(g_surfaceVariantMutex);

    for (auto& variant : g_surfaceVariants)
        variant->framebuffers.clear();

    g_framebuffer = nullptr;
}

static std::atomic<uint64_t> g_completedCopyBatchId{ 0 };

static inline void EnsureTextureCopyCompleted(const GuestBaseTexture* texture)
{
    if (texture != nullptr && texture->pendingCopyBatchId > g_completedCopyBatchId.load(std::memory_order_acquire))
        Video::FlushPendingUploads();
}

static void DestructTempResources()
{
    for (auto resource : g_tempResources[g_frame])
    {
        switch (resource->type)
        {
        case ResourceType::Texture:
        case ResourceType::VolumeTexture:
        case ResourceType::ArrayTexture:
        {
            const auto texture = reinterpret_cast<GuestTexture*>(resource);
            EnsureTextureCopyCompleted(texture);
            if (texture->patchedTexture != nullptr)
                EnsureTextureCopyCompleted(texture->patchedTexture.get());

            if (texture->sourceSurface != nullptr)
                texture->sourceSurface->destinationTextures.erase(texture);

            for (const auto surface : texture->sliceSurfaces)
                ClearSliceRedirect(surface);

            if (texture->mappedMemory != nullptr) {
                g_userHeap.Free(texture->mappedMemory);
            }

            SetTextureDescriptor(texture->descriptorIndex, nullptr, {});
            g_textureDescriptorAllocator.free(texture->descriptorIndex);
            FreeArrayTextureView(*texture);

            if (texture->patchedTexture != nullptr)
            {
                SetTextureDescriptor(texture->patchedTexture->descriptorIndex, nullptr, {});
                g_textureDescriptorAllocator.free(texture->patchedTexture->descriptorIndex);
                FreeArrayTextureView(*texture->patchedTexture);
            }

            texture->~GuestTexture();
            break;
        }

        case ResourceType::VertexBuffer:
        case ResourceType::IndexBuffer:
        {
            const auto buffer = reinterpret_cast<GuestBuffer*>(resource);


            if (buffer->mappedMemory != nullptr)
                g_userHeap.Free(buffer->mappedMemory);

            buffer->~GuestBuffer();
            break;
        }

        case ResourceType::RenderTarget:
        case ResourceType::DepthStencil:
        {
            const auto surface = reinterpret_cast<GuestSurface*>(resource);

            g_pendingSurfaceCopies.erase(surface);
            g_pendingResolves.erase(surface);

            if (g_renderTarget == surface)
            {
                g_renderTarget = nullptr;
                g_dirtyStates.renderTargetAndDepthStencil = true;
            }

            if (g_depthStencil == surface)
            {
                g_depthStencil = nullptr;
                g_dirtyStates.renderTargetAndDepthStencil = true;
            }

            for (const auto& [texture, _] : surface->destinationTextures)
                texture->sourceSurface = nullptr;

            if (surface->sliceTexture != nullptr)
                surface->sliceTexture->sliceSurfaces.erase(surface);

            if (surface->variant != nullptr)
            {
                ReleaseSurfaceVariant(surface->variant);
            }
            else if (surface->descriptorIndex != NULL)
            {
                SetTextureDescriptor(surface->descriptorIndex, nullptr, {});
                g_textureDescriptorAllocator.free(surface->descriptorIndex);
            }

            surface->~GuestSurface();
            break;
        }

        case ResourceType::VertexDeclaration:
            reinterpret_cast<GuestVertexDeclaration*>(resource)->~GuestVertexDeclaration();
            break;

        case ResourceType::VertexShader:
        case ResourceType::PixelShader:
        {
            ForgetVertexLocationRemap(reinterpret_cast<GuestShader*>(resource)->shader.get());
            reinterpret_cast<GuestShader*>(resource)->~GuestShader();
            break;
        }
        }

        g_userHeap.Free(resource);
    }

    g_tempResources[g_frame].clear();
    g_tempBuffers[g_frame].clear();
}

static std::thread::id g_presentThreadId = std::this_thread::get_id();
static std::atomic<bool> g_readyForCommands;

// Upstream #394 (reimplementation of UnleashedRecomp #1486): a thread taking over the D3D
// device waits until the next command list has begun, so ownership never changes between
// Present and BeginCommandList. Mid-asm hook in Marathon.toml.in.
void App_AcquireD3DThread()
{
    // Guard against thread ownership changes when between command lists.
    g_readyForCommands.wait(false);
}

// PPC_FUNC_IMPL(__imp__sub_824ECA00);
// PPC_FUNC(sub_824ECA00)
// {
//     g_readyForCommands.wait(false);
//     g_presentThreadId = std::this_thread::get_id();
//     __imp__sub_824ECA00(ctx, base);
// }

static ankerl::unordered_dense::map<RenderTexture*, RenderTextureLayout> g_barrierMap;

static RenderTextureLayout& GetLayoutRef(GuestBaseTexture* texture)
{
    if (texture->type == ResourceType::RenderTarget || texture->type == ResourceType::DepthStencil)
    {
        const auto surface = reinterpret_cast<GuestSurface*>(texture);
        if (surface->sliceTexture != nullptr)
            return surface->sliceTexture->layout;

        if (surface->variant != nullptr && surface->texture == surface->variant->textureHolder.get())
            return surface->variant->layout;
    }

    return texture->layout;
}

static void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout, bool synchronizeSameLayout = false)
{
    if (texture != nullptr)
    {
        EnsureTextureCopyCompleted(texture);
        auto& layoutRef = GetLayoutRef(texture);
        if (layoutRef != layout || synchronizeSameLayout)
        {
            g_barrierMap[texture->texture] = layout;
            layoutRef = layout;
        }
    }
}

static std::vector<RenderTextureBarrier> g_barriers;

static bool FlushBarriers(const RenderTexture* depthCopySource = nullptr, const RenderTexture* depthCopyDestination = nullptr)
{
    if (g_barrierMap.empty() && depthCopySource == nullptr)
        return false;

    for (auto& [texture, layout] : g_barrierMap)
        g_barriers.emplace_back(texture, layout);

    auto& commandList = g_commandLists[g_frame];
    const bool clearDeferred = depthCopySource != nullptr &&
        commandList->deferFramebufferClear(depthCopySource, depthCopyDestination, g_barriers.data(), uint32_t(g_barriers.size()));
    if (!g_barriers.empty())
        commandList->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY, g_barriers);

    g_barrierMap.clear();
    g_barriers.clear();
    return clearDeferred;
}

static std::unique_ptr<uint8_t[]> g_shaderCache;
static std::unique_ptr<uint8_t[]> g_buttonBcDiff;

static void LoadEmbeddedResources()
{
    switch (g_backend)
    {
    case Backend::VULKAN:
        g_shaderCache = std::make_unique<uint8_t[]>(g_spirvCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_spirvCacheDecompressedSize, g_compressedSpirvCache, g_spirvCacheCompressedSize);
        break;
#if defined(MARATHON_RECOMP_D3D12)
    case Backend::D3D12:
        g_shaderCache = std::make_unique<uint8_t[]>(g_dxilCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_dxilCacheDecompressedSize, g_compressedDxilCache, g_dxilCacheCompressedSize);
        break;
#elif defined(MARATHON_RECOMP_METAL)
    case Backend::METAL:
        g_shaderCache = std::make_unique<uint8_t[]>(g_airCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_airCacheDecompressedSize, g_compressedAirCache, g_airCacheCompressedSize);
        break;
#endif
    default:
        assert(false);
    }

    g_buttonBcDiff = decompressZstd(g_button_bc_diff, g_button_bc_diff_uncompressed_size);
}

struct ShaderConstantRange
{
    uint32_t index;
    uint32_t size;
};

enum class RenderCommandType
{
    SetRenderState,
    DestructResource,
    UnlockTextureRect,
    UnlockBuffer,
    DrawImGui,
    ExecuteCommandList,
    BeginCommandList,
    StretchRect,
    SetRenderTarget,
    SetDepthStencilSurface,
    ExecutePendingStretchRectCommands,
    Clear,
    SetViewport,
    SetTexture,
    SetScissorRect,
    SetSamplerState,
    SetBooleans,
    SetVertexShaderConstants,
    SetPixelShaderConstants,
    SetDrawShaderConstants,
    AddPipeline,
    DrawPrimitive,
    DrawIndexedPrimitive,
    DrawPrimitiveUP,
    SetVertexDeclaration,
    SetVertexShader,
    SetStreamSource,
    SetIndices,
    SetPixelShader,
    SetConditionalSurvey,
    SetConditionalRendering,
};

struct RenderCommand
{
    RenderCommandType type;
    union
    {
        struct
        {
            GuestRenderState type;
            uint32_t value;
        } setRenderState;

        struct 
        {
            GuestResource* resource;
        } destructResource;

        struct
        {
            GuestTexture* texture;
            uint8_t* data;
        } unlockTextureRect;

        struct
        {
            GuestBuffer* buffer;
            RenderBuffer* upload;
        } unlockBuffer;

        struct
        {
            uint32_t flags;
            GuestTexture* texture;
            uint32_t destSliceOrFace;
            uint32_t guestReturnAddress;
        } stretchRect;

        struct 
        {
            GuestSurface* renderTarget;
        } setRenderTarget;

        struct 
        {
            GuestSurface* depthStencil;
            DepthCopyDestination plannedDestination;
        } setDepthStencilSurface;

        struct 
        {
            uint32_t flags;
            float color[4];
            float z;
            uint32_t stencil;
        } clear;

        struct 
        {
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;
        } setViewport;

        struct 
        {
            uint32_t index;
            GuestTexture* texture;
        } setTexture;

        struct 
        {
            int32_t left;
            int32_t top;
            int32_t right;
            int32_t bottom;
        } setScissorRect;

        struct
        {
            uint32_t index;
            uint32_t data0;
            uint32_t data3;
            uint32_t data5;
        } setSamplerState;

        struct
        {
            uint32_t booleans;
        } setBooleans;

        struct
        {
            const uint8_t* memory;
            ShaderConstantRange range;
        } setShaderConstants;

        struct
        {
            const uint8_t* memory;
            ShaderConstantRange vertex;
            ShaderConstantRange pixel;
        } setDrawShaderConstants;

        struct
        {
            XXH64_hash_t hash;
            RenderPipeline* pipeline;
        } addPipeline;

        struct 
        {
            uint32_t primitiveType; 
            uint32_t startVertex; 
            uint32_t primitiveCount;
        } drawPrimitive;

        struct 
        {
            uint32_t primitiveType;
            int32_t baseVertexIndex; 
            uint32_t startIndex;
            uint32_t primCount;
        } drawIndexedPrimitive;

        struct 
        {
            uint32_t primitiveType;
            uint32_t primitiveCount; 
            uint8_t* vertexStreamZeroData;
            uint32_t vertexStreamZeroSize;
            uint32_t vertexStreamZeroStride;
        } drawPrimitiveUP;

        struct 
        {
            GuestVertexDeclaration* vertexDeclaration;
        } setVertexDeclaration;

        struct 
        {
            GuestShader* shader;
        } setVertexShader;

        struct 
        {
            uint32_t index;
            GuestBuffer* buffer;
            uint32_t offset;
            uint32_t stride;
        } setStreamSource;

        struct 
        {
            GuestBuffer* buffer;
        } setIndices;

        struct 
        {
            GuestShader* shader;
        } setPixelShader;

        struct
        {
            bool enabled;
            uint32_t index;
        } setConditionalSurvey;

        struct
        {
            bool enabled;
            uint32_t index;
        } setConditionalRendering;
    };
};

static_assert(sizeof(decltype(RenderCommand::setDrawShaderConstants)) <= sizeof(decltype(RenderCommand::drawPrimitiveUP)));

static moodycamel::BlockingConcurrentQueue<RenderCommand> g_renderQueue;
static moodycamel::ProducerToken g_orderedRenderProducer(g_renderQueue);
static std::mutex g_renderProducerMutex;

static bool EnqueueRenderCommand(const RenderCommand& cmd)
{
    if (!g_stageGuestBuffers)
        return g_renderQueue.enqueue(cmd);

    // Work submitted from a guest worker must precede draws enqueued after that worker finishes.
    std::lock_guard lock(g_renderProducerMutex);
    return g_renderQueue.enqueue(g_orderedRenderProducer, cmd);
}

static void EnqueueRenderCommands(const RenderCommand* commands, size_t count)
{
    if (!g_stageGuestBuffers)
        g_renderQueue.enqueue_bulk(commands, count);
    else
    {
        std::lock_guard lock(g_renderProducerMutex);
        g_renderQueue.enqueue_bulk(g_orderedRenderProducer, commands, count);
    }
}

template<GuestRenderState TType>
static void SetRenderState(GuestDevice* device, uint32_t value)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetRenderState;
    cmd.setRenderState.type = TType;
    cmd.setRenderState.value = value;
    EnqueueRenderCommand(cmd);
}

static void SetRenderStateUnimplemented(GuestDevice* device, uint32_t value)
{
    LOGF_WARNING("{:x}\n", value);
}

static void SetAlphaTestMode(bool enable)
{
    uint32_t specConstants = 0;
    bool enableAlphaToCoverage = false;

    if (enable)
    {
        enableAlphaToCoverage = Config::TransparencyAntiAliasing && g_renderTarget != nullptr && g_renderTarget->sampleCount != RenderSampleCount::COUNT_1;

        if (enableAlphaToCoverage)
            specConstants = SPEC_CONSTANT_ALPHA_TO_COVERAGE;
        else
            specConstants = SPEC_CONSTANT_ALPHA_TEST;
    }

    specConstants |= (g_pipelineState.specConstants & ~(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE));

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.enableAlphaToCoverage, enableAlphaToCoverage);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

static RenderBlend ConvertBlendMode(uint32_t blendMode)
{
    switch (blendMode)
    {
    case D3DBLEND_ZERO:
        return RenderBlend::ZERO;
    case D3DBLEND_ONE:
        return RenderBlend::ONE;
    case D3DBLEND_SRCCOLOR:
        return RenderBlend::SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
        return RenderBlend::INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
        return RenderBlend::SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
        return RenderBlend::INV_SRC_ALPHA;
    case D3DBLEND_DESTCOLOR:
        return RenderBlend::DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
        return RenderBlend::INV_DEST_COLOR;
    case D3DBLEND_DESTALPHA:
        return RenderBlend::DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA:
        return RenderBlend::INV_DEST_ALPHA;
    default:
        assert(false && "Invalid blend mode");
        return RenderBlend::ZERO;
    }
}

static RenderBlendOperation ConvertBlendOp(uint32_t blendOp)
{
    switch (blendOp)
    {
    case D3DBLENDOP_ADD:
        return RenderBlendOperation::ADD;
    case D3DBLENDOP_SUBTRACT:
        return RenderBlendOperation::SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT:
        return RenderBlendOperation::REV_SUBTRACT;
    case D3DBLENDOP_MIN:
        return RenderBlendOperation::MIN;
    case D3DBLENDOP_MAX:
        return RenderBlendOperation::MAX;
    default:
        assert(false && "Unknown blend operation");
        return RenderBlendOperation::ADD;
    }
}

static RenderComparisonFunction ConvertCompareFunc(uint32_t compareFunc)
{
    switch (compareFunc)
    {
    case D3DCMP_NEVER:
        return RenderComparisonFunction::NEVER;
    case D3DCMP_LESS:
        return RenderComparisonFunction::LESS;
    case D3DCMP_EQUAL:
        return RenderComparisonFunction::EQUAL;
    case D3DCMP_LESSEQUAL:
        return RenderComparisonFunction::LESS_EQUAL;
    case D3DCMP_GREATER:
        return RenderComparisonFunction::GREATER;
    case D3DCMP_NOTEQUAL:
        return RenderComparisonFunction::NOT_EQUAL;
    case D3DCMP_GREATEREQUAL:
        return RenderComparisonFunction::GREATER_EQUAL;
    case D3DCMP_ALWAYS:
        return RenderComparisonFunction::ALWAYS;
    default:
        assert(false && "Unknown comparison function");
        return RenderComparisonFunction::NEVER;
    }
}

static RenderStencilOp ConvertStencilOp(uint32_t stencilOp)
{
    switch (stencilOp)
    {
    case D3DSTENCILOP_KEEP:
        return RenderStencilOp::KEEP;
    case D3DSTENCILOP_ZERO:
        return RenderStencilOp::ZERO;
    case D3DSTENCILOP_REPLACE:
        return RenderStencilOp::REPLACE;
    case D3DSTENCILOP_INCRSAT:
        return RenderStencilOp::INCREMENT_AND_CLAMP;
    case D3DSTENCILOP_DECRSAT:
        return RenderStencilOp::DECREMENT_AND_CLAMP;
    case D3DSTENCILOP_INVERT:
        return RenderStencilOp::INVERT;
    case D3DSTENCILOP_INCR:
        return RenderStencilOp::INCREMENT_AND_WRAP;
    case D3DSTENCILOP_DECR:
        return RenderStencilOp::DECREMENT_AND_WRAP;
    default:
        assert(false && "Unknown stencil op");
        return RenderStencilOp::KEEP;
    }
}

static void ProcSetRenderState(const RenderCommand& cmd)
{
    uint32_t value = cmd.setRenderState.value;

    switch (cmd.setRenderState.type)
    {
    case D3DRS_ZENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, value != 0);
        g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
        break;
    }
    case D3DRS_ZWRITEENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
        break;
    }
    case D3DRS_STENCILENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, value != 0);
        g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
        break;
    }
    case D3DRS_TWOSIDEDSTENCILMODE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilTwoSided, value != 0);
        break;
    }
    case D3DRS_ALPHATESTENABLE:
    {
        SetAlphaTestMode(value != 0);
        break;
    }
    case D3DRS_SRCBLEND:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlend, ConvertBlendMode(value));
        break;
    }
    case D3DRS_DESTBLEND:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend, ConvertBlendMode(value));
        break;
    }
    case D3DRS_CULLMODE:
    {
        RenderCullMode cullMode;

        switch (value) {
        case D3DCULL_NONE_CCW:
        case D3DCULL_NONE_CW:
            cullMode = RenderCullMode::NONE;
            break;
        case D3DCULL_FRONT_CCW:
        case D3DCULL_FRONT_CW:
            cullMode = RenderCullMode::FRONT;
            break;
        case D3DCULL_BACK_CCW:
        case D3DCULL_BACK_CW:
            cullMode = RenderCullMode::BACK;
            break;
        default:
            assert(false && "Invalid cull mode");
            cullMode = RenderCullMode::NONE;
            break;
        }

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.cullMode, cullMode);
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.frontFace, value < D3DCULL_NONE_CW ? RenderFrontFace::COUNTER_CLOCKWISE : RenderFrontFace::CLOCKWISE);
        break;
    }
    case D3DRS_ZFUNC:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, ConvertCompareFunc(value));
        break;
    }
    case D3DRS_STENCILFUNC:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFunc, ConvertCompareFunc(value));
        break;
    }
    case D3DRS_STENCILFAIL:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFail, ConvertStencilOp(value));
        break;
    }
    case D3DRS_STENCILZFAIL:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilZFail, ConvertStencilOp(value));
        break;
    }
    case D3DRS_STENCILPASS:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilPass, ConvertStencilOp(value));
        break;
    }
    case D3DRS_CCW_STENCILFUNC:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFuncCCW, ConvertCompareFunc(value));
        break;
    }
    case D3DRS_CCW_STENCILFAIL:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFailCCW, ConvertStencilOp(value));
        break;
    }
    case D3DRS_CCW_STENCILZFAIL:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilZFailCCW, ConvertStencilOp(value));
        break;
    }
    case D3DRS_CCW_STENCILPASS:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilPassCCW, ConvertStencilOp(value));
        break;
    }
    case D3DRS_STENCILREF:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilRef, value);
        break;
    }
    case D3DRS_STENCILMASK:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilMask, value);
        break;
    }
    case D3DRS_STENCILWRITEMASK:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilWriteMask, value);
        break;
    }
    case D3DRS_ALPHAREF:
    {
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.alphaThreshold, float(value) / 256.0f);
        break;
    }
    case D3DRS_ALPHABLENDENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.alphaBlendEnable, value != 0);
        break;
    }
    case D3DRS_BLENDOP:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp, ConvertBlendOp(value));
        break;
    }
    case D3DRS_SCISSORTESTENABLE:
    {
        // HACK: Ignore scissor test on depth-only draws to allow CSM:3 to properly scale
        if (g_pipelineState.depthStencilFormat == RenderFormat::UNKNOWN || g_pipelineState.renderTargetFormat != RenderFormat::UNKNOWN)
            SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
        break;
    }
    case D3DRS_SLOPESCALEDEPTHBIAS:
    {
        if (g_capabilities.dynamicDepthBias)
            SetDirtyValue(g_dirtyStates.depthBias, g_slopeScaledDepthBias, *reinterpret_cast<float*>(&value));
        else 
            SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, *reinterpret_cast<float*>(&value));

        break;
    }
    case D3DRS_DEPTHBIAS:
    {
        if (g_capabilities.dynamicDepthBias)
            SetDirtyValue(g_dirtyStates.depthBias, g_depthBias, int32_t(*reinterpret_cast<float*>(&value) * (1 << 24)));
        else
            SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, int32_t(*reinterpret_cast<float*>(&value)* (1 << 24)));

        break;
    }
    case D3DRS_SRCBLENDALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha, ConvertBlendMode(value));
        break;
    }
    case D3DRS_DESTBLENDALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha, ConvertBlendMode(value));
        break;
    }
    case D3DRS_BLENDOPALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOpAlpha, ConvertBlendOp(value));
        break;
    }
    case D3DRS_COLORWRITEENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, value);
        g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
        break;
    }
    case D3DRS_CLIPPLANEENABLE:
    {
        // The game uses clip plane 0.
        const bool enabled = (value & 1) != 0;
        if (g_useSoftwareClip)
        {
            const uint32_t specConstants = (g_pipelineState.specConstants & ~SPEC_CONSTANT_CLIP_PLANE) |
                (enabled ? SPEC_CONSTANT_CLIP_PLANE : 0);
            SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
        }
        else
        {
            SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.clipPlaneEnabled, enabled);
        }
        break;
    }
    }
}

static const std::pair<GuestRenderState, PPCFunc*> g_setRenderStateFunctions[] =
{
    { D3DRS_ZENABLE, HostToGuestFunction<SetRenderState<D3DRS_ZENABLE>> },
    { D3DRS_ZWRITEENABLE, HostToGuestFunction<SetRenderState<D3DRS_ZWRITEENABLE>> },
    { D3DRS_ALPHATESTENABLE, HostToGuestFunction<SetRenderState<D3DRS_ALPHATESTENABLE>> },
    { D3DRS_SRCBLEND, HostToGuestFunction<SetRenderState<D3DRS_SRCBLEND>> },
    { D3DRS_DESTBLEND, HostToGuestFunction<SetRenderState<D3DRS_DESTBLEND>> },
    { D3DRS_CULLMODE, HostToGuestFunction<SetRenderState<D3DRS_CULLMODE>> },
    { D3DRS_ZFUNC, HostToGuestFunction<SetRenderState<D3DRS_ZFUNC>> },
    { D3DRS_ALPHAREF, HostToGuestFunction<SetRenderState<D3DRS_ALPHAREF>> },
    { D3DRS_ALPHABLENDENABLE, HostToGuestFunction<SetRenderState<D3DRS_ALPHABLENDENABLE>> },
    { D3DRS_BLENDOP, HostToGuestFunction<SetRenderState<D3DRS_BLENDOP>> },
    { D3DRS_SCISSORTESTENABLE, HostToGuestFunction<SetRenderState<D3DRS_SCISSORTESTENABLE>> },
    { D3DRS_SLOPESCALEDEPTHBIAS, HostToGuestFunction<SetRenderState<D3DRS_SLOPESCALEDEPTHBIAS>> },
    { D3DRS_DEPTHBIAS, HostToGuestFunction<SetRenderState<D3DRS_DEPTHBIAS>> },
    { D3DRS_SRCBLENDALPHA, HostToGuestFunction<SetRenderState<D3DRS_SRCBLENDALPHA>> },
    { D3DRS_DESTBLENDALPHA, HostToGuestFunction<SetRenderState<D3DRS_DESTBLENDALPHA>> },
    { D3DRS_BLENDOPALPHA, HostToGuestFunction<SetRenderState<D3DRS_BLENDOPALPHA>> },
    { D3DRS_COLORWRITEENABLE, HostToGuestFunction<SetRenderState<D3DRS_COLORWRITEENABLE>> },
    { D3DRS_STENCILENABLE, HostToGuestFunction<SetRenderState<D3DRS_STENCILENABLE>> },
    { D3DRS_TWOSIDEDSTENCILMODE, HostToGuestFunction<SetRenderState<D3DRS_TWOSIDEDSTENCILMODE>> },
    { D3DRS_STENCILFAIL, HostToGuestFunction<SetRenderState<D3DRS_STENCILFAIL>> },
    { D3DRS_STENCILZFAIL, HostToGuestFunction<SetRenderState<D3DRS_STENCILZFAIL>> },
    { D3DRS_STENCILPASS, HostToGuestFunction<SetRenderState<D3DRS_STENCILPASS>> },
    { D3DRS_STENCILFUNC, HostToGuestFunction<SetRenderState<D3DRS_STENCILFUNC>> },
    { D3DRS_STENCILREF, HostToGuestFunction<SetRenderState<D3DRS_STENCILREF>> },
    { D3DRS_STENCILMASK, HostToGuestFunction<SetRenderState<D3DRS_STENCILMASK>> },
    { D3DRS_STENCILWRITEMASK, HostToGuestFunction<SetRenderState<D3DRS_STENCILWRITEMASK>> },
    { D3DRS_CCW_STENCILFAIL, HostToGuestFunction<SetRenderState<D3DRS_CCW_STENCILFAIL>> },
    { D3DRS_CCW_STENCILZFAIL, HostToGuestFunction<SetRenderState<D3DRS_CCW_STENCILZFAIL>> },
    { D3DRS_CCW_STENCILPASS, HostToGuestFunction<SetRenderState<D3DRS_CCW_STENCILPASS>> },
    { D3DRS_CCW_STENCILFUNC, HostToGuestFunction<SetRenderState<D3DRS_CCW_STENCILFUNC>> },
    { D3DRS_CLIPPLANEENABLE, HostToGuestFunction<SetRenderState<D3DRS_CLIPPLANEENABLE>> }
};

static std::unique_ptr<RenderShader> g_copyShader;

static std::unique_ptr<RenderShader> g_copyColorShader;
static ankerl::unordered_dense::map<RenderFormat, std::unique_ptr<RenderPipeline>> g_copyColorPipelines;
static std::unique_ptr<RenderPipeline> g_copyDepthPipeline;

static std::unique_ptr<RenderShader> g_resolveMsaaColorShaders[3];
static ankerl::unordered_dense::map<RenderFormat, std::array<std::unique_ptr<RenderPipeline>, 3>> g_resolveMsaaColorPipelines;
static std::unique_ptr<RenderPipeline> g_resolveMsaaDepthPipelines[3];

static std::unique_ptr<RenderShader> g_enhancedBurnoutBlurVSShader;
static std::unique_ptr<RenderShader> g_enhancedBurnoutBlurPSShader;

static std::unique_ptr<RenderShader> g_MoviePSShader;

static std::unique_ptr<GuestShader> g_conditionalSurveyPSShader;

#if defined(MARATHON_RECOMP_D3D12)

#define CREATE_SHADER(NAME) \
    g_device->createShader( \
        (g_backend == Backend::VULKAN) ? g_##NAME##_spirv : g_##NAME##_dxil, \
        (g_backend == Backend::VULKAN) ? sizeof(g_##NAME##_spirv) : sizeof(g_##NAME##_dxil), \
        "shaderMain", \
        (g_backend == Backend::VULKAN) ? RenderShaderFormat::SPIRV : RenderShaderFormat::DXIL)

#elif defined(MARATHON_RECOMP_METAL)

#define CREATE_SHADER(NAME) \
    g_device->createShader( \
        (g_backend == Backend::VULKAN) ? g_##NAME##_spirv : g_##NAME##_air, \
        (g_backend == Backend::VULKAN) ? sizeof(g_##NAME##_spirv) : sizeof(g_##NAME##_air), \
        "shaderMain", \
        (g_backend == Backend::VULKAN) ? RenderShaderFormat::SPIRV : RenderShaderFormat::METAL)

#elif defined(__ANDROID__)

#define CREATE_SHADER(NAME) \
    (g_useFixedDescriptors ? CreateSpirvShader(g_##NAME##_fixed_spirv, sizeof(g_##NAME##_fixed_spirv)) : \
                             CreateSpirvShader(g_##NAME##_spirv, sizeof(g_##NAME##_spirv)))

#else

#define CREATE_SHADER(NAME) \
    CreateSpirvShader(g_##NAME##_spirv, sizeof(g_##NAME##_spirv))

#endif

#ifdef __ANDROID__
#define CREATE_CONSTANT_SHADER(NAME) \
    (g_useBufferConstants ? CreateSpirvShader(g_##NAME##_buffer_spirv, sizeof(g_##NAME##_buffer_spirv)) : \
                            CREATE_SHADER(NAME))
#else
#define CREATE_CONSTANT_SHADER(NAME) CREATE_SHADER(NAME)
#endif

#ifdef _WIN32
static bool DetectWine()
{
    HMODULE dllHandle = GetModuleHandle("ntdll.dll");
    return dllHandle != nullptr && GetProcAddress(dllHandle, "wine_get_version") != nullptr;
}
#endif

static constexpr size_t TEXTURE_DESCRIPTOR_SIZE = 32768;
static constexpr size_t SAMPLER_DESCRIPTOR_SIZE = 1024;

// Clamped to the device's descriptor set limits after device creation; desktop GPUs, Adreno and
// recent Mali report more than the defaults, some mobile drivers less.
static uint32_t g_textureDescriptorSize = TEXTURE_DESCRIPTOR_SIZE;
static uint32_t g_samplerDescriptorSize = SAMPLER_DESCRIPTOR_SIZE;

// The pipeline layout uses 5 descriptor sets: textures (3 views of one set), samplers and the
// conditional survey buffer (space4). Drivers limited to 4 sets (older Mali) crash in
// vkCreatePipelineLayout or reject every pipeline that touches set 4. The compact layout moves
// the survey buffer into the sampler set (binding 0, samplers at binding 1); shaders are
// remapped to it when their SPIR-V is loaded (RemapSpirvForCompactDescriptorSets).
static bool g_compactDescriptorSets = false;

// Flat Plume descriptor index of shader sampler 0 in g_samplerDescriptorSet.
static uint32_t g_samplerSlotOffset = 0;

// Vertex inputs use locations 0-19 (see the location table in CreateVertexDeclaration), while
// Vulkan requires location < maxVertexInputAttributes, which is 16 on Mali. No shader reads more
// than 16 inputs, so with this flag every vertex shader's input locations are renumbered densely
// when its SPIR-V is loaded, and pipelines translate the declaration's locations the same way.
static bool g_compactVertexLocations = false;

// Every vertex shader writes SV_ClipDistance (Xbox user clip planes). Drivers without
// shaderClipDistance (Mali-G710 r54p3) crash compiling them. With this flag the ClipDistance
// output becomes an ordinary unused output when the SPIR-V is loaded; clip planes are lost.
static bool g_stripClipDistance = false;
static std::mutex g_vertexLocationRemapMutex;
static ankerl::unordered_dense::map<const RenderShader*, std::array<uint8_t, 32>> g_vertexLocationRemaps;

static void BuildSamplerDescriptorSet(RenderDescriptorSetBuilder& builder, bool bufferConstants = false)
{
    builder.begin();
    if (g_compactDescriptorSets)
    {
        builder.addReadWriteStructuredBuffer(0);
        builder.addSampler(1, g_samplerDescriptorSize);
        if (bufferConstants)
            builder.addByteAddressBuffer(2);
    }
    else
    {
        builder.addSampler(0, g_samplerDescriptorSize);
    }
    builder.end(!g_useFixedDescriptors, g_useFixedDescriptors ? 0 : g_samplerDescriptorSize);
}

static RenderDescriptorSetBuilder g_fixedTextureSetBuilders[TEXTURE_DESCRIPTOR_NULL_COUNT];
static RenderDescriptorSetBuilder g_fixedSamplerSetBuilder;
static RenderDescriptorSetBuilder g_fixedBufferSamplerSetBuilder;
static std::vector<const RenderSampler*> g_fixedSamplers;

using FixedDescriptorKey = std::array<uint64_t, FIXED_DESCRIPTOR_SLOTS>;

struct FixedDescriptorKeyHash
{
    uint64_t operator()(const FixedDescriptorKey& key) const
    {
        return XXH3_64bits(key.data(), sizeof(key));
    }
};

struct FixedDescriptorSets
{
    std::vector<std::unique_ptr<RenderDescriptorSet>> sets;
    ankerl::unordered_dense::map<FixedDescriptorKey, RenderDescriptorSet*, FixedDescriptorKeyHash> bindings;
    size_t next = 0;

    RenderDescriptorSet* get(const FixedDescriptorKey& key, const RenderDescriptorSetBuilder& builder,
        const auto& writeDescriptors)
    {
        if (auto existing = bindings.find(key); existing != bindings.end())
            return existing->second;

        if (next == sets.size())
        {
            auto set = builder.create(g_device.get());
            if (set == nullptr || !set->isValid())
            {
                LOG_ERROR("Could not allocate a fixed Vulkan descriptor set.");
                return nullptr;
            }
            sets.push_back(std::move(set));
        }

        auto* set = sets[next++].get();
        writeDescriptors(*set);
        bindings.emplace(key, set);
        return set;
    }

    void reset()
    {
        bindings.clear();
        next = 0;
    }
};

static FixedDescriptorSets g_fixedTextureSets[NUM_FRAMES][TEXTURE_DESCRIPTOR_NULL_COUNT];
static FixedDescriptorSets g_fixedSamplerSets[NUM_FRAMES];
static ankerl::unordered_dense::map<const RenderBuffer*, FixedDescriptorSets> g_fixedBufferSamplerSets[NUM_FRAMES];

static RenderDescriptorSet* GetFixedTextureSet(const uint32_t* indices, uint32_t blank, bool single = false)
{
    FixedDescriptorKey key{};
    std::array<FixedTextureDescriptor, FIXED_DESCRIPTOR_SLOTS> descriptors;
    const uint32_t slotMask = FixedTextureSlots(blank);
    const uint32_t count = std::popcount(slotMask);
    {
        std::lock_guard lock(g_fixedTextureMutex);
        uint32_t remaining = slotMask;
        for (uint32_t slot = 0; slot < count; ++slot)
        {
            const uint32_t guestSlot = std::countr_zero(remaining);
            remaining &= remaining - 1;
            const uint32_t index = indices[single ? 0 : guestSlot];
            const auto& descriptor = index < g_fixedTextureDescriptors.size() &&
                g_fixedTextureDescriptors[index].texture != nullptr
                ? g_fixedTextureDescriptors[index] : g_fixedTextureDescriptors[blank];

            descriptors[slot] = descriptor;
            key[slot] = descriptor.revision;
        }
    }

    const uint32_t layoutIndex = g_compactFixedDescriptors ? blank : 0;
    return g_fixedTextureSets[g_frame][layoutIndex].get(key, g_fixedTextureSetBuilders[layoutIndex], [&](RenderDescriptorSet& set)
    {
        for (uint32_t slot = 0; slot < count; ++slot)
        {
            const auto& descriptor = descriptors[slot];
            set.setTexture(slot, descriptor.texture, descriptor.layout, descriptor.view);
        }
    });
}

static RenderDescriptorSet* GetFixedTextureSetForPass(uint32_t index)
{
    return GetFixedTextureSet(&index, TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D, true);
}

static RenderDescriptorSet* GetFixedSamplerSet(const uint32_t* indices, const RenderBuffer* constantBuffer = nullptr)
{
    FixedDescriptorKey key{};
    if (indices != nullptr)
    {
        uint32_t remaining = g_compactFixedDescriptors ? COMPACT_FIXED_SAMPLER_SLOTS : ALL_FIXED_DESCRIPTOR_SLOTS;
        for (uint32_t slot = 0; slot < g_samplerDescriptorSize; ++slot)
        {
            key[slot] = indices[std::countr_zero(remaining)];
            remaining &= remaining - 1;
        }
    }
    auto& sets = constantBuffer != nullptr ? g_fixedBufferSamplerSets[g_frame][constantBuffer] : g_fixedSamplerSets[g_frame];
    const auto& builder = constantBuffer != nullptr ? g_fixedBufferSamplerSetBuilder : g_fixedSamplerSetBuilder;
    return sets.get(key, builder, [&](RenderDescriptorSet& set)
    {
        if (g_compactDescriptorSets)
        {
            const RenderBufferStructuredView surveyView(sizeof(uint32_t));
            set.setBuffer(0, g_conditionalSurveyBuffer.get(), 0, &surveyView);
        }

        for (uint32_t slot = 0; slot < g_samplerDescriptorSize; ++slot)
            set.setSampler(slot + g_samplerSlotOffset, g_fixedSamplers[key[slot]]);

        if (constantBuffer != nullptr)
            set.setBuffer(g_samplerSlotOffset + g_samplerDescriptorSize, constantBuffer);
    });
}

// The requested sets and the sets bound in Vulkan can differ after a service draw.
static constexpr uint32_t FIXED_DRAW_SAMPLER_SET = TEXTURE_DESCRIPTOR_NULL_COUNT;
static constexpr size_t FIXED_DRAW_SET_COUNT = FIXED_DRAW_SAMPLER_SET + 1;

struct FixedDrawDescriptorState
{
    uint32_t indices[FIXED_DRAW_SET_COUNT][FIXED_DESCRIPTOR_SLOTS]{};
    std::array<RenderDescriptorSet*, FIXED_DRAW_SET_COUNT> requested{};
    std::array<RenderDescriptorSet*, FIXED_DRAW_SET_COUNT> bound{};
    const RenderBuffer* constantBuffer = nullptr;
    uint64_t textureRevision = 0;
};

static FixedDrawDescriptorState g_fixedDrawDescriptorState;

static bool BindFixedDrawDescriptorSet(RenderCommandList& commandList, RenderDescriptorSet* set, uint32_t index)
{
    auto& bound = g_fixedDrawDescriptorState.bound[index];
    if (g_reuseFixedDrawDescriptors && bound == set)
        return false;

    commandList.setGraphicsDescriptorSet(set, index);
    bound = set;
    return true;
}

static bool BindFixedDrawDescriptorSets(RenderCommandList& commandList)
{
    const uint32_t* const indices[] = {
        g_sharedConstants.texture2DIndices,
        g_sharedConstants.texture2DArrayIndices,
        g_sharedConstants.textureCubeIndices,
        g_sharedConstants.samplerIndices
    };

    auto& state = g_fixedDrawDescriptorState;
    // Capture the revision before the lookups. A resource published during the lookups
    // invalidates the next draw, even if the inputs to the draw are unchanged.
    const uint64_t revision = g_reuseFixedDrawDescriptors ?
        g_fixedTextureRevision.load(std::memory_order_acquire) : 0;
    const bool texturesChanged = revision != state.textureRevision;
    const bool bufferChanged = g_drawConstantBuffer != state.constantBuffer;

    [[maybe_unused]] uint32_t lookups = 0;
    for (uint32_t i = 0; i < FIXED_DRAW_SET_COUNT; ++i)
    {
        const bool sampler = i == FIXED_DRAW_SAMPLER_SET;
        const bool resourcesChanged = sampler ? bufferChanged : texturesChanged;
        auto& requested = state.requested[i];
        auto& savedIndices = state.indices[i];
        if (!g_reuseFixedDrawDescriptors || requested == nullptr || resourcesChanged ||
            memcmp(savedIndices, indices[i], sizeof(savedIndices)) != 0)
        {
            requested = sampler ? GetFixedSamplerSet(indices[i], g_drawConstantBuffer) : GetFixedTextureSet(indices[i], i);
            if (g_reuseFixedDrawDescriptors)
                memcpy(savedIndices, indices[i], sizeof(savedIndices));
            ++lookups;
        }
    }

    state.textureRevision = revision;
    state.constantBuffer = g_drawConstantBuffer;

    if (std::find(state.requested.begin(), state.requested.end(), nullptr) != state.requested.end())
        return false;

    [[maybe_unused]] uint32_t binds = 0;
    for (uint32_t i = 0; i < FIXED_DRAW_SET_COUNT; ++i)
        binds += BindFixedDrawDescriptorSet(commandList, state.requested[i], i);

#ifdef __ANDROID__
    if (g_passQueryPools[g_frame] != nullptr)
    {
        auto& stats = g_fixedDescriptorFrameStats[g_frame];
        ++stats.preparedDraws;
        stats.lookups += lookups;
        stats.binds += binds;
    }
#endif
    return true;
}

// Rewrites the descriptor decorations of a SPIR-V module for the compact layout: sampler arrays
// move from binding 0 to binding 1 of their set, and the survey buffer moves from set 4 to the
// sampler set (set 3, binding 0). Returns the number of changed decorations.
static uint32_t RemapSpirvForCompactDescriptorSets(uint32_t* words, size_t wordCount)
{
    constexpr uint32_t OP_TYPE_SAMPLER = 26;
    constexpr uint32_t OP_TYPE_ARRAY = 28;
    constexpr uint32_t OP_TYPE_RUNTIME_ARRAY = 29;
    constexpr uint32_t OP_TYPE_POINTER = 32;
    constexpr uint32_t OP_VARIABLE = 59;
    constexpr uint32_t OP_DECORATE = 71;
    constexpr uint32_t DECORATION_BINDING = 33;
    constexpr uint32_t DECORATION_DESCRIPTOR_SET = 34;

    if (wordCount < 5 || words[0] != 0x07230203)
        return 0;

    const uint32_t bound = words[3];
    std::vector<uint8_t> isSampler(bound, 0);
    std::vector<uint32_t> arrayElement(bound, 0);
    std::vector<uint32_t> pointee(bound, 0);
    std::vector<uint32_t> variableType(bound, 0);
    std::vector<uint32_t*> setWord(bound, nullptr);
    std::vector<uint32_t*> bindingWord(bound, nullptr);

    for (size_t i = 5; i < wordCount;)
    {
        const uint32_t length = words[i] >> 16;
        const uint32_t opcode = words[i] & 0xFFFF;
        if (length == 0 || i + length > wordCount)
            return 0;

        const uint32_t* op = words + i;
        auto valid = [&](uint32_t id) { return id < bound; };

        if (opcode == OP_TYPE_SAMPLER && length >= 2 && valid(op[1]))
            isSampler[op[1]] = 1;
        else if ((opcode == OP_TYPE_ARRAY || opcode == OP_TYPE_RUNTIME_ARRAY) && length >= 3 && valid(op[1]))
            arrayElement[op[1]] = op[2];
        else if (opcode == OP_TYPE_POINTER && length >= 4 && valid(op[1]))
            pointee[op[1]] = op[3];
        else if (opcode == OP_VARIABLE && length >= 4 && valid(op[2]))
            variableType[op[2]] = op[1];
        else if (opcode == OP_DECORATE && length >= 4 && valid(op[1]))
        {
            if (op[2] == DECORATION_DESCRIPTOR_SET)
                setWord[op[1]] = words + i + 3;
            else if (op[2] == DECORATION_BINDING)
                bindingWord[op[1]] = words + i + 3;
        }

        i += length;
    }

    uint32_t changes = 0;
    for (uint32_t id = 0; id < bound; id++)
    {
        if (setWord[id] == nullptr || bindingWord[id] == nullptr)
            continue;

        uint32_t type = variableType[id] < bound ? pointee[variableType[id]] : 0;
        if (type < bound && arrayElement[type] != 0)
            type = arrayElement[type];

        const bool samplerVariable = type < bound && isSampler[type];
        if (samplerVariable && *bindingWord[id] == 0)
        {
            *bindingWord[id] = 1;
            ++changes;
        }
        else if (!samplerVariable && *setWord[id] == 4 && *bindingWord[id] == 0)
        {
            *setWord[id] = 3;
            ++changes;
        }
    }

    return changes;
}

// Renumbers the Location decorations of a vertex shader's input variables densely (in ascending
// order) and returns the old-to-new table (0xFF = not an input). False for other stages, or when
// the shader reads more inputs than fit.
static bool CompactVertexInputLocations(uint32_t* words, size_t wordCount, std::array<uint8_t, 32>& remap)
{
    constexpr uint32_t OP_ENTRY_POINT = 15;
    constexpr uint32_t OP_VARIABLE = 59;
    constexpr uint32_t OP_DECORATE = 71;
    constexpr uint32_t DECORATION_LOCATION = 30;
    constexpr uint32_t STORAGE_INPUT = 1;
    constexpr uint32_t EXECUTION_MODEL_VERTEX = 0;

    if (wordCount < 5 || words[0] != 0x07230203)
        return false;

    const uint32_t bound = words[3];
    std::vector<uint8_t> isInput(bound, 0);
    std::vector<uint32_t*> locationWord(bound, nullptr);
    bool vertex = false;

    for (size_t i = 5; i < wordCount;)
    {
        const uint32_t length = words[i] >> 16;
        const uint32_t opcode = words[i] & 0xFFFF;
        if (length == 0 || i + length > wordCount)
            return false;

        const uint32_t* op = words + i;
        if (opcode == OP_ENTRY_POINT && length >= 2 && op[1] == EXECUTION_MODEL_VERTEX)
            vertex = true;
        else if (opcode == OP_VARIABLE && length >= 4 && op[3] == STORAGE_INPUT && op[2] < bound)
            isInput[op[2]] = 1;
        else if (opcode == OP_DECORATE && length >= 4 && op[2] == DECORATION_LOCATION && op[1] < bound)
            locationWord[op[1]] = words + i + 3;

        i += length;
    }

    if (!vertex)
        return false;

    std::array<bool, 32> used{};
    for (uint32_t id = 0; id < bound; id++)
    {
        if (isInput[id] && locationWord[id] != nullptr && *locationWord[id] < used.size())
            used[*locationWord[id]] = true;
    }

    remap.fill(0xFF);
    uint32_t next = 0;
    for (uint32_t location = 0; location < used.size(); location++)
    {
        if (used[location])
            remap[location] = uint8_t(next++);
    }

    const uint32_t limit = g_capabilities.maxVertexInputAttributes != 0 ? g_capabilities.maxVertexInputAttributes : 16;
    if (next > limit)
        return false;

    for (uint32_t id = 0; id < bound; id++)
    {
        if (isInput[id] && locationWord[id] != nullptr && *locationWord[id] < used.size())
            *locationWord[id] = remap[*locationWord[id]];
    }

    return true;
}

// The generated vertex shaders write ClipDistance through one access chain. Remove the
// write and its interface variable together, so disabled clip planes take no output slot.
static bool StripSpirvClipDistance(std::vector<uint32_t>& words)
{
    constexpr uint32_t OP_NAME = 5;
    constexpr uint32_t OP_ENTRY_POINT = 15;
    constexpr uint32_t OP_CAPABILITY = 17;
    constexpr uint32_t OP_FUNCTION_CALL = 57;
    constexpr uint32_t OP_VARIABLE = 59;
    constexpr uint32_t OP_LOAD = 61;
    constexpr uint32_t OP_STORE = 62;
    constexpr uint32_t OP_COPY_MEMORY = 63;
    constexpr uint32_t OP_COPY_MEMORY_SIZED = 64;
    constexpr uint32_t OP_ACCESS_CHAIN = 65;
    constexpr uint32_t OP_IN_BOUNDS_ACCESS_CHAIN = 66;
    constexpr uint32_t OP_PTR_ACCESS_CHAIN = 67;
    constexpr uint32_t OP_IN_BOUNDS_PTR_ACCESS_CHAIN = 70;
    constexpr uint32_t OP_DECORATE = 71;
    constexpr uint32_t OP_COPY_OBJECT = 83;
    constexpr uint32_t OP_SELECT = 169;
    constexpr uint32_t OP_PHI = 245;
    constexpr uint32_t OP_RETURN_VALUE = 254;
    constexpr uint32_t CAPABILITY_SHADER = 1;
    constexpr uint32_t CAPABILITY_CLIP_DISTANCE = 32;
    constexpr uint32_t DECORATION_BUILTIN = 11;
    constexpr uint32_t DECORATION_LOCATION = 30;
    constexpr uint32_t BUILTIN_CLIP_DISTANCE = 3;
    constexpr uint32_t STORAGE_OUTPUT = 3;
    constexpr uint32_t EXECUTION_VERTEX = 0;

    if (words.size() < 5 || words[0] != 0x07230203)
        return false;

    bool simpleOutput = words[1] == 0x00010000;
    uint32_t* capability = nullptr;
    uint32_t* clipDecoration = nullptr;
    const uint32_t* entryPoint = nullptr;
    const uint32_t* clipVariable = nullptr;
    const uint32_t* clipAccess = nullptr;
    const uint32_t* clipStore = nullptr;
    uint32_t clipId = 0;
    uint32_t accessId = 0;
    const auto usesClipPointer = [&](uint32_t id)
    {
        return id != 0 && (id == clipId || id == accessId);
    };

    for (size_t i = 5; i < words.size();)
    {
        uint32_t* op = words.data() + i;
        const uint32_t length = op[0] >> 16;
        const uint32_t opcode = op[0] & 0xFFFF;
        if (length == 0 || length > words.size() - i)
            return false;

        if (opcode == OP_ENTRY_POINT && length >= 4)
        {
            simpleOutput &= entryPoint == nullptr && op[1] == EXECUTION_VERTEX;
            entryPoint = op;
        }
        else if (opcode == OP_CAPABILITY && length == 2 && op[1] == CAPABILITY_CLIP_DISTANCE)
        {
            simpleOutput &= capability == nullptr;
            capability = op;
        }
        else if (opcode == OP_DECORATE && length == 4 && op[2] == DECORATION_BUILTIN && op[3] == BUILTIN_CLIP_DISTANCE)
        {
            simpleOutput &= clipDecoration == nullptr;
            clipDecoration = op;
            clipId = op[1];
        }
        else if (clipId != 0)
        {
            if (opcode == OP_VARIABLE && length >= 4 && op[2] == clipId)
            {
                simpleOutput &= clipVariable == nullptr && length == 4 && op[3] == STORAGE_OUTPUT;
                clipVariable = op;
            }
            else if (opcode == OP_ACCESS_CHAIN && length == 5 && op[3] == clipId)
            {
                simpleOutput &= clipAccess == nullptr;
                clipAccess = op;
                accessId = op[2];
            }
            else if (opcode == OP_STORE && length >= 3 && usesClipPointer(op[1]))
            {
                simpleOutput &= clipStore == nullptr && op[1] == accessId;
                clipStore = op;
            }
            else if ((opcode == OP_LOAD || opcode == OP_ACCESS_CHAIN || opcode == OP_IN_BOUNDS_ACCESS_CHAIN ||
                         opcode == OP_PTR_ACCESS_CHAIN || opcode == OP_IN_BOUNDS_PTR_ACCESS_CHAIN) && length >= 4)
            {
                simpleOutput &= !usesClipPointer(op[3]);
            }
            else if ((opcode == OP_COPY_MEMORY || opcode == OP_COPY_MEMORY_SIZED) && length >= 3)
            {
                simpleOutput &= !usesClipPointer(op[1]) && !usesClipPointer(op[2]);
            }
            else if (opcode == OP_FUNCTION_CALL || opcode == OP_COPY_OBJECT || opcode == OP_SELECT ||
                     opcode == OP_PHI || opcode == OP_RETURN_VALUE)
            {
                simpleOutput &= std::none_of(op + 1, op + length, usesClipPointer);
            }
        }

        i += length;
    }

    if (clipDecoration == nullptr && capability == nullptr)
        return false;

    if (simpleOutput && capability && entryPoint && clipVariable && clipAccess && clipStore)
    {
        const auto* entryEnd = entryPoint + (entryPoint[0] >> 16);
        auto* interfaceStart = entryPoint + 3;
        // The null-terminated entry point name occupies a whole number of words.
        while (interfaceStart != entryEnd)
        {
            const bool endsName = std::memchr(interfaceStart, 0, sizeof(uint32_t)) != nullptr;
            ++interfaceStart;
            if (endsName)
                break;
        }
        const auto* clipOperand = std::find(interfaceStart, entryEnd, clipId);
        if (clipOperand != entryEnd && std::find(clipOperand + 1, entryEnd, clipId) == entryEnd)
        {
            size_t output = 5;
            for (size_t i = 5; i < words.size();)
            {
                const auto* op = words.data() + i;
                const uint32_t length = op[0] >> 16;
                const uint32_t opcode = op[0] & 0xFFFF;

                if (op == entryPoint)
                {
                    words[output++] = ((length - 1) << 16) | OP_ENTRY_POINT;
                    for (const auto* operand = op + 1; operand != op + length; operand++)
                    {
                        if (operand != clipOperand)
                            words[output++] = *operand;
                    }
                }
                else if (op != capability && op != clipVariable && op != clipAccess && op != clipStore &&
                    !((opcode == OP_NAME || opcode == OP_DECORATE) && length >= 2 && usesClipPointer(op[1])))
                {
                    std::memmove(words.data() + output, op, length * sizeof(uint32_t));
                    output += length;
                }

                i += length;
            }
            words.resize(output);
            return true;
        }
    }

    // Preserve the existing compatibility path for a different compiler output shape.
    if (clipDecoration != nullptr)
    {
        const uint32_t bound = words[3];
        std::vector<uint8_t> isOutput(bound, 0);
        std::vector<uint32_t> location(bound, UINT32_MAX);
        for (size_t i = 5; i < words.size();)
        {
            const uint32_t* op = words.data() + i;
            const uint32_t length = op[0] >> 16;
            const uint32_t opcode = op[0] & 0xFFFF;
            if (opcode == OP_VARIABLE && length >= 4 && op[3] == STORAGE_OUTPUT && op[2] < bound)
                isOutput[op[2]] = 1;
            else if (opcode == OP_DECORATE && length == 4 && op[2] == DECORATION_LOCATION && op[1] < bound)
                location[op[1]] = op[3];
            i += length;
        }

        uint32_t freeLocation = 0;
        for (uint32_t id = 0; id < bound; id++)
        {
            if (isOutput[id] && location[id] != UINT32_MAX)
                freeLocation = std::max(freeLocation, location[id] + 1);
        }

        clipDecoration[2] = DECORATION_LOCATION;
        clipDecoration[3] = freeLocation;
    }

    if (capability != nullptr)
        capability[1] = CAPABILITY_SHADER;

    return true;
}

// Compact fixed SPIR-V arrays and their constant accesses using the same guest slot masks
// as the descriptor writes. New constants keep other users of each original ID unchanged.
static bool CompactFixedDescriptorArrays(std::vector<uint32_t>& words)
{
    constexpr uint32_t OP_TYPE_INT = 21;
    constexpr uint32_t OP_TYPE_IMAGE = 25;
    constexpr uint32_t OP_TYPE_SAMPLER = 26;
    constexpr uint32_t OP_TYPE_ARRAY = 28;
    constexpr uint32_t OP_TYPE_POINTER = 32;
    constexpr uint32_t OP_CONSTANT = 43;
    constexpr uint32_t OP_CONSTANT_NULL = 46;
    constexpr uint32_t OP_VARIABLE = 59;
    constexpr uint32_t OP_LOAD = 61;
    constexpr uint32_t OP_ACCESS_CHAIN = 65;
    constexpr uint32_t OP_IN_BOUNDS_ACCESS_CHAIN = 66;
    constexpr uint32_t OP_PTR_ACCESS_CHAIN = 67;
    constexpr uint32_t OP_IN_BOUNDS_PTR_ACCESS_CHAIN = 70;
    constexpr uint32_t OP_COPY_OBJECT = 83;
    constexpr uint32_t OP_FUNCTION_CALL = 57;
    constexpr uint32_t UNIFORM_CONSTANT = 0;
    constexpr uint32_t DIM_2D = 1;
    constexpr uint32_t DIM_CUBE = 3;

    if (words.size() < 5 || words[0] != 0x07230203u || words[3] > words.size())
        return false;

    struct Resource
    {
        uint32_t slotMask = 0;
        bool descriptorVariable = false;
    };
    struct Constant
    {
        size_t end = 0;
        uint32_t type = 0;
        uint32_t value = 0;
    };
    struct NewConstant
    {
        size_t end;
        uint32_t type;
        uint32_t id;
        uint32_t value;
    };

    uint32_t nextId = words[3];
    std::vector<Resource> resources(nextId);
    std::vector<bool> int32Types(nextId, false);
    std::vector<Constant> constants(nextId);
    std::vector<NewConstant> newConstants;
    const auto isDescriptor = [&](uint32_t id)
    {
        return id < resources.size() && resources[id].descriptorVariable;
    };
    const auto remapConstant = [&](uint32_t id, uint32_t value)
    {
        const Constant& old = constants[id];
        if (old.value == value)
            return id;

        const auto replacement = std::find_if(newConstants.begin(), newConstants.end(), [&](const NewConstant& candidate)
        {
            return candidate.end == old.end && candidate.value == value;
        });
        if (replacement != newConstants.end())
            return replacement->id;

        newConstants.push_back({ old.end, old.type, nextId, value });
        return nextId++;
    };

    for (size_t at = 5; at < words.size();)
    {
        auto* op = words.data() + at;
        const uint32_t count = op[0] >> 16;
        if (count == 0 || count > words.size() - at)
            return false;
        const uint32_t opcode = op[0] & 0xFFFF;
        const auto valid = [&](uint32_t id) { return id < resources.size(); };

        switch (opcode)
        {
        case OP_TYPE_INT:
            if (count == 4 && valid(op[1]))
                int32Types[op[1]] = op[2] == 32;
            break;
        case OP_TYPE_IMAGE:
            if (count >= 9 && valid(op[1]) && op[7] == 1)
            {
                if (op[3] == DIM_2D && op[6] == 0)
                    resources[op[1]].slotMask = COMPACT_FIXED_TEXTURE_SLOTS[op[5] ?
                        TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY : TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D];
                else if (op[3] == DIM_CUBE && op[5] == 0 && op[6] == 0)
                    resources[op[1]].slotMask = COMPACT_FIXED_TEXTURE_SLOTS[TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE];
                else if (op[3] == DIM_2D && op[5] == 0 && op[6] != 0)
                    resources[op[1]].slotMask = 1; // Single-source MSAA resolve.
            }
            break;
        case OP_TYPE_SAMPLER:
            if (count == 2 && valid(op[1]))
                resources[op[1]].slotMask = COMPACT_FIXED_SAMPLER_SLOTS;
            break;
        case OP_CONSTANT:
        case OP_CONSTANT_NULL:
            if (count == (opcode == OP_CONSTANT ? 4u : 3u) && valid(op[1]) && valid(op[2]) && int32Types[op[1]])
                constants[op[2]] = { at + count, op[1], opcode == OP_CONSTANT ? op[3] : 0u };
            break;
        case OP_TYPE_ARRAY:
            if (count == 4 && valid(op[1]) && valid(op[2]) && resources[op[2]].slotMask != 0)
            {
                if (!valid(op[3]) || constants[op[3]].end == 0)
                    return false;
                const uint32_t length = constants[op[3]].value;
                if (length != 1 && length != FIXED_DESCRIPTOR_SLOTS)
                    return false;

                // Copy, resolve and ImGui declare single-element arrays with index zero.
                const uint32_t mask = length == 1 ? 1 : resources[op[2]].slotMask;
                resources[op[1]].slotMask = mask;
                op[3] = remapConstant(op[3], std::popcount(mask));
            }
            break;
        case OP_TYPE_POINTER:
            if (count == 4 && op[2] == UNIFORM_CONSTANT && valid(op[1]) && valid(op[3]))
                resources[op[1]].slotMask = resources[op[3]].slotMask;
            break;
        case OP_VARIABLE:
            if (count >= 4 && op[3] == UNIFORM_CONSTANT)
            {
                if (!valid(op[1]) || !valid(op[2]) || resources[op[1]].slotMask == 0)
                    return false;
                resources[op[2]] = { resources[op[1]].slotMask, true };
            }
            break;
        case OP_ACCESS_CHAIN:
        case OP_IN_BOUNDS_ACCESS_CHAIN:
            if (count >= 4 && isDescriptor(op[3]))
            {
                if (count != 5 || !valid(op[4]) || constants[op[4]].end == 0)
                    return false;
                const uint32_t guestSlot = constants[op[4]].value;
                const uint32_t mask = resources[op[3]].slotMask;
                if (guestSlot >= FIXED_DESCRIPTOR_SLOTS || (mask & (1u << guestSlot)) == 0)
                    return false;
                op[4] = remapConstant(op[4], std::popcount(mask & ((1u << guestSlot) - 1)));
            }
            break;
        case OP_LOAD:
        case OP_PTR_ACCESS_CHAIN:
        case OP_IN_BOUNDS_PTR_ACCESS_CHAIN:
        case OP_COPY_OBJECT:
            if (count >= 4 && isDescriptor(op[3]))
                return false;
            break;
        case OP_FUNCTION_CALL:
            if (count > 4 && std::any_of(op + 4, op + count, isDescriptor))
                return false;
            break;
        default:
            break;
        }

        at += count;
    }

    if (newConstants.empty())
        return true;

    std::stable_sort(newConstants.begin(), newConstants.end(), [](const NewConstant& a, const NewConstant& b)
    {
        return a.end < b.end;
    });
    std::vector<uint32_t> compacted;
    compacted.reserve(words.size() + 4 * newConstants.size());
    words[3] = nextId;
    size_t start = 0;
    for (const auto& constant : newConstants)
    {
        compacted.insert(compacted.end(), words.begin() + start, words.begin() + constant.end);
        compacted.insert(compacted.end(), { (4u << 16) | OP_CONSTANT, constant.type, constant.id, constant.value });
        start = constant.end;
    }
    compacted.insert(compacted.end(), words.begin() + start, words.end());
    words = std::move(compacted);
    return true;
}

static std::unique_ptr<RenderShader> CreateSpirvShader(const void* data, size_t size)
{
    if (!g_compactDescriptorSets && !g_compactVertexLocations && !g_stripClipDistance)
        return g_device->createShader(data, size, "shaderMain", RenderShaderFormat::SPIRV);

    std::vector<uint32_t> words((size + 3) / 4);
    memcpy(words.data(), data, size);

    if (g_compactDescriptorSets)
        RemapSpirvForCompactDescriptorSets(words.data(), words.size());

    if (g_compactFixedDescriptors && !CompactFixedDescriptorArrays(words))
    {
        LOG_ERROR("SPIR-V uses a texture or sampler that is outside the compact fixed descriptor layout.");
        return nullptr;
    }

    if (g_stripClipDistance)
        StripSpirvClipDistance(words);
    size = words.size() * sizeof(uint32_t);

    std::array<uint8_t, 32> remap{};
    const bool remapped = g_compactVertexLocations && CompactVertexInputLocations(words.data(), words.size(), remap);

    auto shader = g_device->createShader(words.data(), size, "shaderMain", RenderShaderFormat::SPIRV);
    if (remapped && shader != nullptr)
    {
        std::lock_guard lock(g_vertexLocationRemapMutex);
        g_vertexLocationRemaps[shader.get()] = remap;
    }

    return shader;
}

static void ForgetVertexLocationRemap(const RenderShader* shader)
{
    if (shader == nullptr || !g_compactVertexLocations)
        return;

    std::lock_guard lock(g_vertexLocationRemapMutex);
    g_vertexLocationRemaps.erase(shader);
}

static std::unique_ptr<GuestTexture> g_imFontTexture;
static std::unique_ptr<RenderPipelineLayout> g_imPipelineLayout;
static std::unique_ptr<RenderPipeline> g_imPipeline;
static std::unique_ptr<RenderPipeline> g_imAdditivePipeline;

struct DdsUploadBuffer
{
    std::unique_ptr<RenderBuffer> buffer;
    uint32_t capacity = 0;
    bool reused = false;
};

static constexpr uint32_t MAX_CACHED_DDS_UPLOAD_BYTES = 4 * 1024 * 1024;
static constexpr uint32_t MAX_TEXTURES_PER_DDS_BATCH = 64;
static std::mutex g_ddsUploadBufferMutex;
static DdsUploadBuffer g_cachedDdsUploadBuffer;

static DdsUploadBuffer AcquireDdsUploadBuffer(uint32_t size)
{
    if (g_batchTextureUploads && size <= MAX_CACHED_DDS_UPLOAD_BYTES)
    {
        std::lock_guard lock(g_ddsUploadBufferMutex);
        if (g_cachedDdsUploadBuffer.buffer != nullptr && size <= g_cachedDdsUploadBuffer.capacity)
        {
            auto upload = std::exchange(g_cachedDdsUploadBuffer, {});
            upload.reused = true;
            return upload;
        }
    }

    return { g_device->createBuffer(RenderBufferDesc::UploadBuffer(size)), size };
}

static void RecycleDdsUploadBuffer(DdsUploadBuffer upload)
{
    if (!g_batchTextureUploads || upload.capacity > MAX_CACHED_DDS_UPLOAD_BYTES)
        return;

    std::lock_guard lock(g_ddsUploadBufferMutex);
    if (g_cachedDdsUploadBuffer.buffer == nullptr || upload.capacity > g_cachedDdsUploadBuffer.capacity)
        std::swap(g_cachedDdsUploadBuffer, upload);
}

struct DdsCopyBatch
{
    DdsUploadBuffer upload;
    uint8_t* mappedMemory = nullptr;
    uint32_t usedBytes = 0;
    uint32_t texturesInBatch = 0;
    uint64_t batchId = 0;
    bool recording = false;
};

static DdsCopyBatch g_ddsCopyBatch;
static uint64_t g_nextCopyBatchId = 0;

// Caller must hold g_copyMutex.
static bool FlushDdsCopyBatchLocked()
{
    if (!g_ddsCopyBatch.recording)
        return true;

    auto batch = std::exchange(g_ddsCopyBatch, {});
    if (batch.mappedMemory != nullptr)
        batch.upload.buffer->unmap();

    g_copyCommandList->end();
    g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyCommandFence.get());
    const bool fenceCompleted = g_copyQueue->waitForCommandFence(g_copyCommandFence.get());

#ifdef __ANDROID__
    if (g_profileTextureUploads)
        g_textureUploadDiagnostics.AddSubmission();
#endif

    g_completedCopyBatchId.store(batch.batchId, std::memory_order_release);

    if (fenceCompleted)
        RecycleDdsUploadBuffer(std::move(batch.upload));

    return fenceCompleted;
}

void Video::FlushPendingUploads()
{
    std::lock_guard lock(g_copyMutex);
    FlushDdsCopyBatchLocked();
}

template<typename T>
static bool ExecuteCopyCommandList(const T& function)
{
    std::lock_guard lock(g_copyMutex);
    FlushDdsCopyBatchLocked();

    g_copyCommandList->begin();
    function();
    g_copyCommandList->end();
    g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyCommandFence.get());
    return g_copyQueue->waitForCommandFence(g_copyCommandFence.get());
}

static constexpr uint32_t PITCH_ALIGNMENT = 0x100;
static constexpr uint32_t PLACEMENT_ALIGNMENT = 0x200;

struct ImGuiPushConstants
{
    ImVec2 boundsMin{};
    ImVec2 boundsMax{};
    ImU32 gradientTopLeft{};
    ImU32 gradientTopRight{};
    ImU32 gradientBottomRight{};
    ImU32 gradientBottomLeft{};
    uint32_t shaderModifier{};
    uint32_t texture2DDescriptorIndex{};
    ImVec2 displaySize{};
    ImVec2 inverseDisplaySize{};
    ImVec2 origin{ 0.0f, 0.0f };
    ImVec2 scale{ 1.0f, 1.0f };
    ImVec2 proceduralOrigin{ 0.0f, 0.0f };
    float outline{};
};

extern ImFontBuilderIO g_fontBuilderIO;

static void CreateImGuiBackend()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // We scale and offset ImGui mouse events to adjust for viewports that differ from the window size.
    // With ImGui's default trickle behavior, we may scale the same event multiple times, so disable it.
    io.ConfigInputTrickleEventQueue = false;

#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    IM_DELETE(io.Fonts);
    io.Fonts = ImFontAtlasSnapshot::Load();
#else
    io.Fonts->AddFontDefault();
    ImFontAtlasSnapshot::GenerateGlyphRanges();
#endif

    InitImGuiUtils();
    OptionsMenu::Init();
    InstallerWizard::Init();
#ifdef __ANDROID__
    TouchControls::Init();
#endif

    ImGui_ImplSDL3_InitForOther(GameWindow::s_pWindow);

#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    g_imFontTexture = LoadTexture(
        decompressZstd(g_im_font_atlas_texture, g_im_font_atlas_texture_uncompressed_size).get(), g_im_font_atlas_texture_uncompressed_size);
#else
    io.Fonts->FontBuilderIO = &g_fontBuilderIO;
    io.Fonts->Build();

    g_imFontTexture = std::make_unique<GuestTexture>(ResourceType::Texture);

    uint8_t* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    RenderTextureDesc textureDesc;
    textureDesc.dimension = RenderTextureDimension::TEXTURE_2D;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = 1;
    textureDesc.mipLevels = 1;
    textureDesc.arraySize = 1;
    textureDesc.format = RenderFormat::R8G8B8A8_UNORM;

    g_imFontTexture->textureHolder = g_device->createTexture(textureDesc);
    g_imFontTexture->texture = g_imFontTexture->textureHolder.get();

    uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
    uint32_t slicePitch = (rowPitch * height + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
    auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
    uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

    if (rowPitch == (width * 4))
    {
        memcpy(mappedMemory, pixels, slicePitch);
    }
    else
    {
        for (size_t i = 0; i < height; i++)
        {
            memcpy(mappedMemory, pixels, width * 4);
            pixels += width * 4;
            mappedMemory += rowPitch;
        }
    }

    uploadBuffer->unmap();

    ExecuteCopyCommandList([&]
        {
            g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(g_imFontTexture->texture, RenderTextureLayout::COPY_DEST));

            g_copyCommandList->copyTextureRegion(
                RenderTextureCopyLocation::Subresource(g_imFontTexture->texture, 0),
                RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
        });

    g_imFontTexture->layout = RenderTextureLayout::COPY_DEST;

    RenderTextureViewDesc textureViewDesc;
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
    textureViewDesc.mipLevels = 1;
    g_imFontTexture->textureView = g_imFontTexture->texture->createTextureView(textureViewDesc);

    g_imFontTexture->descriptorIndex = g_textureDescriptorAllocator.allocate();
    SetTextureDescriptor(g_imFontTexture->descriptorIndex, g_imFontTexture->texture, RenderTextureLayout::SHADER_READ, g_imFontTexture->textureView.get());
#endif

    io.Fonts->SetTexID(g_imFontTexture.get());

    RenderPipelineLayoutBuilder pipelineLayoutBuilder;
    pipelineLayoutBuilder.begin(false, true);

    RenderDescriptorSetBuilder descriptorSetBuilder;
    descriptorSetBuilder.begin();
    descriptorSetBuilder.addTexture(0, g_textureDescriptorSize);
    descriptorSetBuilder.end(!g_useFixedDescriptors, g_useFixedDescriptors ? 0 : g_textureDescriptorSize);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    BuildSamplerDescriptorSet(descriptorSetBuilder);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    pipelineLayoutBuilder.addPushConstant(0, 2, sizeof(ImGuiPushConstants), RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);

    pipelineLayoutBuilder.end();
    g_imPipelineLayout = pipelineLayoutBuilder.create(g_device.get());

    auto vertexShader = CREATE_SHADER(imgui_vs);
    auto pixelShader = CREATE_SHADER(imgui_ps);
    if (vertexShader == nullptr || pixelShader == nullptr)
        return;

    RenderInputElement inputElements[3];
    inputElements[0] = RenderInputElement("POSITION", 0, 0, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, pos));
    inputElements[1] = RenderInputElement("TEXCOORD", 0, 1, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, uv));
    inputElements[2] = RenderInputElement("COLOR", 0, 2, RenderFormat::R8G8B8A8_UNORM, 0, offsetof(ImDrawVert, col));

    RenderInputSlot inputSlot(0, sizeof(ImDrawVert));

    RenderGraphicsPipelineDesc pipelineDesc;
    pipelineDesc.pipelineLayout = g_imPipelineLayout.get();
    pipelineDesc.vertexShader = vertexShader.get();
    pipelineDesc.pixelShader = pixelShader.get();
    pipelineDesc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
    pipelineDesc.renderTargetBlend[0] = RenderBlendDesc::AlphaBlend();
    pipelineDesc.renderTargetCount = 1;
    pipelineDesc.inputElements = inputElements;
    pipelineDesc.inputElementsCount = std::size(inputElements);
    pipelineDesc.inputSlots = &inputSlot;
    pipelineDesc.inputSlotsCount = 1;
    g_imPipeline = g_device->createGraphicsPipeline(pipelineDesc);
    CheckBuiltInPipeline(g_imPipeline.get(), "ImGui");

    pipelineDesc.renderTargetBlend[0].dstBlend = RenderBlend::ONE;
    g_imAdditivePipeline = g_device->createGraphicsPipeline(pipelineDesc);
    CheckBuiltInPipeline(g_imAdditivePipeline.get(), "ImGui additive");

#ifndef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    ImFontAtlasSnapshot snapshot;
    snapshot.Snap();

    FILE* file = fopen("im_font_atlas.bin", "wb");
    if (file)
    {
        fwrite(snapshot.data.data(), 1, snapshot.data.size(), file);
        fclose(file);
    }

    ddspp::Header header;
    ddspp::HeaderDXT10 headerDX10;
    ddspp::encode_header(ddspp::R8G8B8A8_UNORM, width, height, 1, ddspp::Texture2D, 1, 1, header, headerDX10);

    file = fopen("im_font_atlas.dds", "wb");
    if (file)
    {
        fwrite(&ddspp::DDS_MAGIC, 4, 1, file);
        fwrite(&header, sizeof(header), 1, file);
        fwrite(&headerDX10, sizeof(headerDX10), 1, file);
        fwrite(pixels, 4, width * height, file);
        fclose(file);
    }
#endif
}

#ifdef __ANDROID__
// Ported from UnleashedRecomp-Android: without a frame-rate vote, high refresh rate phones
// keep the game surface at 60 Hz, so FIFO presentation caps the game at 60 FPS whatever the
// FPS option says. Vote the FPS option on every new ANativeWindow and whenever the option
// changes. ANativeWindow_setFrameRate is API 30+, minSdk is 29, so it is resolved at runtime.
static void UpdateAndroidFrameRateVote(plume::RenderWindow window)
{
    static plume::RenderWindow s_votedWindow = nullptr;
    static int32_t s_votedFps = 0;
    if (window == nullptr || (window == s_votedWindow && Config::FPS.Value == s_votedFps))
        return;

    // Wait until the user confirms or cancels an in-progress FPS slider drag in OptionsMenu
    // so intermediate ticks do not spam ANativeWindow_setFrameRate or display mode switches.
    if (window == s_votedWindow && OptionsMenu::s_isVisible &&
        OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected)
    {
        return;
    }

    s_votedWindow = window;
    s_votedFps = Config::FPS.Value;

    // FPS_MAX means uncapped: vote high and let the platform pick the fastest mode.
    const float frameRate = (Config::FPS.Value >= FPS_MIN && Config::FPS.Value < FPS_MAX) ? float(Config::FPS.Value) : 240.0f;

    using SetFrameRateFn = int32_t (*)(ANativeWindow*, float, int8_t);
    static SetFrameRateFn s_setFrameRate = []() -> SetFrameRateFn
    {
        void* library = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL);
        return library != nullptr ? reinterpret_cast<SetFrameRateFn>(dlsym(library, "ANativeWindow_setFrameRate")) : nullptr;
    }();

    if (s_setFrameRate == nullptr)
    {
        LOG_WARNING("ANativeWindow_setFrameRate is unavailable (Android 10); using window display mode only.");
    }
    else
    {
        const int32_t result = s_setFrameRate(window, frameRate, 0 /* ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT */);
        LOGF("Android frame-rate vote: {} FPS (result {}).", frameRate, result);
    }

    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (env != nullptr && activity != nullptr)
    {
        jclass activityClass = env->GetObjectClass(activity);
        if (activityClass != nullptr)
        {
            jmethodID method = env->GetMethodID(activityClass, "setPreferredRefreshRate", "(F)V");
            if (method != nullptr)
                env->CallVoidMethod(activity, method, frameRate);
            if (env->ExceptionCheck())
                env->ExceptionClear();
            env->DeleteLocalRef(activityClass);
        }
        env->DeleteLocalRef(activity);
    }
}
#endif

static void CheckSwapChain()
{
    g_swapChain->setVsyncEnabled(Config::VSync);
    g_swapChainValid &= !g_swapChain->needsResize();

#ifdef __ANDROID__
    // Android replaces the ANativeWindow across background/foreground. The old VkSurfaceKHR
    // then references a dead window: swap chain recreation fails or presents never reach the
    // screen. Compare every frame so the change is caught even if lifecycle events were missed.
    plume::RenderWindow currentNativeWindow = GameWindow::GetAndroidNativeWindow();
    if (currentNativeWindow != g_swapChain->getWindow())
        g_swapChainValid = false;

    UpdateAndroidFrameRateVote(currentNativeWindow);
#endif

    if (!g_swapChainValid)
    {
#ifdef __ANDROID__
        const uint64_t nowMs = SDL_GetTicks();
        if (nowMs < g_androidSwapchainRetryAfterMs)
            goto skipResizeAttempt;

        // Backgrounded: no window to present into yet. Keep rendering offscreen and retry.
        if (currentNativeWindow == nullptr)
        {
            g_androidSwapchainRetryAfterMs = nowMs + 100;
            goto skipResizeAttempt;
        }

        if (currentNativeWindow != g_swapChain->getWindow() || g_androidSurfaceNeedsRecreate)
        {
            Video::WaitForGPU();
            LOGF("Recreating the Vulkan surface (native window {}).",
                currentNativeWindow != g_swapChain->getWindow() ? "changed" : "unchanged, previous surface lost");
            GameWindow::s_renderWindow = currentNativeWindow;

            // The swap chain images are destroyed with the surface; drop framebuffers first.
            g_backBuffer->framebuffers.clear();
            PurgeSurfaceVariantFramebuffers();

            if (!g_swapChain->recreateSurface(currentNativeWindow))
            {
                g_androidSwapchainRetryAfterMs = nowMs + 500;
                LOG_WARNING("Vulkan surface recreation failed; retrying shortly.");
                goto skipResizeAttempt;
            }

            g_androidSurfaceNeedsRecreate = false;
        }
#endif

        Video::WaitForGPU();
        g_backBuffer->framebuffers.clear();
        PurgeSurfaceVariantFramebuffers();
        g_swapChainValid = g_swapChain->resize();
        Video::s_needsResize = g_swapChainValid;

        while (g_renderSemaphores.size() < g_swapChain->getTextureCount())
            g_renderSemaphores.emplace_back(g_device->createCommandSemaphore());

#ifdef __ANDROID__
        if (!g_swapChainValid)
        {
            g_androidSwapchainRetryAfterMs = nowMs + 500;
            g_androidSurfaceNeedsRecreate = true;
            LOG_WARNING("Swap chain recreation failed; recreating the surface on the next attempt.");
        }
        else
        {
            LOGF("Swap chain (re)created: {}x{}, pre-rotation {} degrees, {} images (window {}x{}).",
                g_swapChain->getWidth(), g_swapChain->getHeight(), g_swapChain->getPreRotation(),
                g_swapChain->getTextureCount(), ANativeWindow_getWidth(currentNativeWindow),
                ANativeWindow_getHeight(currentNativeWindow));
        }
#endif
    }

#ifdef __ANDROID__
skipResizeAttempt:
#endif
    if (g_swapChainValid)
    {
#ifdef __ANDROID__
        // Idempotent after success; covers a deferred initial swapchain and retries a
        // marker removal that failed on a transient storage error.
        AndroidMarkVulkanStartupSuccessful();
#endif
        g_swapChainAcquireProfiler.Begin();
        g_swapChainValid = g_swapChain->acquireTexture(g_acquireSemaphores[g_frame].get(), &g_backBufferIndex);
        g_swapChainAcquireProfiler.End();
    }

    if (Video::s_needsResize)
        Video::ComputeViewportDimensions();

    g_backBuffer->width = Video::s_viewportWidth;
    g_backBuffer->height = Video::s_viewportHeight;
}

static void BeginCommandList()
{
#ifdef __ANDROID__
    auto& timing = g_renderThreadFrameTimings[g_frame];
    timing = {};
    if (g_isRenderThread && g_passQueryPools[g_frame] != nullptr)
        timing.beginCpuMs = AndroidFrameDiagnostics::CpuMilliseconds();
#endif

    g_renderTarget = g_backBuffer;
    g_depthStencil = nullptr;
    g_framebuffer = nullptr;

    g_pipelineState.renderTargetFormat = BACKBUFFER_FORMAT;
    g_pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;

    if (g_swapChainValid)
    {
        uint32_t width = Video::s_viewportWidth;
        uint32_t height = Video::s_viewportHeight;

        if (g_intermediaryBackBufferTextureWidth != width ||
            g_intermediaryBackBufferTextureHeight != height)
        {
#ifndef __ANDROID__
            if (g_intermediaryBackBufferTextureDescriptorIndex == NULL)
                g_intermediaryBackBufferTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();
#endif

            Video::WaitForGPU(); // Fine to wait for GPU, this'll only happen during resize.

            g_intermediaryBackBufferTexture = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));
#ifdef __ANDROID__
            g_gammaSourceDescriptorSet->setTexture(0, g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ);
#else
            SetTextureDescriptor(g_intermediaryBackBufferTextureDescriptorIndex, g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ);
#endif

            g_intermediaryBackBufferTextureWidth = width;
            g_intermediaryBackBufferTextureHeight = height;

            g_backBuffer->framebuffers.clear();
            PurgeSurfaceVariantFramebuffers();

            g_backBuffer->texture = g_intermediaryBackBufferTexture.get();
            g_backBuffer->layout = RenderTextureLayout::UNKNOWN;
            DiscardTexture(g_backBuffer, RenderTextureLayout::COLOR_WRITE);
        }

        g_backBuffer->texture = g_intermediaryBackBufferTexture.get();
    }
    else if (g_intermediaryBackBufferTexture != nullptr)
    {
        g_backBuffer->texture = g_intermediaryBackBufferTexture.get();
    }
    else
    {
        g_backBuffer->texture = g_backBuffer->textureHolder.get();
    }

    g_backBuffer->layout = RenderTextureLayout::UNKNOWN;

    for (size_t i = 0; i < 16; i++)
    {
        g_sharedConstants.texture2DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
        g_sharedConstants.texture2DArrayIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY;
        g_sharedConstants.textureCubeIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE;
    }

    memset(g_textures, 0, sizeof(g_textures));

    auto& commandList = g_commandLists[g_frame];

    commandList->begin();
    commandList->resetQueryPool(g_queryPools[g_frame].get(), 0, NUM_QUERIES);
    commandList->writeTimestamp(g_queryPools[g_frame].get(), 0);
#ifdef __ANDROID__
    g_imGuiFrameStats[g_frame] = {};
    g_drawConstantFrameStats[g_frame] = {};
    g_fixedDescriptorFrameStats[g_frame] = {};
    g_directCsmSliceSkips[g_frame] = 0;
    if (g_passQueryPools[g_frame] != nullptr)
    {
        commandList->resetQueryPool(g_passQueryPools[g_frame].get(), 0, NUM_PASS_QUERIES);
        commandList->setRenderPassTimestamps(g_passQueryPools[g_frame].get(), 0, NUM_PASS_QUERIES);
    }
#endif
    g_fixedDrawDescriptorState = {};
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    if (!g_useFixedDescriptors)
    {
        commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
        commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 1);
        commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 2);
        commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    }
    if (g_conditionalSurveyDescriptorSet != nullptr)
        commandList->setGraphicsDescriptorSet(g_conditionalSurveyDescriptorSet.get(), 4);

    g_readyForCommands = true;
    g_readyForCommands.notify_one();
}

#ifdef __ANDROID__
// Vulkan pipeline cache in <game root>/cache/pipeline_cache.bin: pipelines compiled in one session
// are cheap in the next, which shortens the precompilation at startup and the first use of an
// effect. Saved by a background thread 30 s after new pipelines appeared and when the app goes
// to the background. pipeline_cache.disabled turns it off. pipeline_cache.loading is written
// before the data reaches the driver and removed once the game has run 30 s with it; if it is
// still there at the next launch (a crash, possibly in the driver on that data), the cache is
// deleted instead of loaded.
static std::mutex g_pipelineCacheSaveMutex;
static std::condition_variable g_pipelineCacheSaveCondition;
static bool g_pipelineCacheSaveRequested = false;

static void SavePipelineCache(const char* reason)
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<uint8_t> data;
    if (!g_device->getPipelineCacheData(data))
    {
        LOGFN_WARNING("Pipeline cache: nothing to save ({}).", reason);
        return;
    }

    std::error_code error;
    const auto directory = os::android::GetDataRoot() / "cache";
    std::filesystem::create_directories(directory, error);
    const auto path = directory / "pipeline_cache.bin";
    const auto temporary = directory / "pipeline_cache.bin.tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
        if (!file)
        {
            LOGFN_WARNING("Pipeline cache: writing {} failed ({}).", temporary.string(), reason);
            return;
        }
    }
    std::filesystem::rename(temporary, path, error);
    if (error)
    {
        LOGFN_WARNING("Pipeline cache: renaming to {} failed: {} ({}).", path.string(), error.message(), reason);
        return;
    }

    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    LOGFN("Pipeline cache: saved {:.1f} MB in {:.0f} ms ({}).", data.size() / (1024.0 * 1024.0), ms, reason);
}

// The activity's pause and resume reach SDL only while the main thread pumps events, and the
// game then waits inside SDL_PumpEvents until the resume; logged so testers' logs show it.
static bool SDLCALL OnLifecycleEvent(void*, SDL_Event* event)
{
    static std::chrono::steady_clock::time_point s_backgroundSince;
    if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND)
    {
        s_backgroundSince = std::chrono::steady_clock::now();
        LOGN("App lifecycle: entering the background, the game is paused until it returns.");
    }
    else if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND)
    {
        LOGFN("App lifecycle: back in the foreground after {:.1f} s.",
            std::chrono::duration<double>(std::chrono::steady_clock::now() - s_backgroundSince).count());
    }

    return true;
}

static bool SDLCALL OnPipelineCacheEvent(void*, SDL_Event* event)
{
    if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND || event->type == SDL_EVENT_DID_ENTER_BACKGROUND)
    {
        std::lock_guard lock(g_pipelineCacheSaveMutex);
        g_pipelineCacheSaveRequested = true;
        g_pipelineCacheSaveCondition.notify_one();
    }

    return true;
}

static void PipelineCacheSaverThread(std::filesystem::path loadingMarker)
{
    pthread_setname_np(pthread_self(), "PipelineCacheIO");
    bool first = true;
    while (true)
    {
        bool requested;
        {
            std::unique_lock lock(g_pipelineCacheSaveMutex);
            g_pipelineCacheSaveCondition.wait_for(lock, std::chrono::seconds(30), [] { return g_pipelineCacheSaveRequested; });
            requested = g_pipelineCacheSaveRequested;
            g_pipelineCacheSaveRequested = false;
        }

        if (first && !loadingMarker.empty())
        {
            std::error_code error;
            std::filesystem::remove(loadingMarker, error);
        }
        first = false;

        if (g_pipelinesSinceCacheSave.exchange(0) != 0)
            SavePipelineCache(requested ? "going to the background" : "new pipelines");
    }
}

static void LoadPipelineCache()
{
    std::error_code error;
    const auto& root = os::android::GetDataRoot();
    if (std::filesystem::exists(root / "pipeline_cache.disabled", error))
    {
        LOG("Pipeline cache: disabled by pipeline_cache.disabled.");
        return;
    }

    const auto directory = root / "cache";
    const auto path = directory / "pipeline_cache.bin";
    const auto loadingMarker = directory / "pipeline_cache.loading";
    std::filesystem::create_directories(directory, error);

    std::vector<uint8_t> data;
    if (std::filesystem::exists(loadingMarker, error))
    {
        LOG_WARNING("Pipeline cache: the previous launch ended before the cache was confirmed; deleting it and starting empty.");
        std::filesystem::remove(path, error);
        std::filesystem::remove(loadingMarker, error);
    }
    else
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file)
        {
            data.resize(size_t(file.tellg()));
            file.seekg(0);
            file.read(reinterpret_cast<char*>(data.data()), std::streamsize(data.size()));
            if (!file)
                data.clear();
        }
    }

    if (!data.empty())
        std::ofstream(loadingMarker).close();

    const auto start = std::chrono::steady_clock::now();
    const bool accepted = g_device->setPipelineCacheData(data.empty() ? nullptr : data.data(), data.size());
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (data.empty())
        LOG("Pipeline cache: no saved cache, starting empty.");
    else if (accepted)
        LOGFN("Pipeline cache: loaded {:.1f} MB in {:.0f} ms.", data.size() / (1024.0 * 1024.0), ms);
    else
        LOGFN_WARNING("Pipeline cache: {:.1f} MB from another device or driver ignored, starting empty.", data.size() / (1024.0 * 1024.0));

    if (data.empty() || !accepted)
    {
        std::filesystem::remove(loadingMarker, error);
        std::thread(PipelineCacheSaverThread, std::filesystem::path()).detach();
    }
    else
    {
        std::thread(PipelineCacheSaverThread, loadingMarker).detach();
    }

    SDL_AddEventWatch(OnPipelineCacheEvent, nullptr);
}
#endif

template<typename T>
static void ApplyLowEndDefault(ConfigDef<T> &configDef, T newDefault, bool &changed)
{
    if (configDef.IsDefaultValue() && !configDef.IsLoadedFromConfig)
    {
        configDef = newDefault;
        changed = true;
    }
    
    configDef.DefaultValue = newDefault;
}

static void ApplyLowEndDefaults()
{
    bool changed = false;

    ApplyLowEndDefault(Config::AntiAliasing, EAntiAliasing::MSAA2x, changed);
    ApplyLowEndDefault(Config::ShadowResolution, EShadowResolution::x1024, changed);
    ApplyLowEndDefault(Config::ReflectionResolution, EReflectionResolution::Quarter, changed);
    ApplyLowEndDefault(Config::TransparencyAntiAliasing, false, changed);

    if (changed) 
    {
        Config::Save();
    }
}

using RenderInterfaceFunction = std::unique_ptr<RenderInterface>(void);

static bool TryCreateHostDevice(std::vector<RenderInterfaceFunction*>& interfaceFunctions, bool graphicsApiRetry)
{
#ifdef MARATHON_RECOMP_D3D12
    bool allowVulkanRedirection = true;

    if (graphicsApiRetry)
    {
        // If we are attempting to create again after a reboot due to a crash, swap the order.
        g_backend = (g_backend == Backend::VULKAN) ? Backend::D3D12 : Backend::VULKAN;

        // Don't allow redirection to Vulkan if we are retrying after a crash, 
        // so the user can at least boot the game with D3D12 if Vulkan fails to work.
        allowVulkanRedirection = false;
    }
#endif

    for (size_t i = 0; i < interfaceFunctions.size(); i++)
    {
        RenderInterfaceFunction* interfaceFunction = interfaceFunctions[i];

#ifdef MARATHON_RECOMP_D3D12
        // Wrap the device creation in __try/__except to survive from driver crashes.
        __try
#endif
        {
            g_interface = interfaceFunction();
            if (g_interface == nullptr)
            {
                continue;
            }

            g_device = g_interface->createDevice(Config::GraphicsDevice);
            if (g_device != nullptr)
            {
                const RenderDeviceDescription &deviceDescription = g_device->getDescription();
                
#if defined(MARATHON_RECOMP_D3D12)
                if (interfaceFunction == CreateD3D12Interface)
                {
                    if (allowVulkanRedirection)
                    {
                        bool redirectToVulkan = false;

                        // ...
                        // There used to be driver redirections here, but they are all free from Vulkan purgatory for now...
                        // ...

                        if (redirectToVulkan)
                        {
                            g_device.reset();
                            g_interface.reset();

                            // In case Vulkan fails to initialize, we will try D3D12 again afterwards, 
                            // just to get the game to boot. This only really happens in very old Intel GPU drivers.
                            if (g_backend != Backend::VULKAN)
                            {
                                interfaceFunctions.push_back(CreateD3D12Interface);
                                allowVulkanRedirection = false;
                            }

                            continue;
                        }
                    }
                }

                g_backend = (interfaceFunction == CreateVulkanInterface) ? Backend::VULKAN : Backend::D3D12;
#elif defined(MARATHON_RECOMP_METAL)
                g_backend = (interfaceFunction == CreateVulkanInterface) ? Backend::VULKAN : Backend::METAL;
#endif
                // Enable triangle strip workaround if we are on AMD, as there is a bug where
                // restart indices cause triangles to be culled incorrectly. Converting them to degenerate triangles fixes it.
                g_triangleStripWorkaround = (deviceDescription.vendor == RenderDeviceVendor::AMD);

                break;
            }
        }
#ifdef MARATHON_RECOMP_D3D12
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (graphicsApiRetry)
            {
                // If we were retrying, and this also failed, then we'll show the user neither of the graphics APIs succeeded.
                return false;
            }
            else
            {
                // If this is the first crash we ran into, reboot and try the other graphics API.
                os::process::StartProcess(os::process::GetExecutablePath(), { "--graphics-api-retry" });
                std::_Exit(0);
            }
        }
#endif
    }

    return true;
}

bool Video::CreateHostDevice(const char* sdlVideoDriver, bool graphicsApiRetry)
{
    for (uint32_t i = 0; i < 16; i++)
        g_inputSlots[i].index = i;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    GameWindow::Init(sdlVideoDriver);

#if defined(MARATHON_RECOMP_D3D12)
    g_backend = (DetectWine() || Config::GraphicsAPI == EGraphicsAPI::Vulkan) ? Backend::VULKAN : Backend::D3D12;
#elif defined(MARATHON_RECOMP_METAL)
    g_backend = Config::GraphicsAPI == EGraphicsAPI::Vulkan ? Backend::VULKAN : Backend::METAL;
#endif

    // Attempt to create the possible backends using a vector of function pointers. Whichever succeeds first will be the chosen API.
    std::vector<RenderInterfaceFunction*> interfaceFunctions;

#ifdef MARATHON_RECOMP_D3D12
    interfaceFunctions.push_back((g_backend == Backend::VULKAN) ? CreateVulkanInterface : CreateD3D12Interface);
    interfaceFunctions.push_back((g_backend == Backend::VULKAN) ? CreateD3D12Interface : CreateVulkanInterface);
#elif defined(MARATHON_RECOMP_METAL)
    interfaceFunctions.push_back((g_backend == Backend::VULKAN) ? CreateVulkanInterface : CreateMetalInterface);
    interfaceFunctions.push_back((g_backend == Backend::VULKAN) ? CreateMetalInterface : CreateVulkanInterface);
#else
    interfaceFunctions.push_back(CreateVulkanInterface);
#endif

    if (!TryCreateHostDevice(interfaceFunctions, graphicsApiRetry))
    {
        return false;
    }

    if (g_device == nullptr)
    {
        return false;
    }

#ifdef MARATHON_RECOMP_D3D12
    if (graphicsApiRetry)
    {
        // If we managed to create a device after retrying it in a reboot, remember the one we picked.
        Config::GraphicsAPI = g_backend == Backend::VULKAN ? EGraphicsAPI::Vulkan : EGraphicsAPI::D3D12;
    }
#endif

    g_capabilities = g_device->getCapabilities();

    {
        const RenderDeviceDescription& deviceInfo = g_device->getDescription();
        const uint32_t apiVersion = deviceInfo.apiVersion;
        LOGF("GPU: \"{}\" (vendor 0x{:X}), driver version 0x{:X}, Vulkan {}.{}.{}.",
            deviceInfo.name, uint32_t(deviceInfo.vendor), deviceInfo.driverVersion,
            (apiVersion >> 22) & 0x7F, (apiVersion >> 12) & 0x3FF, apiVersion & 0xFFF);

        if (g_backend == Backend::VULKAN)
        {
            const uint32_t loader = deviceInfo.loaderApiVersion;
            const uint32_t effective = deviceInfo.effectiveApiVersion;
            LOGF("Vulkan loader: {}.{}.{}, effective device API: {}.{}.{}, properties2: {}.",
                (loader >> 22) & 0x7F, (loader >> 12) & 0x3FF, loader & 0xFFF,
                (effective >> 22) & 0x7F, (effective >> 12) & 0x3FF, effective & 0xFFF,
                deviceInfo.usesKhrProperties2 ? "KHR" : "core");
        }

        if (!deviceInfo.driverName.empty() || !deviceInfo.driverInfo.empty())
            LOGF("GPU driver: \"{}\" \"{}\".", deviceInfo.driverName, deviceInfo.driverInfo);

        LOGF("GPU limits: vertexAttributes={} vertexBindings={} clipDistance={} fragmentStoresAndAtomics={} vertexPipelineStoresAndAtomics={} shaderInt64={} maxVertexOutputComponents={} maxFragmentInputComponents={}.",
            g_capabilities.maxVertexInputAttributes, g_capabilities.maxVertexInputBindings, g_capabilities.shaderClipDistance,
            g_capabilities.fragmentStoresAndAtomics, g_capabilities.vertexPipelineStoresAndAtomics, g_capabilities.shaderInt64,
            g_capabilities.maxVertexOutputComponents, g_capabilities.maxFragmentInputComponents);

        LOGF("GPU features: descriptorSets={} descriptorIndexing={} scalarBlockLayout={} bufferDeviceAddress={} BC={} ETC2={} mirrorClampToEdge={} presentWait={} triangleFan={} sampledImages={} samplers={} maxTexture={} VRAM={} MiB.",
            g_capabilities.maxBoundDescriptorSets, g_capabilities.descriptorIndexing, g_capabilities.scalarBlockLayout, g_capabilities.bufferDeviceAddress,
            g_capabilities.textureCompressionBC, g_capabilities.textureCompressionETC2, g_capabilities.samplerMirrorClampToEdge,
            g_capabilities.presentWait, g_capabilities.triangleFan, g_capabilities.maxSampledImageDescriptors,
            g_capabilities.maxSamplerDescriptors, g_capabilities.maxTextureSize, deviceInfo.dedicatedVideoMemory / (1024 * 1024));

        // What the device can do with the formats of the game's textures and render targets; a
        // missing letter (linear filtering of a float or depth format, blending) is a lead for
        // rendering differences between GPUs.
        static constexpr std::pair<RenderFormat, const char*> formats[] =
        {
            { RenderFormat::R8_UNORM, "R8" },
            { RenderFormat::R8G8B8A8_UNORM, "RGBA8" },
            { RenderFormat::B8G8R8A8_UNORM, "BGRA8" },
            { RenderFormat::R16G16_FLOAT, "RG16F" },
            { RenderFormat::R16G16B16A16_FLOAT, "RGBA16F" },
            { RenderFormat::R32_FLOAT, "R32F" },
            { RenderFormat::D32_FLOAT, "D32F" },
            { RenderFormat::D32_FLOAT_S8_UINT, "D32F_S8" },
            { RenderFormat::D24_UNORM_S8_UINT, "D24_S8" },
        };

        std::string formatText;
        for (const auto& [format, name] : formats)
        {
            const RenderFormatSupport support = g_device->getFormatSupport(format);
            formatText += fmt::format(" {}={}{}{}{}{}", name, support.sampled ? 'S' : '-', support.linearFilter ? 'L' : '-',
                support.renderTarget ? 'T' : '-', support.blend ? 'B' : '-', support.depthStencil ? 'D' : '-');
        }

        LOGF("GPU formats (S sampled, L linear filter, T render target, B blend, D depth-stencil):{}.", formatText);
    }

#ifdef __ANDROID__
    {
        // <game root>/descriptor_sets_compact.enabled forces the compact layout on GPUs that do
        // not need it (Adreno), to test it without a Mali device.
        std::error_code error;
        const bool forced = std::filesystem::exists(os::android::GetDataRoot() / "descriptor_sets_compact.enabled", error);
        const bool limited = g_capabilities.maxBoundDescriptorSets != 0 && g_capabilities.maxBoundDescriptorSets < 5;
        const bool forceFixed = std::filesystem::exists(os::android::GetDataRoot() / "fixed_descriptors.enabled", error);
        const bool needsBufferConstants = !g_capabilities.bufferDeviceAddress || !g_capabilities.scalarBlockLayout;
        const bool forceBufferConstants = std::filesystem::exists(os::android::GetDataRoot() / "buffer_constants.enabled", error);
        g_useBufferConstants = needsBufferConstants || forceBufferConstants;
        if (g_useBufferConstants)
        {
            const bool hasBufferVariants = g_shaderCacheEntryCount != 0 &&
                g_shaderCacheEntries[0].androidSpirvSizes[BUFFER_CONSTANTS_SHADER_VARIANT] != 0 &&
                g_shaderCacheEntries[0].androidSpirvSizes[BUFFER_CONSTANTS_SHADER_VARIANT + 1] != 0;
            if (!hasBufferVariants)
            {
                if (needsBufferConstants)
                {
                    LOG_ERROR("The Vulkan device needs buffer constants, but the shader cache lacks their variants.");
                    return false;
                }
                LOG_WARNING("Ignoring buffer_constants.enabled because the shader cache lacks buffer constant variants.");
                g_useBufferConstants = false;
            }
        }
        if (g_useBufferConstants)
            LOGF("GPU shader constants: buffer offsets ({}).", needsBufferConstants ? "Vulkan features" : "buffer_constants.enabled");

        const bool forceCompactFixed = std::filesystem::exists(os::android::GetDataRoot() / "compact_fixed_descriptors.enabled", error);
        const uint32_t fixedStorageAndColor = 2 + uint32_t(g_useBufferConstants);
        const bool legacyFixedExceedsLimits =
            g_capabilities.maxFixedSampledImageDescriptors < 3 * FIXED_DESCRIPTOR_SLOTS ||
            g_capabilities.maxFixedSamplerDescriptors < FIXED_DESCRIPTOR_SLOTS ||
            g_capabilities.maxFixedPerStageResources < 4 * FIXED_DESCRIPTOR_SLOTS + fixedStorageAndColor;

        g_useFixedDescriptors = g_backend == Backend::VULKAN && (forceFixed || forceCompactFixed || g_useBufferConstants || !g_capabilities.descriptorIndexing ||
            (g_capabilities.maxSampledImageDescriptors != 0 && g_capabilities.maxSampledImageDescriptors < 3 * TEXTURE_DESCRIPTOR_SIZE) ||
            (g_capabilities.maxSamplerDescriptors != 0 && g_capabilities.maxSamplerDescriptors < SAMPLER_DESCRIPTOR_SIZE));
        g_compactDescriptorSets = g_backend == Backend::VULKAN && (forced || limited || g_useFixedDescriptors);
        g_samplerSlotOffset = g_compactDescriptorSets ? 1 : 0;
        g_compactFixedDescriptors = g_useFixedDescriptors && (forceCompactFixed || legacyFixedExceedsLimits);
        LOGF("GPU fixed descriptor limits: sampledImages={} samplers={} stageResources={}.",
            g_capabilities.maxFixedSampledImageDescriptors, g_capabilities.maxFixedSamplerDescriptors,
            g_capabilities.maxFixedPerStageResources);

        // <game root>/vertex_locations_compact.enabled forces the renumbering on GPUs with enough
        // vertex attributes (Adreno) to test it without a Mali device.
        const bool forcedVertex = std::filesystem::exists(os::android::GetDataRoot() / "vertex_locations_compact.enabled", error);
        const bool limitedVertex = g_capabilities.maxVertexInputAttributes != 0 && g_capabilities.maxVertexInputAttributes < 20;
        g_compactVertexLocations = (g_backend == Backend::VULKAN) && (forcedVertex || limitedVertex);

        const auto& gameRoot = os::android::GetDataRoot();
        // The unpacked interface can use 19 locations plus vertex built-ins.
        const bool varyingLimit = g_capabilities.maxVertexOutputComponents < 80 ||
            g_capabilities.maxFragmentInputComponents < 76;
        const bool forceCompactInterface = std::filesystem::exists(gameRoot / "compact_shader_interfaces.enabled", error);
        const bool compactInterface = g_backend == Backend::VULKAN && (varyingLimit || forceCompactInterface);
        g_device->setShaderInterfaceCompaction(compactInterface);
        if (compactInterface)
            LOGF("Vulkan shader interface compaction enabled ({}).", forceCompactInterface ? "startup marker" : "varying limits");

        const bool stripRequested = std::filesystem::exists(gameRoot / "clip_distance_strip.enabled", error);
        const bool softwareRequested = !stripRequested &&
            std::filesystem::exists(gameRoot / "clip_distance_software.enabled", error);
#ifdef MARATHON_RECOMP_FORCE_CLIP_DISTANCE_STRIP
        constexpr bool stripByDefault = true;
#else
        constexpr bool stripByDefault = false;
#endif
        const bool needsClipFallback = !g_capabilities.shaderClipDistance;
        // Strip by default only when the GPU lacks hardware shaderClipDistance (e.g. Mali),
        // unless clip_distance_strip.enabled explicitly forces stripping on a capable GPU.
        const bool forceStrip = stripRequested || (stripByDefault && needsClipFallback);

        // The software variant uses TEXCOORD16 after the existing 18 interpolators.
        constexpr uint32_t requiredVaryingComponents = 19 * 4;
        const bool fitsUnpackedInterface = g_capabilities.maxVertexOutputComponents >= requiredVaryingComponents &&
            g_capabilities.maxFragmentInputComponents >= requiredVaryingComponents;
        const bool fitsCompactedInterface = compactInterface && g_capabilities.maxVertexOutputComponents >= 64 &&
            g_capabilities.maxFragmentInputComponents >= 64;
        const bool supportsSoftwareClip = g_backend == Backend::VULKAN && (fitsUnpackedInterface || fitsCompactedInterface);
        g_useSoftwareClip = supportsSoftwareClip && (softwareRequested || (!forceStrip && needsClipFallback));
        g_stripClipDistance = g_backend == Backend::VULKAN && (forceStrip || needsClipFallback || g_useSoftwareClip);

        if (softwareRequested && !supportsSoftwareClip)
            LOG_WARNING("Fragment-shader clipping unavailable on this device; ignoring clip_distance_software.enabled.");

        if (g_useSoftwareClip)
        {
            LOGF("User clip planes use fragment-shader clipping{}.", softwareRequested ? " (clip_distance_software.enabled)" : "");
        }
        else if (g_stripClipDistance)
        {
            LOGF_WARNING("ClipDistance removed from shaders ({}); user clip planes are disabled.",
                stripRequested ? "clip_distance_strip.enabled" : "shaderClipDistance unsupported");
        }
        else if (g_backend == Backend::VULKAN)
        {
            LOG("User clip planes use hardware shaderClipDistance.");
        }

        constexpr uint32_t armVendorId = 0x13B5;
        const bool disableStagedBuffers = std::filesystem::exists(os::android::GetDataRoot() / "staged_guest_buffers.disabled", error);
        g_stageGuestBuffers = g_backend == Backend::VULKAN && !disableStagedBuffers &&
            (uint32_t(g_device->getDescription().vendor) == armVendorId ||
             std::filesystem::exists(os::android::GetDataRoot() / "staged_guest_buffers.enabled", error));
        if (g_stageGuestBuffers)
            LOG("Guest vertex/index updates are ordered on the render queue.");
        else if (disableStagedBuffers && g_backend == Backend::VULKAN)
            LOG("Staged guest vertex/index updates disabled by staged_guest_buffers.disabled.");

        g_hasConditionalSurvey = g_backend != Backend::VULKAN || g_capabilities.fragmentStoresAndAtomics;
        if (!g_hasConditionalSurvey)
            LOG_WARNING("Conditional survey disabled: this device has no fragment shader atomics.");

        if (g_compactVertexLocations)
        {
            LOGF_WARNING("Compact vertex input locations ({}).",
                forcedVertex ? "forced by vertex_locations_compact.enabled" : fmt::format("the device allows only {} vertex attributes", g_capabilities.maxVertexInputAttributes));
        }

        if (g_compactDescriptorSets)
        {
            LOGF_WARNING("Compact descriptor layout: the conditional survey buffer shares the sampler set ({}).",
                forced ? "forced by descriptor_sets_compact.enabled" : fmt::format("the device allows only {} descriptor sets", g_capabilities.maxBoundDescriptorSets));
        }
    }

    {
        // Testing hook: <game root>/bc_fallback.enabled runs the BC texture fallback on GPUs that
        // support BC (Adreno), to check it without a Mali device. If the file contains "decode",
        // ETC2 is ignored too and textures are decoded to plain RGBA.
        std::error_code error;
        const std::filesystem::path markerPath = os::android::GetDataRoot() / "bc_fallback.enabled";
        if (g_capabilities.textureCompressionBC && std::filesystem::exists(markerPath, error))
        {
            g_capabilities.textureCompressionBC = false;

            std::ifstream marker(markerPath);
            std::string contents((std::istreambuf_iterator<char>(marker)), std::istreambuf_iterator<char>());
            if (contents.find("decode") != std::string::npos)
                g_capabilities.textureCompressionETC2 = false;

            LOG_WARNING("bc_fallback.enabled present: BC texture support disabled to test the fallback.");
        }
    }

    if (!g_capabilities.textureCompressionBC)
    {
        if (g_capabilities.textureCompressionETC2)
            LOG_WARNING("BC texture formats are unsupported by this device. Textures will be transcoded to ETC2/EAC on the CPU at load time.");
        else
            LOG_WARNING("BC texture formats are unsupported by this device. Textures will be decompressed on the CPU at load time, increasing memory usage.");
    }
#endif

    if (g_capabilities.maxSampledImageDescriptors != 0)
        g_textureDescriptorSize = uint32_t(std::min<size_t>(TEXTURE_DESCRIPTOR_SIZE, g_capabilities.maxSampledImageDescriptors));

    if (g_capabilities.maxSamplerDescriptors != 0)
        g_samplerDescriptorSize = uint32_t(std::min<size_t>(SAMPLER_DESCRIPTOR_SIZE, g_capabilities.maxSamplerDescriptors));

    if (g_textureDescriptorSize < TEXTURE_DESCRIPTOR_SIZE || g_samplerDescriptorSize < SAMPLER_DESCRIPTOR_SIZE)
    {
        LOGF_WARNING("Descriptor set sizes clamped to device limits: {} textures, {} samplers.",
            g_textureDescriptorSize, g_samplerDescriptorSize);
    }

    if (g_useFixedDescriptors)
    {
        const uint32_t textures2D = std::popcount(FixedTextureSlots(TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D));
        const uint32_t texturesArray = std::popcount(FixedTextureSlots(TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY));
        const uint32_t texturesCube = std::popcount(FixedTextureSlots(TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE));
        const uint32_t images = textures2D + texturesArray + texturesCube;
        const uint32_t samplers = g_compactFixedDescriptors ? std::popcount(COMPACT_FIXED_SAMPLER_SLOTS) : FIXED_DESCRIPTOR_SLOTS;
        const uint32_t resources = images + samplers + 2 + uint32_t(g_useBufferConstants);
        if (g_capabilities.maxBoundDescriptorSets < 4 ||
            g_capabilities.maxFixedSampledImageDescriptors < images ||
            g_capabilities.maxFixedSamplerDescriptors < samplers ||
            g_capabilities.maxFixedPerStageResources < resources)
        {
            LOG_ERROR("GPU limits do not fit the fixed Vulkan descriptor layouts.");
            return false;
        }
        g_textureDescriptorSize = textures2D;
        g_samplerDescriptorSize = samplers;
        LOGF("GPU descriptor mode: {} ({} 2D, {} array, {} cube, {} samplers).",
            g_compactFixedDescriptors ? "compact fixed" : "fixed", textures2D, texturesArray, texturesCube, samplers);
    }

    LoadEmbeddedResources();

    constexpr uint64_t LowEndMemoryLimit = 2048ULL * 1024ULL * 1024ULL;
    RenderDeviceDescription deviceDescription = g_device->getDescription();
    bool lowEndType = deviceDescription.type != RenderDeviceType::UNKNOWN && deviceDescription.type != RenderDeviceType::DISCRETE;
    bool lowEndMemory = deviceDescription.dedicatedVideoMemory < LowEndMemoryLimit;
    bool lowEndUMA = deviceDescription.type == RenderDeviceType::UNKNOWN && g_capabilities.uma;
    if (lowEndType || lowEndMemory || lowEndUMA)
    {
        // Switch to low end defaults if a non-discrete GPU was detected or a low amount of VRAM was detected.
        // Checking for UMA on D3D12 seems to be a reliable way to detect integrated GPUs.
        ApplyLowEndDefaults();
    }

    const RenderSampleCounts colourSampleCount = g_device->getSampleCountsSupported(RenderFormat::R16G16B16A16_FLOAT);
    g_depthStencilFormat = RenderFormat::D32_FLOAT_S8_UINT;
    RenderSampleCounts depthSampleCount = g_device->getSampleCountsSupported(g_depthStencilFormat);
#ifdef __ANDROID__
    if ((colourSampleCount & RenderSampleCount::COUNT_1) == 0)
    {
        LOGF_ERROR("Vulkan RGBA16F does not support a single-sample sampled color attachment (mask 0x{:X}).", colourSampleCount);
        return false;
    }

    std::error_code depthFormatMarkerError;
    const bool testPackedDepth = std::filesystem::exists(os::android::GetDataRoot() / "depth_d24s8.enabled", depthFormatMarkerError);
    const bool hasFloatDepth = (depthSampleCount & RenderSampleCount::COUNT_1) != 0;
    if (testPackedDepth || !hasFloatDepth)
    {
        const auto packedDepth = RenderFormat::D24_UNORM_S8_UINT;
        const RenderSampleCounts packedSampleCount = g_device->getSampleCountsSupported(packedDepth);
        if ((packedSampleCount & RenderSampleCount::COUNT_1) != 0)
        {
            g_depthStencilFormat = packedDepth;
            depthSampleCount = packedSampleCount;
            LOGF("Using D24_S8 depth/stencil: {}.", testPackedDepth ? "requested by depth_d24s8.enabled" : "D32F_S8 lacks sampled attachment support");
        }
        else if (!hasFloatDepth)
        {
            LOGF_ERROR("No sampled Vulkan depth/stencil attachment format: D32F_S8=0x{:X} D24_S8=0x{:X}.",
                depthSampleCount, packedSampleCount);
            return false;
        }
        else
        {
            LOG_WARNING("D24_S8 lacks sampled attachment support; ignoring depth_d24s8.enabled.");
        }
    }

    g_pointFilterR32 = !g_device->getFormatSupport(RenderFormat::R32_FLOAT).linearFilter;
    g_pointFilterDepthStencil = !g_device->getFormatSupport(g_depthStencilFormat).linearFilter;
#endif
    const RenderSampleCounts commonSampleCount = colourSampleCount & depthSampleCount;
#ifdef __ANDROID__
    LOGF("GPU sample counts: RGBA16F=0x{:X} {}=0x{:X} common=0x{:X}. Android surfaces use 1 sample.",
        colourSampleCount, g_depthStencilFormat == RenderFormat::D32_FLOAT_S8_UINT ? "D32F_S8" : "D24_S8",
        depthSampleCount, commonSampleCount);
#endif

    // Disable specific MSAA levels if they are not supported.
    if ((commonSampleCount & RenderSampleCount::COUNT_2) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA2x);
    if ((commonSampleCount & RenderSampleCount::COUNT_4) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA4x);
    if ((commonSampleCount & RenderSampleCount::COUNT_8) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA8x);

    // Set Anti-Aliasing to nearest supported level.
    Config::AntiAliasing.SnapToNearestAccessibleValue(false);

    g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);

    for (auto& commandList : g_commandLists)
        commandList = g_queue->createCommandList();

    for (auto& commandFence : g_commandFences)
        commandFence = g_device->createCommandFence();

    for (auto& queryPool : g_queryPools)
        queryPool = g_device->createQueryPool(NUM_QUERIES);

#ifdef __ANDROID__
    if (g_backend == Backend::VULKAN && os::android::perf::IsCaptureRequested())
    {
        for (auto& queryPool : g_passQueryPools)
        {
            queryPool = g_device->createQueryPool(NUM_PASS_QUERIES);
            if (queryPool != nullptr && queryPool->getCount() != NUM_PASS_QUERIES)
                queryPool.reset();
        }
    }
#endif

    g_waitCommandList = g_queue->createCommandList();
    g_waitCommandFence = g_device->createCommandFence();

    g_copyQueue = g_device->createCommandQueue(RenderCommandListType::COPY);
    g_copyCommandList = g_copyQueue->createCommandList();
    g_copyCommandFence = g_device->createCommandFence();

    if (g_backend == Backend::D3D12)
    {
        g_discardCommandList = g_queue->createCommandList();
        g_discardCommandFence = g_device->createCommandFence();
    }

    uint32_t bufferCount = 2;

    switch (Config::TripleBuffering)
    {
    case ETripleBuffering::Auto:
        switch (g_backend) {
        case Backend::VULKAN:
            // Defaulting to 3 is fine if presentWait as supported, as the maximum frame latency allowed is only 1.
            bufferCount = g_device->getCapabilities().presentWait ? 3 : 2;
            break;
        case Backend::D3D12:
            // Defaulting to 3 is fine on D3D12 thanks to flip discard model.
            bufferCount = 3;
            break;
        case Backend::METAL:
            bufferCount = 2;
            break;
        }

        break;
    case ETripleBuffering::On:
        bufferCount = 3;
        break;
    case ETripleBuffering::Off:
        bufferCount = 2;
        break;
    }

#ifdef __ANDROID__
    // Experiment (a launcher switch, off by default): three images under Auto, which asks for two
    // without presentWait (Pixel 7 Pro, SM7475). Compare acquire_ms and frame times in one scene.
    {
        std::error_code error;
        const bool tripleBuffering = Config::TripleBuffering == ETripleBuffering::Auto &&
            std::filesystem::exists(os::android::GetDataRoot() / "triple_buffering.enabled", error);
        if (tripleBuffering)
            bufferCount = 3;
        LOGF("Swap chain: requesting {} images (TripleBuffering {}{}, presentWait {}, MaxFrameLatency {}).", bufferCount,
            Config::TripleBuffering == ETripleBuffering::Auto ? "Auto" : Config::TripleBuffering == ETripleBuffering::On ? "On" : "Off",
            tripleBuffering ? ", triple_buffering.enabled" : "", g_device->getCapabilities().presentWait, uint32_t(Config::MaxFrameLatency));
    }
#endif

    RenderSwapChainDesc swapChainDesc;
    swapChainDesc.renderWindow = GameWindow::s_renderWindow;
    swapChainDesc.textureCount = bufferCount;
    swapChainDesc.format = BACKBUFFER_FORMAT;
    swapChainDesc.maxFrameLatency = Config::MaxFrameLatency;
    swapChainDesc.enablePresentWait = g_capabilities.presentWait;

#ifdef __ANDROID__
    std::error_code outputScaleError;
    if (std::filesystem::is_regular_file(os::android::GetDataRoot() / "reduced_output.enabled", outputScaleError))
        swapChainDesc.outputScalePercent = 75;
    LOGF("[graphics] Low graphics: {}, requested output: {}%, shadows: {}, reflections: {}, radial blur: {}, anisotropy: {}x.",
        Config::IsLowGraphicsEnabled(), swapChainDesc.outputScalePercent,
        static_cast<int>(Config::GetEffectiveShadowResolution()),
        Config::ReflectionResolution.GetValueLocalised(ELanguage::English, Config::GetEffectiveReflectionResolution()),
        Config::RadialBlur.GetValueLocalised(ELanguage::English, Config::GetEffectiveRadialBlur()),
        Config::GetEffectiveAnisotropicFiltering());
    LOGF("[graphics] CSM size override: {} (0 = automatic), two object shadow cascades: {}, short shadow distance: {}, bloom disabled: {}.",
        Config::GetShadowMapResolutionOverride(), Config::LimitObjectShadowCascades(),
        Config::UseShortShadowDistance(), Config::IsBloomDisabled());
#endif

    g_swapChain = g_queue->createSwapChain(swapChainDesc);
    g_swapChain->setVsyncEnabled(Config::VSync);

#ifdef __ANDROID__
    // Pre-rotate the swap chain so SurfaceFlinger does not rotate every frame with GPU composition.
    // Create <game root>/prerotation.disabled before launching to compare against the old path.
    {
        std::error_code error;
        const bool preRotationDisabled = std::filesystem::exists(os::android::GetDataRoot() / "prerotation.disabled", error);
        g_swapChain->setPreRotationEnabled(!preRotationDisabled);
        LOGF("Swap chain pre-rotation {}.", preRotationDisabled ? "disabled by prerotation.disabled" : "enabled");
    }

    // Tile-friendly render passes: clears become load-op clears, barriers get masks from the
    // texture layouts. On by default; render_pass_optimizations.disabled (a launcher switch)
    // restores the previous behaviour.
    {
        std::error_code error;
        const bool disabled = std::filesystem::exists(os::android::GetDataRoot() / "render_pass_optimizations.disabled", error);
        g_device->setRenderPassOptimizations(!disabled, !disabled);
        LOGF("Render pass optimizations {}.", disabled ? "disabled by render_pass_optimizations.disabled" : "enabled (clear on load, precise barriers)");

        g_batchImGui = !std::filesystem::exists(os::android::GetDataRoot() / "imgui_batching.disabled", error);
        LOGF("ImGui batching {}.", g_batchImGui ? "enabled" : "disabled by imgui_batching.disabled");

        g_optimizeDrawConstants = g_backend == Backend::VULKAN &&
            !std::filesystem::exists(os::android::GetDataRoot() / "constant_upload_optimizations.disabled", error);
        LOGF("Draw constant upload optimizations {}.", g_optimizeDrawConstants ? "enabled" : "disabled by constant_upload_optimizations.disabled");

        g_batchDrawState = g_backend == Backend::VULKAN &&
            !std::filesystem::exists(os::android::GetDataRoot() / "draw_state_batching.disabled", error);
        LOGF("Draw state batching {}.", g_batchDrawState ? "enabled" : "disabled by draw_state_batching.disabled");

        g_batchTextureUploads = g_backend == Backend::VULKAN &&
            !std::filesystem::exists(os::android::GetDataRoot() / "texture_upload_batching.disabled", error);
        g_batchMultiTextureUploads = g_batchTextureUploads &&
            !std::filesystem::exists(os::android::GetDataRoot() / "dds_multi_batch.disabled", error);
        g_profileTextureUploads = os::android::perf::IsCaptureRequested();
        LOGF("DDS upload batching {} (multi-texture loading arena {}).",
            g_batchTextureUploads ? "enabled" : "disabled by texture_upload_batching.disabled",
            g_batchMultiTextureUploads ? "enabled" : "disabled");

        g_capLoadingFps = !std::filesystem::exists(os::android::GetDataRoot() / "loading_fps_cap.disabled", error);
        LOGF("Loading screen FPS cap: {}.", g_capLoadingFps ? "60 FPS" : "disabled by loading_fps_cap.disabled");

        g_reuseFixedDrawDescriptors = g_useFixedDescriptors &&
            !std::filesystem::exists(os::android::GetDataRoot() / "fixed_draw_descriptor_reuse.disabled", error);
        if (g_useFixedDescriptors)
            LOGF("Fixed draw descriptor reuse {}.", g_reuseFixedDrawDescriptors ? "enabled" : "disabled by fixed_draw_descriptor_reuse.disabled");

        const bool depthCopyDiscardDisabled = std::filesystem::exists(os::android::GetDataRoot() / "depth_copy_discard.disabled", error);
        g_discardDepthCopyLoad = !disabled && !depthCopyDiscardDisabled;
        LOGF("Full depth copy discards previous depth: {}.", g_discardDepthCopyLoad);

        g_useDepthTransfer = std::filesystem::exists(os::android::GetDataRoot() / "depth_copy_transfer.enabled", error);
        if (g_useDepthTransfer)
            LOG("Depth-only Vulkan transfers enabled by depth_copy_transfer.enabled.");

        g_useColorTransfer = std::filesystem::exists(os::android::GetDataRoot() / "color_copy_transfer.enabled", error);
        if (g_useColorTransfer)
            LOG("Color Vulkan transfers enabled by color_copy_transfer.enabled.");

        g_mergeDepthCopyClears = g_backend == Backend::VULKAN && !disabled && !g_useDepthTransfer &&
            std::filesystem::exists(os::android::GetDataRoot() / "depth_copy_clear_merge.enabled", error);
        if (g_mergeDepthCopyClears)
            LOG("Merging full clears across independent depth copies.");

        g_directCsmSlice = !std::filesystem::exists(os::android::GetDataRoot() / "direct_csm_slice.disabled", error);
        LOGF("Direct CSM slice rendering {}.", g_directCsmSlice ? "enabled" : "disabled by direct_csm_slice.disabled");

        g_depthCopyTraceEnabled = std::filesystem::exists(os::android::GetDataRoot() / "depth_copy_trace.enabled", error);
        if (g_depthCopyTraceEnabled)
        {
            g_depthCopyTraceRemaining = MAX_DEPTH_COPY_TRACE_ENTRIES;
            LOGF("Tracing the first {} depth copy requests into array textures.", g_depthCopyTraceRemaining);
        }
    }
#endif

    g_swapChainValid = !g_swapChain->needsResize();

#ifdef __ANDROID__
    // A custom loader reaching dlopen/device creation is not sufficient: only a usable WSI
    // swapchain proves that Vulkan startup completed (see os/android/vulkan_driver_android.cpp).
    if (g_swapChainValid)
        AndroidMarkVulkanStartupSuccessful();
#endif

    for (auto& acquireSemaphore : g_acquireSemaphores)
        acquireSemaphore = g_device->createCommandSemaphore();

    g_renderSemaphores.resize(g_swapChain->getTextureCount());
    
    for (auto& renderSemaphore : g_renderSemaphores)
        renderSemaphore = g_device->createCommandSemaphore();

#ifdef __ANDROID__
    SDL_AddEventWatch(OnLifecycleEvent, nullptr);
    LoadPipelineCache();
#endif

    RenderPipelineLayoutBuilder pipelineLayoutBuilder;
    pipelineLayoutBuilder.begin(false, true);
    
    RenderDescriptorSetBuilder descriptorSetBuilder;
    descriptorSetBuilder.begin();
    descriptorSetBuilder.addTexture(0, g_textureDescriptorSize);
    descriptorSetBuilder.end(!g_useFixedDescriptors, g_useFixedDescriptors ? 0 : g_textureDescriptorSize);
    
    LOGF("Init: texture descriptor set ({} textures).", g_textureDescriptorSize);
    if (g_useFixedDescriptors)
    {
        const uint32_t layoutCount = g_compactFixedDescriptors ? TEXTURE_DESCRIPTOR_NULL_COUNT : 1;
        for (uint32_t i = 0; i < layoutCount; ++i)
        {
            auto& builder = g_fixedTextureSetBuilders[i];
            builder.begin();
            builder.addTexture(0, std::popcount(FixedTextureSlots(i)));
            builder.end();
        }
    }
    else
        g_textureDescriptorSet = descriptorSetBuilder.create(g_device.get());
    LOG("Init: placeholder textures.");
    
    for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
    {
        auto& texture = g_blankTextures[i];
        auto& textureView = g_blankTextureViews[i];

        RenderTextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.format = RenderFormat::R8_UNORM;

        RenderTextureViewDesc viewDesc;
        viewDesc.format = desc.format;
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO);
        viewDesc.mipLevels = 1;

        switch (i)
        {
        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D:
            desc.dimension = RenderTextureDimension::TEXTURE_2D;
            desc.arraySize = 1;
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
            break;

        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY:
            desc.dimension = RenderTextureDimension::TEXTURE_2D;
            desc.arraySize = 1;
#ifdef __ANDROID__
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
#else
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
#endif
            break;

        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE:
            desc.dimension = RenderTextureDimension::TEXTURE_2D;
            desc.arraySize = 6;
            desc.flags = RenderTextureFlag::CUBE;
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_CUBE;
            break;

        default:
            assert(false && "Unknown null descriptor dimension");
            break;
        }

        texture = g_device->createTexture(desc);
        textureView = texture->createTextureView(viewDesc);

        SetTextureDescriptor(i, texture.get(), RenderTextureLayout::SHADER_READ, textureView.get());
    }

    for (uint32_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; ++i)
    {
        pipelineLayoutBuilder.addDescriptorSet(g_useFixedDescriptors ?
            g_fixedTextureSetBuilders[g_compactFixedDescriptors ? i : 0] : descriptorSetBuilder);
    }
    
    BuildSamplerDescriptorSet(descriptorSetBuilder, g_useBufferConstants);
    
    LOGF("Init: sampler descriptor set ({} samplers).", g_samplerDescriptorSize);
    if (g_useFixedDescriptors)
    {
        BuildSamplerDescriptorSet(g_fixedSamplerSetBuilder);
        if (g_useBufferConstants)
            BuildSamplerDescriptorSet(g_fixedBufferSamplerSetBuilder, true);
    }
    else
        g_samplerDescriptorSet = descriptorSetBuilder.create(g_device.get());
    auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(&g_samplerDescs[0], sizeof(RenderSamplerDesc))];
    descriptorIndex = 1;
    sampler = g_device->createSampler(g_samplerDescs[0]);
    if (g_useFixedDescriptors)
        g_fixedSamplers.push_back(sampler.get());
    else
        g_samplerDescriptorSet->setSampler(g_samplerSlotOffset, sampler.get());

    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    LOG("Init: conditional survey buffer.");
    RenderBufferDesc conditionalSurveyBufferDesc;
    conditionalSurveyBufferDesc.size = CONDITIONAL_SURVEY_MAX * sizeof(uint32_t);
    conditionalSurveyBufferDesc.heapType = RenderHeapType::DEFAULT;
    conditionalSurveyBufferDesc.flags = RenderBufferFlag::STORAGE | RenderBufferFlag::UNORDERED_ACCESS;
    g_conditionalSurveyBuffer = g_device->createBuffer(conditionalSurveyBufferDesc);

    RenderBufferStructuredView conditionalSurveyStructuredView(sizeof(uint32_t));

    if (g_compactDescriptorSets)
    {
        if (!g_useFixedDescriptors)
            g_samplerDescriptorSet->setBuffer(0, g_conditionalSurveyBuffer.get(), 0, &conditionalSurveyStructuredView);
    }
    else
    {
        RenderDescriptorSetBuilder conditionalSurveyDescriptorSetBuilder;
        conditionalSurveyDescriptorSetBuilder.begin();
        conditionalSurveyDescriptorSetBuilder.addReadWriteStructuredBuffer(0);
        conditionalSurveyDescriptorSetBuilder.end();
        g_conditionalSurveyDescriptorSet = conditionalSurveyDescriptorSetBuilder.create(g_device.get());
        g_conditionalSurveyDescriptorSet->setBuffer(0, g_conditionalSurveyBuffer.get(), 0, &conditionalSurveyStructuredView);

        pipelineLayoutBuilder.addDescriptorSet(conditionalSurveyDescriptorSetBuilder);
    }

    if (g_backend != Backend::D3D12)
    {
        const uint32_t constantSize = g_useBufferConstants ? sizeof(g_drawConstantOffsets) : 24;
        pipelineLayoutBuilder.addPushConstant(0, 4, constantSize, RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);
    }
    else
    {
        pipelineLayoutBuilder.addRootDescriptor(0, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addRootDescriptor(1, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addRootDescriptor(2, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addPushConstant(3, 4, 4, RenderShaderStageFlag::PIXEL); // For copy/resolve shaders.
    }
    pipelineLayoutBuilder.end();
    
    LOG("Init: pipeline layout.");
    g_pipelineLayout = pipelineLayoutBuilder.create(g_device.get());

    LOG("Init: built-in shaders and pipelines.");
    g_copyShader = CREATE_SHADER(copy_vs);
    g_copyColorShader = CREATE_SHADER(copy_color_ps);
    auto copyDepthShader = CREATE_SHADER(copy_depth_ps);
    if (g_copyShader == nullptr || g_copyColorShader == nullptr || copyDepthShader == nullptr)
        return false;

    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = g_copyShader.get();
    desc.pixelShader = copyDepthShader.get();
    desc.depthFunction = RenderComparisonFunction::ALWAYS;
    desc.depthEnabled = true;
    desc.depthWriteEnabled = true;
    desc.depthTargetFormat = g_depthStencilFormat;
    g_copyDepthPipeline = g_device->createGraphicsPipeline(desc);
    CheckBuiltInPipeline(g_copyDepthPipeline.get(), "copy depth");

#ifndef __ANDROID__
    g_resolveMsaaColorShaders[0] = CREATE_SHADER(resolve_msaa_color_2x);
    g_resolveMsaaColorShaders[1] = CREATE_SHADER(resolve_msaa_color_4x);
    g_resolveMsaaColorShaders[2] = CREATE_SHADER(resolve_msaa_color_8x);
    if (std::any_of(std::begin(g_resolveMsaaColorShaders), std::end(g_resolveMsaaColorShaders),
        [](const auto& shader) { return shader == nullptr; }))
        return false;

    constexpr RenderSampleCounts resolveSampleCounts[] = {
        RenderSampleCount::COUNT_2, RenderSampleCount::COUNT_4, RenderSampleCount::COUNT_8
    };
    static_assert(std::size(resolveSampleCounts) == std::size(g_resolveMsaaDepthPipelines));
    for (size_t i = 0; i < std::size(g_resolveMsaaDepthPipelines); i++)
    {
        if ((depthSampleCount & resolveSampleCounts[i]) == 0)
            continue;

        std::unique_ptr<RenderShader> pixelShader;
        switch (i)
        {
        case 0:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_2x);
            break;
        case 1:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_4x);
            break;
        case 2:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_8x);
            break;
        }
        if (pixelShader == nullptr)
            return false;

        desc = {};
        desc.pipelineLayout = g_pipelineLayout.get();
        desc.vertexShader = g_copyShader.get();
        desc.pixelShader = pixelShader.get();
        desc.depthFunction = RenderComparisonFunction::ALWAYS;
        desc.depthEnabled = true;
        desc.depthWriteEnabled = true;
        desc.depthTargetFormat = g_depthStencilFormat;
        g_resolveMsaaDepthPipelines[i] = g_device->createGraphicsPipeline(desc);
        CheckBuiltInPipeline(g_resolveMsaaDepthPipelines[i].get(), "resolve MSAA depth");
    }
#endif

    g_enhancedBurnoutBlurVSShader = CREATE_CONSTANT_SHADER(enhanced_burnout_blur_vs);
    g_enhancedBurnoutBlurPSShader = CREATE_SHADER(enhanced_burnout_blur_ps);
    g_MoviePSShader = CREATE_SHADER(movie_ps);

    if (g_hasConditionalSurvey)
    {
        g_conditionalSurveyPSShader = std::make_unique<GuestShader>(ResourceType::PixelShader);
        g_conditionalSurveyPSShader->shader = CREATE_CONSTANT_SHADER(conditional_survey_ps);
    }

    CreateImGuiBackend();

#ifdef __ANDROID__
    RenderDescriptorSetBuilder gammaSourceBuilder;
    gammaSourceBuilder.begin();
    gammaSourceBuilder.addTexture(0, 1);
    gammaSourceBuilder.end();
    g_gammaSourceDescriptorSet = gammaSourceBuilder.create(g_device.get());
    if (g_gammaSourceDescriptorSet == nullptr || !g_gammaSourceDescriptorSet->isValid())
    {
        LOG_ERROR("Could not allocate the gamma pass texture descriptor set.");
        return false;
    }

    RenderPipelineLayoutBuilder gammaLayoutBuilder;
    gammaLayoutBuilder.begin();
    gammaLayoutBuilder.addDescriptorSet(gammaSourceBuilder);
    gammaLayoutBuilder.addPushConstant(0, 0, sizeof(GammaPassConstants), RenderShaderStageFlag::PIXEL);
    gammaLayoutBuilder.end();
    g_gammaPipelineLayout = gammaLayoutBuilder.create(g_device.get());

    if (g_gammaPipelineLayout == nullptr)
    {
        LOG_ERROR("Could not create the gamma pass pipeline layout.");
        return false;
    }

    auto gammaCorrectionShader = g_device->createShader(g_gamma_correction_ps_spirv,
        sizeof(g_gamma_correction_ps_spirv), "shaderMain", RenderShaderFormat::SPIRV);
    auto* gammaLayout = g_gammaPipelineLayout.get();
#else
    auto gammaCorrectionShader = CREATE_SHADER(gamma_correction_ps);
    auto* gammaLayout = g_pipelineLayout.get();
#endif
    if (gammaCorrectionShader == nullptr || (g_hasConditionalSurvey && g_conditionalSurveyPSShader->shader == nullptr))
        return false;

    desc = {};
    desc.pipelineLayout = gammaLayout;
    desc.vertexShader = g_copyShader.get();
    desc.pixelShader = gammaCorrectionShader.get();
    desc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
    desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
    desc.renderTargetCount = 1;
    g_gammaCorrectionPipeline = g_device->createGraphicsPipeline(desc);
    CheckBuiltInPipeline(g_gammaCorrectionPipeline.get(), "gamma correction");
#ifdef __ANDROID__
    const auto validPipeline = [](const auto& pipeline) { return pipeline && pipeline->isValid(); };
    if (!validPipeline(g_copyDepthPipeline) || !validPipeline(g_imPipeline) ||
        !validPipeline(g_imAdditivePipeline) || !validPipeline(g_gammaCorrectionPipeline))
    {
        LOG_ERROR("Required Vulkan pipelines failed; see the errors above.");
        return false;
    }
#endif

    // NOTE: We initially allocate this on host memory to make the installer work, even if the 4 GB memory allocation fails.
    g_backBufferHolder = std::make_unique<GuestSurface>(ResourceType::RenderTarget);

    g_backBuffer = g_backBufferHolder.get();
    g_backBuffer->width = 1280;
    g_backBuffer->height = 720;
    g_backBuffer->format = BACKBUFFER_FORMAT;
    g_backBuffer->textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(1, 1, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));

    LOG("Init: first swap chain check.");
    Video::ComputeViewportDimensions();
    CheckSwapChain();
    BeginCommandList();

    RenderTextureBarrier blankTextureBarriers[TEXTURE_DESCRIPTOR_NULL_COUNT];
    for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
        blankTextureBarriers[i] = RenderTextureBarrier(g_blankTextures[i].get(), RenderTextureLayout::SHADER_READ);

    g_commandLists[g_frame]->barriers(RenderBarrierStage::NONE, blankTextureBarriers, std::size(blankTextureBarriers));

    return true;
}

static uint32_t g_waitForGPUCount = 0;

void Video::WaitForGPU()
{
    g_waitForGPUCount++;

    FlushPendingUploads();

    // Wait for all queued frames to finish.
    for (size_t i = 0; i < NUM_FRAMES; i++)
    {
        if (g_commandListStates[i])
        {
            g_queue->waitForCommandFence(g_commandFences[i].get());
            g_commandListStates[i] = false;
        }
    }

    // Execute an empty command list and wait for it to end to guarantee that any remaining presentation has finished.
    g_waitCommandList->begin();
    g_waitCommandList->end();
    g_queue->executeCommandLists(g_waitCommandList.get(), g_waitCommandFence.get());
    g_queue->waitForCommandFence(g_waitCommandFence.get());
}

static uint32_t getSetAddress(uint32_t base, int index) {
    uint32_t entryOffset = index * 0xC;
    uint32_t entryAddress = base + entryOffset;
    uint32_t setAddress = entryAddress + sizeof(uint32_t);
    return setAddress;
}

static uint32_t CreateDevice(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, be<uint32_t>* a6)
{
    LOGF_WARNING("{:p} {:p} {:p} {:p} {:p} {:p}\n", reinterpret_cast<void*>(a1), reinterpret_cast<void*>(a2), reinterpret_cast<void*>(a3), reinterpret_cast<void*>(a4), reinterpret_cast<void*>(a5), reinterpret_cast<void*>(a6));
    
    g_xdbfTextureCache = std::unordered_map<uint16_t, GuestTexture*>();

    for (auto &achievement : g_xdbfWrapper.GetAchievements(XDBF_LANGUAGE_ENGLISH))
    {
        if (!achievement.pImageBuffer || !achievement.ImageBufferSize)
            continue;

        g_xdbfTextureCache[achievement.ID] =
            LoadTexture((uint8_t *)achievement.pImageBuffer, achievement.ImageBufferSize).release();
    }

    // Move backbuffer to guest memory.
    assert(!g_memory.IsInMemoryRange(g_backBuffer) && g_backBufferHolder != nullptr);
    g_backBuffer = g_userHeap.AllocPhysical<GuestSurface>(std::move(*g_backBufferHolder));

    // Check for stale reference. BeginCommandList() gets called before CreateDevice() which is where the assignment happens.
    if (g_renderTarget == g_backBufferHolder.get()) g_renderTarget = g_backBuffer;
    if (g_depthStencil == g_backBufferHolder.get()) g_depthStencil = g_backBuffer;

    // Free the host backbuffer.
    g_backBufferHolder = nullptr;

    auto device = g_userHeap.AllocPhysical<GuestDevice>();
    memset(device, 0, sizeof(*device));

    // Append render state functions to the end of guest function table.
    uint32_t functionOffsetUnimplemented = PPC_CODE_BASE + PPC_CODE_SIZE;
    g_memory.InsertFunction(functionOffsetUnimplemented, HostToGuestFunction<SetRenderStateUnimplemented>);
    
    uint32_t functionOffset = 0x82B79868;
    for (size_t i = 0; i < std::size(device->setRenderStateFunctions); i++) {
        device->setRenderStateFunctions[i] = functionOffsetUnimplemented;
    }

    // InsertFucntion doesn't work, so we have to do this manually in the end
    for (auto& [state, function] : g_setRenderStateFunctions)
    {
        auto funcOffset = getSetAddress(functionOffset, state/4);
        uint32_t addr = __builtin_bswap32(*(uint32_t*)g_memory.Translate(funcOffset));
        printf("state %d of %x is %x\n", state, funcOffset, addr);
        g_memory.InsertFunction(addr, function);
        device->setRenderStateFunctions[state / 4] = addr;
    }

    for (size_t i = 0; i < std::size(device->setSamplerStateFunctions); i++)
        device->setSamplerStateFunctions[i] = *reinterpret_cast<uint32_t*>(g_memory.Translate(0x82B79CFC + i * 0xC));

    device->viewport.width = 1280.0f;
    device->viewport.height = 720.0f;
    device->viewport.maxZ = 1.0f;

    *a6 = g_memory.MapVirtual(device);

    return 0;
}

static void DestructResource(GuestResource* resource)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::DestructResource;
    cmd.destructResource.resource = resource;
    EnqueueRenderCommand(cmd);
}

static void ReleaseResource(GuestResource* resource)
{
    resource->Release();

    if (resource->refCount == 0)
        DestructResource(resource);
}

static void ProcDestructResource(const RenderCommand& cmd)
{
    const auto& args = cmd.destructResource;
    g_tempResources[g_renderFrame].push_back(args.resource);
}

static RenderFormat GetTextureUploadFormat(const GuestTexture& texture)
{
    return texture.uploadFormat == RenderFormat::UNKNOWN ? texture.format : texture.uploadFormat;
}

static uint32_t ComputeTexturePitch(uint32_t width, RenderFormat format)
{
    const uint32_t block = RenderFormatBlockWidth(format);
    const uint32_t rowBytes = ((width + block - 1) / block) * RenderFormatSize(format);
    return (rowBytes + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
}

static uint32_t ComputeTextureRows(uint32_t height, RenderFormat format)
{
    const uint32_t block = RenderFormatBlockWidth(format);
    return (height + block - 1) / block;
}

static void LockTextureRect(GuestTexture* texture, uint32_t, GuestLockedRect* lockedRect) 
{
    const RenderFormat format = GetTextureUploadFormat(*texture);
    const uint32_t pitch = ComputeTexturePitch(texture->width, format);
    const uint32_t slicePitch = pitch * ComputeTextureRows(texture->height, format);

    if (texture->mappedMemory == nullptr)
        texture->mappedMemory = g_userHeap.AllocPhysical(slicePitch, 0x10);

    lockedRect->pitch = pitch;
    lockedRect->bits = g_memory.MapVirtual(texture->mappedMemory);
}

static void UnlockTextureRect(GuestTexture* texture)
{
    assert(std::this_thread::get_id() == g_presentThreadId);

    if (texture->mappedMemory == nullptr)
        return;

    const RenderFormat format = GetTextureUploadFormat(*texture);
    const uint32_t byteCount = ComputeTexturePitch(texture->width, format) * ComputeTextureRows(texture->height, format);
    auto snapshot = std::make_unique_for_overwrite<uint8_t[]>(byteCount);
    memcpy(snapshot.get(), texture->mappedMemory, byteCount);

    RenderCommand cmd;
    cmd.type = RenderCommandType::UnlockTextureRect;
    cmd.unlockTextureRect.texture = texture;
    cmd.unlockTextureRect.data = snapshot.get();
    if (EnqueueRenderCommand(cmd))
        snapshot.release();
}

static void ProcUnlockTextureRect(const RenderCommand& cmd)
{
    const auto& args = cmd.unlockTextureRect;

    const std::unique_ptr<uint8_t[]> snapshot(args.data);
    auto& texture = *args.texture;

    const uint32_t pitch = ComputeTexturePitch(texture.width, texture.format);
    const uint32_t slicePitch = pitch * ComputeTextureRows(texture.height, texture.format);
    std::unique_ptr<RenderBuffer> largeUpload;
    UploadAllocation allocation;
    if (slicePitch <= UploadBuffer::SIZE)
    {
        allocation = g_uploadAllocators[g_frame].allocate(slicePitch, PLACEMENT_ALIGNMENT);
    }
    else
    {
        largeUpload = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
        auto* bytes = largeUpload != nullptr ? static_cast<uint8_t*>(largeUpload->map()) : nullptr;
        if (bytes == nullptr)
        {
            LOG_ERROR("Could not allocate the staging buffer for a dynamic texture.");
            return;
        }
        allocation = { largeUpload.get(), 0, bytes, 0 };
    }

#ifdef __ANDROID__
    const RenderFormat sourceFormat = GetTextureUploadFormat(texture);
    if (sourceFormat != texture.format)
    {
        const uint32_t sourcePitch = ComputeTexturePitch(texture.width, sourceFormat);
        const uint32_t blockBytes = RenderFormatSize(sourceFormat);
        auto decode = sourceFormat == RenderFormat::BC1_UNORM ? bcdec_bc1 : bcdec_bc3;
        for (uint32_t y = 0; y < texture.height; y += 4)
        {
            for (uint32_t x = 0; x < texture.width; x += 4)
            {
                alignas(16) uint8_t pixels[4][16];
                decode(snapshot.get() + (y / 4) * sourcePitch + (x / 4) * blockBytes, pixels, sizeof(pixels[0]));
                for (uint32_t row = 0; row < std::min(4u, texture.height - y); ++row)
                {
                    memcpy(allocation.memory + (y + row) * pitch + x * 4, pixels[row],
                        std::min(4u, texture.width - x) * 4);
                }
            }
        }
    }
    else
#endif
    {
        memcpy(allocation.memory, snapshot.get(), slicePitch);
    }

    auto& commandList = g_commandLists[g_frame];
    if (GetLayoutRef(&texture) == RenderTextureLayout::COPY_DEST)
    {
        // Multiple writes to the same image can be queued without a draw in between.
        commandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));
    }
    else
    {
        AddBarrier(&texture, RenderTextureLayout::COPY_DEST);
        FlushBarriers();
    }

    if (largeUpload)
    {
        largeUpload->unmap();
        g_tempBuffers[g_frame].push_back(std::move(largeUpload));
    }

    const uint32_t rowWidth = pitch / RenderFormatSize(texture.format) * RenderFormatBlockWidth(texture.format);
    commandList->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(texture.texture, 0),
        RenderTextureCopyLocation::PlacedFootprint(allocation.buffer, texture.format, texture.width, texture.height, 1, rowWidth, allocation.offset));

    // The game can draw with a texture that was already bound when it was updated.
    AddBarrier(&texture, RenderTextureLayout::SHADER_READ);
    FlushBarriers();
}

static void* LockBuffer(GuestBuffer* buffer, uint32_t flags)
{
    buffer->lockedReadOnly = (flags & 0x10) != 0;

    if (buffer->mappedMemory == nullptr)
        buffer->mappedMemory = g_userHeap.AllocPhysical(buffer->dataSize, 0x10);

    return buffer->mappedMemory;
}

static void* LockVertexBuffer(GuestBuffer* buffer, uint32_t, uint32_t, uint32_t flags)
{
    return LockBuffer(buffer, flags);
}

static std::atomic<uint32_t> g_bufferUploadCount = 0;

template<typename T>
static void UnlockBuffer(GuestBuffer* buffer)
{
    if (buffer->lockedReadOnly || buffer->dataSize == 0)
        return;

    const bool direct = g_capabilities.gpuUploadHeap && !g_stageGuestBuffers;
    std::unique_ptr<RenderBuffer> upload;
    if (!direct)
        upload = g_device->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));

    auto* target = direct ? buffer->buffer.get() : upload.get();
    auto* dst = target != nullptr ? static_cast<T*>(target->map()) : nullptr;
    if (dst == nullptr)
    {
        LOG_ERROR("Failed to map guest buffer upload.");
        return;
    }

    auto* src = static_cast<const T*>(buffer->mappedMemory);
    for (size_t i = 0; i < buffer->dataSize / sizeof(T); ++i)
        dst[i] = ByteSwap(src[i]);
    const size_t packedBytes = buffer->dataSize / sizeof(T) * sizeof(T);
    if (packedBytes < buffer->dataSize)
    {
        memcpy(reinterpret_cast<uint8_t*>(dst) + packedBytes,
            reinterpret_cast<const uint8_t*>(src) + packedBytes, buffer->dataSize - packedBytes);
    }
    target->unmap();

    if (g_stageGuestBuffers)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::UnlockBuffer;
        cmd.unlockBuffer.buffer = buffer;
        cmd.unlockBuffer.upload = upload.get();
        if (EnqueueRenderCommand(cmd))
            upload.release();
    }
    else if (upload)
    {
        ExecuteCopyCommandList([&]
        {
            g_copyCommandList->copyBufferRegion(buffer->buffer->at(0), upload->at(0), buffer->dataSize);
        });
    }

    g_bufferUploadCount++;
}

static void ProcUnlockBuffer(const RenderCommand& cmd)
{
    const auto& args = cmd.unlockBuffer;
    std::unique_ptr<RenderBuffer> upload(args.upload);
    auto& buffer = *args.buffer;
    auto& commandList = g_commandLists[g_frame];

    commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(buffer.buffer.get(), RenderBufferAccess::WRITE));
    commandList->copyBufferRegion(buffer.buffer->at(0), upload->at(0), buffer.dataSize);
    commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(buffer.buffer.get(), RenderBufferAccess::READ));
    g_tempBuffers[g_frame].emplace_back(std::move(upload));
}

static void UnlockVertexBuffer(GuestBuffer* buffer)
{
    UnlockBuffer<uint32_t>(buffer);
}

static void GetVertexBufferDesc(GuestBuffer* buffer, GuestBufferDesc* desc) 
{
    desc->size = buffer->dataSize;
}

static void* LockIndexBuffer(GuestBuffer* buffer, uint32_t, uint32_t, uint32_t flags) 
{
    return LockBuffer(buffer, flags);
}

static void UnlockIndexBuffer(GuestBuffer* buffer) 
{
    if (buffer->guestFormat == D3DFMT_INDEX32)
        UnlockBuffer<uint32_t>(buffer);
    else
        UnlockBuffer<uint16_t>(buffer);
}

static void GetIndexBufferDesc(GuestBuffer* buffer, GuestBufferDesc* desc)
{
    desc->format = buffer->guestFormat;
    desc->size = buffer->dataSize;
}

static void GetSurfaceDesc(GuestSurface* surface, GuestSurfaceDesc* desc) 
{
    if (surface->width == 0 && surface->height == 0) {
        LOGF_WARNING("{:p} {:d} {:d} \n", reinterpret_cast<void*>(desc), surface->width, surface->height);
        __builtin_trap();
    }
    desc->width = surface->width;
    desc->height = surface->height;
    desc->format = surface->guestFormat;
    desc->type = 4; // D3DRTYPE_SURFACE
    // desc->multiSampleType = 0;
    if (surface->sampleCount == RenderSampleCount::COUNT_1) {
        desc->multiSampleType = 0;
    } else if (surface->sampleCount == RenderSampleCount::COUNT_2) {
        desc->multiSampleType = 1;
    } else {
        desc->multiSampleType = 2;
    }
    desc->multiSampleQuality = 0;
    desc->usage = 0;
}

static void GetVertexDeclaration(GuestVertexDeclaration* vertexDeclaration, GuestVertexElement* vertexElements, be<uint32_t>* count) 
{
    memcpy(vertexElements, vertexDeclaration->vertexElements.get(), vertexDeclaration->vertexElementCount * sizeof(GuestVertexElement));
    *count = vertexDeclaration->vertexElementCount;
}

static uint32_t HashVertexDeclaration(uint32_t vertexDeclaration) 
{
    // Vertex declarations are cached on host side, so the pointer itself can be used.
    return vertexDeclaration;
}

static const char *DeviceTypeName(RenderDeviceType type)
{
    switch (type) 
    {
    case RenderDeviceType::INTEGRATED:
        return "Integrated";
    case RenderDeviceType::DISCRETE:
        return "Discrete";
    case RenderDeviceType::VIRTUAL:
        return "Virtual";
    case RenderDeviceType::CPU:
        return "CPU";
    default:
        return "Unknown";
    }
}

static void DrawProfiler()
{
    const auto toggleProfiler = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F1] != 0;

    if (!g_profilerWasToggled && toggleProfiler)
    {
        g_profilerVisible = !g_profilerVisible;

        GameWindow::SetFullscreenCursorVisibility(App::s_isInit ? g_profilerVisible : true);
    }

    g_profilerWasToggled = toggleProfiler;

    if (!g_profilerVisible)
        return;

    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-RodinPro-DB.otf");
    const auto defaultFontScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    ImGui::PushFont(font);

#define IMGUI_GENERIC_ROW(name, value, ...) \
    do                                      \
    {                                       \
        ImGui::TableNextColumn();           \
        ImGui::Text(name);                  \
        ImGui::TableNextColumn();           \
        ImGui::Text(value, __VA_ARGS__);    \
    }                                       \
    while (false);

    if (ImGui::Begin("Profiler", &g_profilerVisible))
    {
        g_applicationValues[g_profilerValueIndex] = App::s_deltaTime * 1000.0;

        const auto applicationAvg = std::accumulate(g_applicationValues, g_applicationValues + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
        const auto gpuFrameAvg = g_gpuFrameProfiler.UpdateAndReturnAverage();
        const auto presentAvg = g_presentProfiler.UpdateAndReturnAverage();
        const auto frameFenceAvg = g_frameFenceProfiler.UpdateAndReturnAverage();
        const auto presentWaitAvg = g_presentWaitProfiler.UpdateAndReturnAverage();
        const auto swapChainAcquireAvg = g_swapChainAcquireProfiler.UpdateAndReturnAverage();

        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImPlot::BeginPlot("Frame Time"))
            {
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 20.0);
                ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_None);
                ImPlot::PlotLine<double>("Application", g_applicationValues, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::PlotLine<double>("GPU Frame", g_gpuFrameProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::PlotLine<double>("Present", g_presentProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::PlotLine<double>("Present Wait", g_presentWaitProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::PlotLine<double>("Frame Fence", g_frameFenceProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::PlotLine<double>("Swap Chain Acquire", g_swapChainAcquireProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
                ImPlot::EndPlot();
            }

            g_profilerValueIndex = (g_profilerValueIndex + 1) % PROFILER_VALUE_COUNT;

            if (ImGui::BeginTable("Performance", 5))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Current Time");
                ImGui::TableSetupColumn("Average Time");
                ImGui::TableSetupColumn("Current FPS");
                ImGui::TableSetupColumn("Average FPS");
                ImGui::TableHeadersRow();

                auto drawPerfRow = [](const char* name, double ms, double msAvg, bool showFPS = false, double fps = 0, double fpsAvg = 0)
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", name);
                    ImGui::TableNextColumn();
                    ImGui::Text("%g ms", ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%g ms", msAvg);
                    ImGui::TableNextColumn();

                    if (showFPS)
                        ImGui::Text("%g FPS", fps);

                    ImGui::TableNextColumn();

                    if (showFPS)
                        ImGui::Text("%g FPS", fpsAvg);
                };

                // -------- Name ---------------- Current Time --------------------------- Average Time -------- Current FPS ----------------------------- Average FPS ---------- //
                drawPerfRow("Application",        App::s_deltaTime * 1000.0,               applicationAvg, true, 1.0 / App::s_deltaTime,                   1000.0 / applicationAvg);
                drawPerfRow("GPU Frame",          g_gpuFrameProfiler.value.load(),         gpuFrameAvg,    true, 1000.0 / g_gpuFrameProfiler.value.load(), 1000.0 / gpuFrameAvg   );
                drawPerfRow("Present",            g_presentProfiler.value.load(),          presentAvg,     true, 1000.0 / g_presentProfiler.value.load(),  1000.0 / presentAvg    );
                drawPerfRow("Present Wait",       g_presentWaitProfiler.value.load(),      presentWaitAvg                                                                         );
                drawPerfRow("Frame Fence",        g_frameFenceProfiler.value.load(),       frameFenceAvg                                                                          );
                drawPerfRow("Swap Chain Acquire", g_swapChainAcquireProfiler.value.load(), swapChainAcquireAvg                                                                    );

                ImGui::EndTable();
            }

            ImGui::Separator();

            ImGui::Checkbox("Show FPS", &Config::ShowFPS.Value);
        }

        if (g_userHeap.heap && g_userHeap.physicalHeap)
        {
            if (ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::lock_guard heapLock(g_userHeap.mutex);
                O1HeapDiagnostics heapDiagnostics = o1heapGetDiagnostics(g_userHeap.heap);

                std::lock_guard physicalHeapLock(g_userHeap.physicalMutex);
                O1HeapDiagnostics physicalHeapDiagnostics = o1heapGetDiagnostics(g_userHeap.physicalHeap);

                if (ImGui::BeginTable("Memory", 2))
                {
                    IMGUI_GENERIC_ROW("Heap Allocated", "%.2f MiB", double(heapDiagnostics.allocated / (1024.0 * 1024.0)));
                    IMGUI_GENERIC_ROW("Physical Heap Allocated", "%.2f MiB", double(physicalHeapDiagnostics.allocated / (1024.0 * 1024.0)));

                    ImGui::EndTable();
                }
            }
        }

        if (ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* backend{};

            switch (g_backend)
            {
                case Backend::VULKAN:
                    backend = "Vulkan";
                    break;

                case Backend::D3D12:
                    backend = "D3D12";
                    break;

                case Backend::METAL:
                    backend = "Metal";
                    break;

                default:
                    backend = "Unknown";
                    break;
            }

            if (ImGui::BeginTable("GPU", 2))
            {
                IMGUI_GENERIC_ROW("API", "%s", backend);

                if (const auto pSDLVideoDriver = SDL_GetCurrentVideoDriver())
                    IMGUI_GENERIC_ROW("SDL Video Driver", "%s", pSDLVideoDriver);

                IMGUI_GENERIC_ROW("Device", "%s", g_device->getDescription().name.c_str());
                IMGUI_GENERIC_ROW("Device Type", "%s", DeviceTypeName(g_device->getDescription().type));
                IMGUI_GENERIC_ROW("VRAM", "%.2f MiB", double(g_device->getDescription().dedicatedVideoMemory) / (1024.0 * 1024.0));
                IMGUI_GENERIC_ROW("GPU Waits", "%u", g_waitForGPUCount);
                IMGUI_GENERIC_ROW("Buffer Uploads", "%u", g_bufferUploadCount.load());

                IMGUI_GENERIC_ROW("Resolution", "%ux%u (%ux%u)",
                    Video::s_viewportWidth, Video::s_viewportHeight,
                    uint32_t(round(Video::s_viewportWidth * Config::ResolutionScale)),
                    uint32_t(round(Video::s_viewportHeight * Config::ResolutionScale)));

                ImGui::EndTable();
            }

            ImGui::Separator();

#ifdef ASYNC_PSO_DEBUG
            if (ImGui::TreeNode("Async PSO"))
            {
                ImGui::Indent();

                if (ImGui::BeginTable("Async PSO", 2))
                {
                    IMGUI_GENERIC_ROW("Pipelines created in render thread", "%u", g_pipelinesCreatedInRenderThread.load());
                    IMGUI_GENERIC_ROW("Pipelines created asynchronously", "%u", g_pipelinesCreatedAsynchronously.load());
                    IMGUI_GENERIC_ROW("Pipelines dropped", "%u", g_pipelinesDropped.load());
                    IMGUI_GENERIC_ROW("Pipelines currently compiling", "%u", g_pipelinesCurrentlyCompiling.load());
                    IMGUI_GENERIC_ROW("Compiling pipeline tasks", "%u", g_compilingPipelineTaskCount.load());
                    IMGUI_GENERIC_ROW("Pending pipeline tasks", "%u", g_pendingPipelineTaskCount.load());

                    ImGui::EndTable();
                }

                ImGui::Separator();
                
                if (ImGui::TreeNode("Pipeline States"))
                {
                    ImGui::Indent();

                    std::lock_guard lock(g_debugMutex);
                    ImGui::TextUnformatted(g_pipelineDebugText.c_str());

                    ImGui::Unindent();
                    ImGui::TreePop();
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }
#endif

            if (ImGui::TreeNode("Devices"))
            {
                ImGui::Indent();

                if (ImGui::BeginTable("Devices", 2))
                {
                    auto deviceIndex = 0;

                    for (const auto& deviceName : g_interface->getDeviceNames())
                    {
                        ImGui::TableNextColumn();
                        ImGui::Text("Device #%d", deviceIndex++);
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", deviceName.c_str());
                        ImGui::SameLine();
                    }

                    ImGui::EndTable();
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Features"))
            {
                ImGui::Indent();

                if (ImGui::BeginTable("Features", 2))
                {
                    IMGUI_GENERIC_ROW("Dynamic Depth Bias", "%s", g_capabilities.dynamicDepthBias ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("GPU Upload Heap", "%s", g_capabilities.gpuUploadHeap ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("Hardware Resolve Modes", "%s", g_capabilities.resolveModes ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("Present Wait", "%s", g_capabilities.presentWait ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("Triangle Fan", "%s", g_capabilities.triangleFan ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("Triangle Strip Workaround", "%s", g_triangleStripWorkaround ? "Enabled" : "Disabled");
                    IMGUI_GENERIC_ROW("UMA", "%s", g_capabilities.uma ? "Supported" : "Unsupported");
                    IMGUI_GENERIC_ROW("BC Textures", "%s", g_capabilities.textureCompressionBC
                        ? "Supported" : (g_capabilities.textureCompressionETC2 ? "ETC2 Transcode" : "CPU Decode"));

                    ImGui::EndTable();
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }
        }
    }

#undef IMGUI_GENERIC_ROW

    ImGui::End();
    ImGui::PopFont();

    font->Scale = defaultFontScale;
}

static void DrawFPS()
{
    if (!Config::ShowFPS)
        return;

    double time = ImGui::GetTime();
    static double updateTime = time;
    static double fps = 0;
    static double totalDeltaTime = 0.0;
    static uint32_t totalDeltaCount = 0;

    totalDeltaTime += g_presentProfiler.value.load();
    totalDeltaCount++;

    if (time - updateTime >= 1.0f)
    {
        fps = 1000.0 / std::max(totalDeltaTime / double(totalDeltaCount), 1.0);
        updateTime = time;
        totalDeltaTime = 0.0;
        totalDeltaCount = 0;
    }

    auto drawList = ImGui::GetBackgroundDrawList();

    auto fmt = fmt::format("FPS: {:.2f}", fps);
    auto font = ImFontAtlasSnapshot::GetFont("FOT-RodinPro-DB.otf");
    auto fontSize = Scale(10);
    auto textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, fmt.c_str());

    ImVec2 min = { Scale(40), Scale(30) };
    ImVec2 max = { min.x + std::max(Scale(75), textSize.x + Scale(10)), min.y + Scale(15) };
    ImVec2 textPos = { min.x + Scale(2), CENTRE_TEXT_VERT(min, max, textSize) + Scale(0.2f) };

    drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 200));
    drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, fmt.c_str());
}

#ifdef __ANDROID__
// Performance overlay (overlay.enabled): frame and GPU times from the renderer's profilers, the
// rest sampled by os::android::overlay. Replaces the plain FPS counter while enabled.
static void DrawAndroidOverlay()
{
    static double updateTime = ImGui::GetTime();
    static double frameSum = 0.0, gpuSum = 0.0, frameWorst = 0.0;
    static uint32_t frameCount = 0;
    static double fps = 0.0, frameMs = 0.0, gpuMs = 0.0, worstMs = 0.0;
    static float history[120]{};
    static size_t historyIndex = 0;

    const double frame = g_presentProfiler.value.load();
    frameSum += frame;
    gpuSum += g_gpuFrameProfiler.value.load();
    frameWorst = std::max(frameWorst, frame);
    frameCount++;
    history[historyIndex] = float(frame);
    historyIndex = (historyIndex + 1) % std::size(history);

    const double time = ImGui::GetTime();
    if (time - updateTime >= 0.5 && frameCount != 0)
    {
        frameMs = frameSum / frameCount;
        fps = 1000.0 / std::max(frameMs, 0.001);
        gpuMs = gpuSum / frameCount;
        worstMs = frameWorst;
        frameSum = gpuSum = frameWorst = 0.0;
        frameCount = 0;
        updateTime = time;
    }

    const auto stats = os::android::overlay::GetStats();
    const auto& device = g_device->getDescription();
    std::vector<std::string> lines;

    const int32_t cap = Config::FPS;
    lines.push_back(fmt::format("FPS {:.1f}   {:.1f} ms (worst {:.1f})   cap {}", fps, frameMs, worstMs,
        cap >= FPS_MAX ? std::string("off") : std::to_string(cap)));

    std::string gpu = fmt::format("GPU {:.1f} ms (~{:.0f}% of frame)", gpuMs, frameMs > 0.0 ? std::min(100.0, 100.0 * gpuMs / frameMs) : 0.0);
    if (stats.gpuBusyPercent >= 0)
        gpu += fmt::format("   busy {}%", stats.gpuBusyPercent);
    lines.push_back(gpu);

    if (stats.valid)
    {
        std::string cpu = "CPU MHz";
        for (int i = 0; i < stats.cpuCount; i++)
        {
            if (i > 0 && stats.capacity[i] != stats.capacity[i - 1])
                cpu += " |";
            cpu += fmt::format(" {}", stats.curMHz[i]);
        }
        lines.push_back(cpu);

        const int guest = int(os::android::overlay::Role::Guest);
        const int render = int(os::android::overlay::Role::Render);
        lines.push_back(fmt::format("Game {:.0f}% c{}   Render {:.0f}% c{}   Process {:.1f} cores",
            100.0f * stats.threadLoad[guest], stats.threadCore[guest],
            100.0f * stats.threadLoad[render], stats.threadCore[render], stats.processCores));
    }

    const uint32_t api = device.apiVersion;
    lines.push_back(fmt::format("{}   {}   Vulkan {}.{}.{}", device.name,
        device.driverInfo.empty() ? fmt::format("0x{:X}", device.driverVersion) : device.driverInfo,
        (api >> 22) & 0x7F, (api >> 12) & 0x3FF, api & 0xFFF));

    std::string system = fmt::format("{}x{}   scale {:.2f}", g_swapChain->getWidth(), g_swapChain->getHeight(), float(Config::ResolutionScale));
    if (stats.valid)
    {
        if (stats.thermalStatus >= 0)
            system += fmt::format("   thermal {}", stats.thermalStatus);
        if (stats.thermalHeadroom >= 0.0f)
            system += fmt::format(" (headroom {:.2f})", stats.thermalHeadroom);
        if (stats.batteryCelsius > -100.0f)
            system += fmt::format("   battery {:.1f} C", stats.batteryCelsius);
        system += fmt::format("   RAM {:.0f} MB", stats.residentMB);
    }
    system += fmt::format("   ADPF {}", os::android::hint::GetStatus());
    lines.push_back(system);

    auto drawList = ImGui::GetBackgroundDrawList();
    auto font = ImFontAtlasSnapshot::GetFont("FOT-RodinPro-DB.otf");
    const float fontSize = Scale(9);
    const float lineHeight = fontSize + Scale(2);
    const float padding = Scale(4);
    const float graphHeight = Scale(22);

    float width = Scale(160);
    for (const auto& line : lines)
        width = std::max(width, font->CalcTextSizeA(fontSize, FLT_MAX, 0, line.c_str()).x);

    const ImVec2 min = { Scale(40), Scale(30) };
    const ImVec2 max = { min.x + width + padding * 2, min.y + padding * 3 + lineHeight * lines.size() + graphHeight };
    drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 170));

    float y = min.y + padding;
    for (const auto& line : lines)
    {
        drawList->AddText(font, fontSize, { min.x + padding, y }, IM_COL32_WHITE, line.c_str());
        y += lineHeight;
    }

    // Frame time graph: the last 120 frames, scaled to at least 33.3 ms, with a 16.7 ms line.
    const ImVec2 graphMin = { min.x + padding, y + padding };
    const ImVec2 graphMax = { max.x - padding, graphMin.y + graphHeight };
    float graphScale = 33.3f;
    for (float value : history)
        graphScale = std::max(graphScale, value);
    const float referenceY = graphMax.y - (16.7f / graphScale) * graphHeight;
    drawList->AddLine({ graphMin.x, referenceY }, { graphMax.x, referenceY }, IM_COL32(255, 255, 255, 70));

    ImVec2 points[std::size(history)];
    for (size_t i = 0; i < std::size(history); i++)
    {
        const float value = history[(historyIndex + i) % std::size(history)];
        points[i] = { graphMin.x + (graphMax.x - graphMin.x) * float(i) / float(std::size(history) - 1),
            graphMax.y - std::min(value / graphScale, 1.0f) * graphHeight };
    }
    drawList->AddPolyline(points, int(std::size(history)), IM_COL32(120, 230, 120, 255), 0, Scale(1));
}
#endif

static void DrawImGui()
{
    ImGui_ImplSDL3_NewFrame();

    auto& io = ImGui::GetIO();
    io.DisplaySize = { float(Video::s_viewportWidth), float(Video::s_viewportHeight) };

    // ImGui doesn't know that we center the screen for specific aspect ratio
    // settings, which causes mouse events to not work correctly. To fix this, 
    // we can adjust the mouse events before ImGui processes them.
    uint32_t width = g_swapChain->getWidth();
    uint32_t height = g_swapChain->getHeight();
    float mousePosScaleX = float(width) / float(GameWindow::s_width);
    float mousePosScaleY = float(height) / float(GameWindow::s_height);
    float mousePosOffsetX = (width - Video::s_viewportWidth) / 2.0f;
    float mousePosOffsetY = (height - Video::s_viewportHeight) / 2.0f;
    for (int i = 0; i < io.Ctx->InputEventsQueue.Size; i++)
    {
        auto& e = io.Ctx->InputEventsQueue[i];
        if (e.Type == ImGuiInputEventType_MousePos)
        {
            if (e.MousePos.PosX != -FLT_MAX)
            {
                e.MousePos.PosX *= mousePosScaleX;
                e.MousePos.PosX -= mousePosOffsetX;
            }

            if (e.MousePos.PosY != -FLT_MAX)
            {
                e.MousePos.PosY *= mousePosScaleY;
                e.MousePos.PosY -= mousePosOffsetY;
            }
        }
    }

    ImGui::NewFrame();

    ResetImGuiCallbacks();

    UpdateImGuiUtils();
    AchievementMenu::Draw();
    OptionsMenu::Draw();
    InstallerWizard::Draw();
    ButtonWindow::Draw();
    MessageWindow::Draw();
    AchievementOverlay::Draw();
    Fader::Draw();
    BlackBar::Draw();
#ifdef __ANDROID__
    // Foreground overlay above every other UI so the controls stay reachable.
    TouchControls::Draw();
#endif

    assert(ImGui::GetBackgroundDrawList()->_ClipRectStack.Size == 1 && "Some clip rects were not removed from the stack!");

#ifdef __ANDROID__
    if (os::android::overlay::IsEnabled())
        DrawAndroidOverlay();
    else
#endif
    DrawFPS();
    DrawProfiler();
    ImGui::Render();

    auto drawData = ImGui::GetDrawData();
    if (drawData->CmdListsCount != 0)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawImGui;
        EnqueueRenderCommand(cmd);
    }
}

static void SetFramebuffer(GuestSurface *renderTarget, GuestSurface *depthStencil, bool settingForClear);

static void ProcDrawImGui(const RenderCommand& cmd)
{
    auto* fixedSamplerSet = g_useFixedDescriptors ? GetFixedSamplerSet(nullptr) : nullptr;
    if (g_useFixedDescriptors && fixedSamplerSet == nullptr)
        return;

    auto& commandList = g_commandLists[g_frame];
    auto& drawData = *ImGui::GetDrawData();
#ifdef __ANDROID__
    const bool profileImGui = g_passQueryPools[g_frame] != nullptr;
    const auto statsBefore = profileImGui ? commandList->getStats() : RenderCommandListStats{};
#endif
    commandList->setRenderPassLabel(GpuPassImGui);

    AddBarrier(g_backBuffer, RenderTextureLayout::COLOR_WRITE);
    if (g_batchImGui)
    {
        // UI callbacks set constants or the blend pipeline. All sampled images can transition
        // in the same barrier batch before the first UI draw.
        for (const auto* drawList : drawData.CmdLists)
        {
            for (const auto& drawCmd : drawList->CmdBuffer)
            {
                if (drawCmd.UserCallback == nullptr && drawCmd.ClipRect.z > drawCmd.ClipRect.x &&
                    drawCmd.ClipRect.w > drawCmd.ClipRect.y)
                {
                    AddBarrier(reinterpret_cast<GuestTexture*>(drawCmd.TextureId), RenderTextureLayout::SHADER_READ);
                }
            }
        }
    }

    FlushBarriers();
    // The last game command or pending resolve may have selected a different framebuffer.
    g_dirtyStates.renderTargetAndDepthStencil = true;
    SetFramebuffer(g_backBuffer, nullptr, false);
    auto pipeline = g_imPipeline.get();

    g_fixedDrawDescriptorState.bound = {};
    commandList->setGraphicsPipelineLayout(g_imPipelineLayout.get());
    commandList->setPipeline(pipeline);
    if (fixedSamplerSet != nullptr)
        commandList->setGraphicsDescriptorSet(fixedSamplerSet, 1);
    else
    {
        commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
        commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 1);
    }

    commandList->setViewports(RenderViewport(drawData.DisplayPos.x, drawData.DisplayPos.y, drawData.DisplaySize.x, drawData.DisplaySize.y));

    ImGuiPushConstants pushConstants{};
    pushConstants.displaySize = drawData.DisplaySize;
    pushConstants.inverseDisplaySize = { 1.0f / drawData.DisplaySize.x, 1.0f / drawData.DisplaySize.y };
    commandList->setGraphicsPushConstants(0, &pushConstants);

    size_t pushConstantRangeMin = ~0;
    size_t pushConstantRangeMax = 0;

    auto setPushConstants = [&](void* destination, const void* source, size_t size)
        {
            bool dirty = memcmp(destination, source, size) != 0;

            memcpy(destination, source, size);

            if (dirty)
            {
                size_t offset = reinterpret_cast<size_t>(destination) - reinterpret_cast<size_t>(&pushConstants);
                pushConstantRangeMin = std::min(pushConstantRangeMin, offset);
                pushConstantRangeMax = std::max(pushConstantRangeMax, offset + size);
            }
        };

    const auto bindGeometry = [&](const UploadAllocation& vertices, uint32_t vertexBytes,
        const UploadAllocation& indices, uint32_t indexBytes)
    {
        const RenderVertexBufferView vertexView(vertices.buffer->at(vertices.offset), vertexBytes);
        const RenderInputSlot inputSlot(0, sizeof(ImDrawVert));
        commandList->setVertexBuffers(0, &vertexView, 1, &inputSlot);
        const RenderIndexBufferView indexView(indices.buffer->at(indices.offset), indexBytes, RenderFormat::R16_UINT);
        commandList->setIndexBuffer(&indexView);
    };

    static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t));
    auto& allocator = g_uploadAllocators[g_frame];
    const size_t vertexBytes = size_t(drawData.TotalVtxCount) * sizeof(ImDrawVert);
    const size_t indexBytes = size_t(drawData.TotalIdxCount) * sizeof(ImDrawIdx);
    const bool sharedGeometry = g_batchImGui && drawData.CmdListsCount > 1 &&
        vertexBytes != 0 && vertexBytes <= UploadBuffer::SIZE && indexBytes != 0 && indexBytes <= UploadBuffer::SIZE;
    if (sharedGeometry)
    {
        const auto vertices = allocator.allocate(uint32_t(vertexBytes), alignof(ImDrawVert));
        const auto indices = allocator.allocate(uint32_t(indexBytes), alignof(ImDrawIdx));
        auto* vertexOutput = vertices.memory;
        auto* indexOutput = indices.memory;
        for (const auto* drawList : drawData.CmdLists)
        {
            if (!drawList->VtxBuffer.empty())
            {
                const size_t size = drawList->VtxBuffer.Size * sizeof(ImDrawVert);
                memcpy(vertexOutput, drawList->VtxBuffer.Data, size);
                vertexOutput += size;
            }

            if (!drawList->IdxBuffer.empty())
            {
                const size_t size = drawList->IdxBuffer.Size * sizeof(ImDrawIdx);
                memcpy(indexOutput, drawList->IdxBuffer.Data, size);
                indexOutput += size;
            }
        }
        bindGeometry(vertices, uint32_t(vertexBytes), indices, uint32_t(indexBytes));
    }

    ImRect clipRect{};
    uint32_t vertexBase = 0;
    uint32_t indexBase = 0;
    uint32_t boundFixedTexture = UINT32_MAX;

    for (const auto* drawList : drawData.CmdLists)
    {
        if (!sharedGeometry)
        {
            const uint32_t listVertexBytes = drawList->VtxBuffer.Size * sizeof(ImDrawVert);
            const uint32_t listIndexBytes = drawList->IdxBuffer.Size * sizeof(ImDrawIdx);
            const auto vertices = allocator.allocate<false>(drawList->VtxBuffer.Data, listVertexBytes, alignof(ImDrawVert));
            const auto indices = allocator.allocate<false>(drawList->IdxBuffer.Data, listIndexBytes, alignof(ImDrawIdx));
            bindGeometry(vertices, listVertexBytes, indices, listIndexBytes);
        }

        for (int j = 0; j < drawList->CmdBuffer.Size; j++)
        {
            auto& drawCmd = drawList->CmdBuffer[j];
            if (drawCmd.UserCallback != nullptr)
            {
                auto callbackData = reinterpret_cast<const ImGuiCallbackData*>(drawCmd.UserCallbackData);

                switch (static_cast<ImGuiCallback>(reinterpret_cast<size_t>(drawCmd.UserCallback)))
                {
                case ImGuiCallback::SetGradient:
                    setPushConstants(&pushConstants.boundsMin, &callbackData->setGradient, sizeof(callbackData->setGradient));
                    break;       
                case ImGuiCallback::SetShaderModifier:
                    setPushConstants(&pushConstants.shaderModifier, &callbackData->setShaderModifier, sizeof(callbackData->setShaderModifier));
                    break;
                case ImGuiCallback::SetOrigin:
                    setPushConstants(&pushConstants.origin, &callbackData->setOrigin, sizeof(callbackData->setOrigin));
                    break;
                case ImGuiCallback::SetScale:
                    setPushConstants(&pushConstants.scale, &callbackData->setScale, sizeof(callbackData->setScale));
                    break;       
                case ImGuiCallback::SetMarqueeFade:
                    setPushConstants(&pushConstants.boundsMin, &callbackData->setMarqueeFade, sizeof(callbackData->setMarqueeFade));
                    break;
                case ImGuiCallback::SetOutline:
                    setPushConstants(&pushConstants.outline, &callbackData->setOutline, sizeof(callbackData->setOutline));
                    break;
                case ImGuiCallback::SetProceduralOrigin:
                    setPushConstants(&pushConstants.proceduralOrigin, &callbackData->setProceduralOrigin, sizeof(callbackData->setProceduralOrigin));
                    break;
                case ImGuiCallback::SetAdditive:
                {
                    auto pipelineToSet = callbackData->setAdditive.enabled ? g_imAdditivePipeline.get() : g_imPipeline.get();
                    if (pipeline != pipelineToSet)
                    {
                        commandList->setPipeline(pipelineToSet);
                        pipeline = pipelineToSet;
                    }
                    break;
                }
                default:
                    assert(false && "Unknown ImGui callback type.");
                    break;
                }
            }
            else
            {
                if (drawCmd.ClipRect.z <= drawCmd.ClipRect.x || drawCmd.ClipRect.w <= drawCmd.ClipRect.y)
                    continue;

                auto texture = reinterpret_cast<GuestTexture*>(drawCmd.TextureId);
                uint32_t descriptorIndex = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
                if (texture != nullptr)
                {
                    if (!g_batchImGui && texture->layout != RenderTextureLayout::SHADER_READ)
                    {
                        EnsureTextureCopyCompleted(texture);
                        commandList->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY,
                            RenderTextureBarrier(texture->texture, RenderTextureLayout::SHADER_READ));

                        texture->layout = RenderTextureLayout::SHADER_READ;
                    }

                    descriptorIndex = texture->descriptorIndex;

                    if (texture == g_imFontTexture.get())
                        descriptorIndex |= 0x80000000;

                }

                const uint32_t textureIndex = descriptorIndex & 0x7FFFFFFF;
                if (g_useFixedDescriptors && (!g_batchImGui || boundFixedTexture != textureIndex))
                {
                    auto* set = GetFixedTextureSetForPass(textureIndex);
                    if (set == nullptr)
                        continue;
                    commandList->setGraphicsDescriptorSet(set, 0);
                    boundFixedTexture = textureIndex;
                }

                setPushConstants(&pushConstants.texture2DDescriptorIndex, &descriptorIndex, sizeof(descriptorIndex));

                if (pushConstantRangeMin < pushConstantRangeMax)
                {
                    commandList->setGraphicsPushConstants(0, reinterpret_cast<const uint8_t*>(&pushConstants) + pushConstantRangeMin, pushConstantRangeMin, pushConstantRangeMax - pushConstantRangeMin);
                    pushConstantRangeMin = ~0;
                    pushConstantRangeMax = 0;
                }

                if (memcmp(&clipRect, &drawCmd.ClipRect, sizeof(clipRect)) != 0)
                {
                    commandList->setScissors(RenderRect(int32_t(drawCmd.ClipRect.x), int32_t(drawCmd.ClipRect.y), int32_t(drawCmd.ClipRect.z), int32_t(drawCmd.ClipRect.w)));
                    clipRect = drawCmd.ClipRect;
                }

                commandList->drawIndexedInstanced(drawCmd.ElemCount, 1, indexBase + drawCmd.IdxOffset,
                    vertexBase + drawCmd.VtxOffset, 0);
            }
        }

        if (sharedGeometry)
        {
            vertexBase += drawList->VtxBuffer.Size;
            indexBase += drawList->IdxBuffer.Size;
        }
    }

#ifdef __ANDROID__
    if (profileImGui)
    {
        const auto statsAfter = commandList->getStats();
        g_imGuiFrameStats[g_frame] = {
            statsAfter.renderPasses - statsBefore.renderPasses,
            statsAfter.barrierPassBreaks - statsBefore.barrierPassBreaks,
            sharedGeometry ? 1u : uint32_t(drawData.CmdListsCount),
            statsAfter.draws != statsBefore.draws
        };
    }
#endif
}

// We have to check for this to properly handle the following situation:
// 1. Wait on swap chain.
// 2. Create loading thread.
// 3. Loading thread also waits on swap chain.
// 4. Loading thread presents and quits.
// 5. After the loading thread quits, application also presents.
static bool g_pendingWaitOnSwapChain = true;

#ifdef __ANDROID__
// Guest thread: nanoseconds spent waiting for the display since the last frame's report.
static int64_t g_hintDisplayWaitNs;

static int64_t HintNowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif

void Video::WaitOnSwapChain()
{
    if (g_pendingWaitOnSwapChain)
    {
        if (g_swapChainValid)
        {
#ifdef __ANDROID__
            const int64_t waitStart = HintNowNs();
#endif
            g_presentWaitProfiler.Begin();
            g_swapChain->wait();
            g_presentWaitProfiler.End();
#ifdef __ANDROID__
            g_hintDisplayWaitNs += HintNowNs() - waitStart;
#endif
        }

        g_pendingWaitOnSwapChain = false;
    }
}

static bool g_shouldPrecompilePipelines;
static std::atomic<bool> g_executedCommandList;

void CreateTextureLocal(Sonicteam::SoX::Graphics::Texture* pTexture, uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, uint32_t type)
{
    auto pGuestTexture = (GuestTexture*)pTexture->m_pTexture.get();
    auto pGuestTextureNew = CreateTexture(width, height, depth, levels, usage, format, pool, type);

    if (pGuestTexture->sourceSurface != nullptr)
    {
        pGuestTexture->sourceSurface->destinationTextures.erase(pGuestTexture);
        pGuestTexture->sourceSurface = nullptr;
    }

    // Swap host state in place so guest references stay valid, then destroy the old state through the deferred path.
    std::swap(*pGuestTexture, *pGuestTextureNew);
    std::swap(pGuestTexture->refCount, pGuestTextureNew->refCount);
    DestructResource(pGuestTextureNew);

    // Update the texture dimensions
    pTexture->m_Width = width;
    pTexture->m_Height = height;
}

static std::vector<std::pair<stdx::string, boost::shared_ptr<Sonicteam::SoX::Engine::RenderProcess>>> g_renderProcessCache{};

static bool CacheRenderProcess(Sonicteam::SoX::Engine::RenderScheduler* pRenderScheduler, const char* pName)
{
    for (auto& rRenderProcess : pRenderScheduler->m_lRenderProcesses)
    {
        if (rRenderProcess.first != pName)
            continue;

        g_renderProcessCache.push_back(rRenderProcess);

        return true;
    }

    return false;
}

static bool FindRenderProcess(lua50::lua_State* L, const char* pName)
{
    const auto pCallbackData = reinterpret_cast<Sonicteam::RenderAction::CallbackData*>(lua50::lua_topointer(L, 1));

    auto it = std::find_if(g_renderProcessCache.begin(), g_renderProcessCache.end(), [&](const auto& rRenderProcess)
    {
        return rRenderProcess.first == pName;
    });

    if (it == g_renderProcessCache.end())
        return false;

    // Cache hit, push cached render process to scheduler.
    pCallbackData->pRenderScheduler->m_lRenderProcesses.push_back(*it);

    // Remove cached render process.
    g_renderProcessCache.erase(it);

    return true;
}

// RenderGE1Particle callback
PPC_FUNC_IMPL(__imp__sub_8260AAB0);
PPC_FUNC(sub_8260AAB0)
{
    const auto L = reinterpret_cast<lua50::lua_State*>(ctx.r3.u32 + base);

    if (FindRenderProcess(L, "GE1Particle"))
    {
        ctx.r3.u32 = 1;
    }
    else
    {
        __imp__sub_8260AAB0(ctx, base);
    }
}

// RenderPostprocess callback
PPC_FUNC_IMPL(__imp__sub_8260ABB0);
PPC_FUNC(sub_8260ABB0)
{
    const auto L = reinterpret_cast<lua50::lua_State*>(ctx.r3.u32 + base);

    if (FindRenderProcess(L, "Postprocess"))
    {
        ctx.r3.u32 = 1;
    }
    else
    {
        __imp__sub_8260ABB0(ctx, base);
    }
}

// RenderSpanverse callback
PPC_FUNC_IMPL(__imp__sub_8260A9D0);
PPC_FUNC(sub_8260A9D0)
{
    const auto L = reinterpret_cast<lua50::lua_State*>(ctx.r3.u32 + base);

    if (FindRenderProcess(L, "Spanverse"))
    {
        ctx.r3.u32 = 1;
    }
    else 
    {
        __imp__sub_8260A9D0(ctx, base);
    }
}

void Video::Present() 
{
#ifdef __ANDROID__
    auto& diagnostics = g_androidFrameDiagnostics;
    diagnostics.Begin();
    auto perfMark = AndroidFrameDiagnostics::Clock::time_point{};
    double perfRenderWait = 0.0, perfPresent = 0.0, perfFence = 0.0, perfPacing = 0.0;
    int64_t hintFenceNs = 0;
#endif
    g_readyForCommands = false;

    RenderCommand cmd;
    cmd.type = RenderCommandType::ExecutePendingStretchRectCommands;
    EnqueueRenderCommand(cmd);

    DrawImGui();

    cmd.type = RenderCommandType::ExecuteCommandList;
    EnqueueRenderCommand(cmd);

#ifdef __ANDROID__
    if (diagnostics.enabled) perfMark = AndroidFrameDiagnostics::Clock::now();
#endif
    g_executedCommandList.wait(false);
#ifdef __ANDROID__
    if (diagnostics.enabled) perfRenderWait = AndroidFrameDiagnostics::Milliseconds(perfMark, AndroidFrameDiagnostics::Clock::now());
#endif
    g_executedCommandList = false;

    if (g_swapChainValid)
    {
        if (g_pendingWaitOnSwapChain)
        {
#ifdef __ANDROID__
            const int64_t waitStart = HintNowNs();
#endif
            g_presentWaitProfiler.Begin();
            g_swapChain->wait(); // Never gonna happen outside loading threads as explained above.
            g_presentWaitProfiler.End();
#ifdef __ANDROID__
            g_hintDisplayWaitNs += HintNowNs() - waitStart;
#endif
        }

        RenderCommandSemaphore* signalSemaphores[] = { g_renderSemaphores[g_backBufferIndex].get() };
#ifdef __ANDROID__
        if (diagnostics.enabled) perfMark = AndroidFrameDiagnostics::Clock::now();
#endif
        g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, std::size(signalSemaphores));
#ifdef __ANDROID__
        if (diagnostics.enabled) perfPresent = AndroidFrameDiagnostics::Milliseconds(perfMark, AndroidFrameDiagnostics::Clock::now());
#endif
    }

    g_pendingWaitOnSwapChain = true;

#ifdef __ANDROID__
    const uint32_t submittedFrame = g_frame;
    bool hintGpuRead = false;
#endif
    g_frame = g_nextFrame;
    g_nextFrame = (g_frame + 1) % NUM_FRAMES;

    if (g_commandListStates[g_frame])
    {
        g_frameFenceProfiler.Begin();
#ifdef __ANDROID__
        if (diagnostics.enabled) perfMark = AndroidFrameDiagnostics::Clock::now();
        const int64_t fenceStart = HintNowNs();
#endif
        const bool fenceCompleted = g_queue->waitForCommandFence(g_commandFences[g_frame].get());
#ifdef __ANDROID__
        hintFenceNs = HintNowNs() - fenceStart;
        if (diagnostics.enabled) perfFence = AndroidFrameDiagnostics::Milliseconds(perfMark, AndroidFrameDiagnostics::Clock::now());
#endif
        g_frameFenceProfiler.End();
        g_commandListStates[g_frame] = false;

        // The fence covers both the frame timestamps and the commands which wrote the pass queries.
        auto& frameQueryPool = g_queryPools[g_frame];
        bool frameTimesAvailable = fenceCompleted;
        if (frameTimesAvailable)
        {
            if (g_backend == Backend::VULKAN)
                frameTimesAvailable = frameQueryPool->queryResultsPrefix(NUM_QUERIES);
            else
                frameQueryPool->queryResults();
        }
        const uint64_t* frameTimestamps = frameQueryPool->getResults();
        if (frameTimesAvailable && frameTimestamps[1] >= frameTimestamps[0])
        {
            g_gpuFrameProfiler.Set(double(frameTimestamps[1] - frameTimestamps[0]) / 1e6);
#ifdef __ANDROID__
            hintGpuRead = true;

            if (auto& passQueryPool = g_passQueryPools[g_frame])
            {
                const auto& passes = g_commandLists[g_frame]->getRenderPassTimestamps();
                if (passQueryPool->queryResultsPrefix(uint32_t(passes.size())))
                {
                    const RenderCommandListStats stats = g_commandLists[g_frame]->getStats();
                    g_gpuPassDiagnostics.AddFrame(passes, passQueryPool->getResults(), frameTimestamps[0], frameTimestamps[1], stats,
                        g_imGuiFrameStats[g_frame], g_drawConstantFrameStats[g_frame], g_fixedDescriptorFrameStats[g_frame],
                        g_renderThreadFrameTimings[g_frame].renderCpuMs, g_directCsmSliceSkips[g_frame]);
                }
            }
#endif
        }
    }

    g_dirtyStates = DirtyStates(true);
    g_uploadAllocators[g_frame].reset();
    g_drawConstantBuffer = nullptr;
    if (g_useFixedDescriptors)
    {
        // This frame's fence was waited above. None of its sets are in use now.
        for (auto& sets : g_fixedTextureSets[g_frame])
            sets.reset();
        g_fixedSamplerSets[g_frame].reset();
        for (auto& [buffer, sets] : g_fixedBufferSamplerSets[g_frame])
            sets.reset();
    }
    g_intermediaryUploadAllocator.reset();
    g_triangleFanIndexData.reset();
    g_quadIndexData.reset();

    if (s_needsResize || g_resizePending)
    {
        Resize(s_viewportWidth, s_viewportHeight);
        s_needsResize = false;
    }

#ifdef __ANDROID__
    // Acquiring the next image waits for the display when the swap chain is full.
    const int64_t acquireStart = HintNowNs();
#endif
    CheckSwapChain();
#ifdef __ANDROID__
    g_hintDisplayWaitNs += HintNowNs() - acquireStart;
#endif

    cmd.type = RenderCommandType::BeginCommandList;
    EnqueueRenderCommand(cmd);

#ifdef __ANDROID__
    if (diagnostics.enabled) perfMark = AndroidFrameDiagnostics::Clock::now();

    // ADPF: the frame's work runs from the end of the previous frame's limiter to here, minus the
    // waits for the display (swap chain wait and acquire); the CPU part also leaves out the wait
    // for the GPU fence. The GPU timestamps of a frame are read NUM_FRAMES - 1 frames later, so
    // each frame's CPU timings are kept with its command list slot and reported together with
    // its own GPU time.
    struct HintFrame
    {
        int64_t start = 0;
        int64_t total = 0;
        int64_t cpu = 0;
    };
    static HintFrame s_hintFrames[NUM_FRAMES];
    static int64_t s_hintWorkStart = 0;
    {
        const int64_t now = HintNowNs();
        if (s_hintWorkStart != 0)
        {
            const int64_t total = now - s_hintWorkStart - g_hintDisplayWaitNs;
            s_hintFrames[submittedFrame] = { s_hintWorkStart, total, total - hintFenceNs };
        }

        HintFrame& completed = s_hintFrames[g_frame];
        if (hintGpuRead && completed.start != 0)
        {
            const double gpuMs = g_gpuFrameProfiler.value.load();
            os::android::hint::ReportFrame(completed.start, completed.total, completed.cpu,
                gpuMs > 0.0 ? int64_t(gpuMs * 1e6) : -1, int32_t(Config::FPS));
        }
        completed = {};
    }
    os::android::thread_hints::CheckAffinity();
#endif
    int32_t effectiveFps = Config::FPS;
#ifdef __ANDROID__
    const bool loadingCapActive = g_capLoadingFps && App::s_isLoading &&
        (effectiveFps > 60 || effectiveFps >= FPS_MAX);
    if (loadingCapActive)
        effectiveFps = 60;
#else
    constexpr bool loadingCapActive = false;
#endif

    if ((effectiveFps >= FPS_MIN && effectiveFps < FPS_MAX) || loadingCapActive)
    {
        using namespace std::chrono_literals;

        static std::chrono::steady_clock::time_point s_next;

        auto now = std::chrono::steady_clock::now();

        if (now < s_next)
        {
#ifdef __ANDROID__
            // Experiment (a launcher switch, off by default): sleep to 0.5 ms before the deadline
            // instead of whole milliseconds to 2 ms before it, leaving 0.5 ms instead of 2-3 ms
            // of yielding per frame. Compare frame times, pacing and battery use.
            static const bool s_preciseLimiter = []
            {
                std::error_code error;
                const bool enabled = std::filesystem::exists(os::android::GetDataRoot() / "precise_frame_limiter.enabled", error);
                LOGFN("Frame limiter: {}.", enabled ? "precise (precise_frame_limiter.enabled)" : "default");
                return enabled;
            }();

            if (loadingCapActive)
                std::this_thread::sleep_until(s_next);
            else if (s_preciseLimiter)
                std::this_thread::sleep_until(s_next - 500us);
            else
#endif
            std::this_thread::sleep_for(std::chrono::floor<std::chrono::milliseconds>(s_next - now - 2ms));

            if (!loadingCapActive)
            {
                while ((now = std::chrono::steady_clock::now()) < s_next)
                    std::this_thread::yield();
            }
        }
        else
        {
            s_next = now;
        }

        s_next += 1000000000ns / effectiveFps;
    }

#ifdef __ANDROID__
    if (diagnostics.enabled) perfPacing = AndroidFrameDiagnostics::Milliseconds(perfMark, AndroidFrameDiagnostics::Clock::now());
    diagnostics.End(perfRenderWait, perfPresent, perfFence, perfPacing);
    g_androidFrameRateSummary.Frame();
    s_hintWorkStart = HintNowNs();
    g_hintDisplayWaitNs = 0;
#endif
    g_presentProfiler.Reset();
}

bool Video::IsResizePending()
{
    // Android can defer buffer recreation independently of the configuration resize flag.
    return s_needsResize || g_resizePending;
}

void Video::Resize(uint32_t width, uint32_t height)
{
    // Cleared at the end once the game's buffers were actually recreated.
    g_resizePending = true;

    const auto pApp = App::s_pApp;

    if (!pApp)
        return;

    const auto pDoc = pApp->m_pDoc.get();
    const auto pResourceManager = Sonicteam::SoX::ResourceManager::GetInstance();
    const auto pTextureMgr = Sonicteam::SoX::Graphics::TextureMgr::GetInstance();

    if (!pDoc || !pResourceManager || !pTextureMgr)
        return;

    const auto pRenderTargetContainer = pDoc->m_pRenderTargetContainer.get();
    const auto pMyGraphicsDevice = pDoc->m_pMyGraphicsDevice.get();

    if (!pRenderTargetContainer || !pMyGraphicsDevice)
        return;

    // The back buffer and HUD use the output viewport. ResolutionScale only shrinks the
    // scene the game composites into it with its "Rasterize" pass.
    const uint32_t backBufferWidth = width;
    const uint32_t backBufferHeight = height;

    width *= Config::ResolutionScale;
    height *= Config::ResolutionScale;

    if (!width || !height)
    {
        LOGN_ERROR("Attempting to resize buffers to 0x0.");
        return;
    }

    struct BufferInfo
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t Index;
        uint32_t Flags;
    };

    static std::map<std::string, BufferInfo> buffers{};

    // Update buffer dimensions.
    buffers["framebuffer0"] = { width, height, 0, 4 };
    buffers["framebuffer1"] = { width, height, 0, 0 };
    buffers["framebuffer_1_4_0"] = { width >> 2, height >> 2, 3, 2 };
    buffers["framebuffer_1_4_1"] = { width >> 2, height >> 2, 0, 2 };
    buffers["framebuffer_1_8_0"] = { width >> 3, height >> 3, 3, 2 };
    buffers["framebuffer_1_8_1"] = { width >> 3, height >> 3, 0, 2 };
    buffers["framebuffer_1_16_0"] = { width >> 4, height >> 4, 3, 2 };
    buffers["framebuffer_1_16_1"] = { width >> 4, height >> 4, 0, 2 };
    buffers["framebuffer_1_32_0"] = { width >> 5, height >> 5, 3, 2 };
    buffers["framebuffer_1_32_1"] = { width >> 5, height >> 5, 0, 2 };
    buffers["depthstencil_1_4"] = { width, height, 6, 0 };

    if (g_backBuffer && g_backBuffer != pApp->m_pBackBufferSurface.get())
        g_backBuffer->Release();

    if (g_depthStencil && g_depthStencil != pApp->m_pDepthStencilSurface.get())
        g_depthStencil->Release();

    auto& rDeviceInfo = pApp->m_DeviceInfo;

    auto surfaceParams = D3DXBSURFACE_PARAMETERS(0, 0, 0);
    const auto surfaceBase = uint32_t(backBufferHeight * 1.155555555555556); // 720p: 0x340

    // Recreate main buffers.
    ReleaseResource(static_cast<GuestTexture*>(pApp->m_pFrontBufferTexture.get()));
    pApp->m_pFrontBufferTexture = CreateTexture(backBufferWidth, backBufferHeight, 1, 1, 1, D3DFMT_LE_X8R8G8B8, 0, 3);
    ReleaseResource(static_cast<GuestSurface*>(pApp->m_pBackBufferSurface.get()));
    pApp->m_pBackBufferSurface = CreateSurface(backBufferWidth, backBufferHeight, D3DFMT_A8R8G8B8, 0, reinterpret_cast<GuestSurfaceCreateParams*>(&surfaceParams));

    rDeviceInfo.SurfaceParamsA = surfaceParams;
    surfaceParams.Base = surfaceParams.Base + surfaceBase;

    ReleaseResource(static_cast<GuestSurface*>(pApp->m_pDepthStencilSurface.get()));
    pApp->m_pDepthStencilSurface = CreateSurface(width, height, D3DFMT_D24FS8, 0, reinterpret_cast<GuestSurfaceCreateParams*>(&surfaceParams));

    rDeviceInfo.SurfaceParamsB = surfaceParams;
    rDeviceInfo.SurfaceParamsC = rDeviceInfo.SurfaceParamsB;
    rDeviceInfo.SurfaceParamsC.Base = rDeviceInfo.SurfaceParamsB.Base + surfaceBase;

    // Viewport is reset here because we're using the game's backbuffer directly.
    SetRenderTarget(static_cast<GuestDevice*>(pApp->m_pDevice.get()), 0, static_cast<GuestSurface*>(pApp->m_pBackBufferSurface.get()));
    SetDepthStencilSurface(static_cast<GuestDevice*>(pApp->m_pDevice.get()), static_cast<GuestSurface*>(pApp->m_pDepthStencilSurface.get()));

    g_backBuffer = static_cast<GuestSurface*>(pApp->m_pBackBufferSurface.get());
    g_depthStencil = static_cast<GuestSurface*>(pApp->m_pDepthStencilSurface.get());

    g_backBuffer->format = BACKBUFFER_FORMAT;

    rDeviceInfo.PresentParameters.BackBufferWidth = backBufferWidth;
    rDeviceInfo.PresentParameters.BackBufferHeight = backBufferHeight;
    rDeviceInfo.pColorTile2x = pApp->m_pColorTile2x;
    rDeviceInfo.pDepthTile2x = pApp->m_pDepthTile2x;
    rDeviceInfo.pColorTile4x = pApp->m_pColorTile4x;
    rDeviceInfo.pDepthTile4x = pApp->m_pDepthTile4x;

    pMyGraphicsDevice->m_SurfaceParamsA = rDeviceInfo.SurfaceParamsA;
    pMyGraphicsDevice->m_SurfaceParamsB = rDeviceInfo.SurfaceParamsB;
    pMyGraphicsDevice->m_SurfaceParamsC = rDeviceInfo.SurfaceParamsC;

    const auto setSurface = [&](Sonicteam::SoX::Graphics::Surface* pSurface, GuestSurface* pGuestSurface)
    {
        if (!pSurface)
            return;

        GuestToHostFunction<void>(sub_82593038, pSurface, pGuestSurface);
    };

    setSurface(pMyGraphicsDevice->m_spBackBuffer.get(), static_cast<GuestSurface*>(pApp->m_pBackBufferSurface.get()));
    setSurface(pMyGraphicsDevice->m_spDepthStencil.get(), static_cast<GuestSurface*>(pApp->m_pDepthStencilSurface.get()));

    // Refresh graphics device and configure FBO surfaces.
    GuestToHostFunction<void>(sub_82637418, pMyGraphicsDevice);
    GuestToHostFunction<void>(sub_825BAE48, pMyGraphicsDevice->m_FrameBufferObject.get(), 0, &pMyGraphicsDevice->m_spBackBuffer);
    GuestToHostFunction<void>(sub_825BAEB8, pMyGraphicsDevice->m_FrameBufferObject.get(), &pMyGraphicsDevice->m_spDepthStencil);

    struct FormatConfig
    {
        be<GuestFormat> SurfaceFormat;
        be<GuestFormat> TextureFormat;
        be<uint32_t> Usage;
    };

    const auto pFormatConfigs = reinterpret_cast<FormatConfig*>(g_memory.base + 0x82B7BD20);

    const auto getSurfaceParams = [&](uint32_t index, uint32_t flags) -> D3DXBSURFACE_PARAMETERS*
    {
        if (pFormatConfigs[index].Usage != 1 || (flags & 1) != 0)
            return &pMyGraphicsDevice->m_SurfaceParamsB;

        return (flags & 2) == 0
            ? &pMyGraphicsDevice->m_SurfaceParamsA
            : &pMyGraphicsDevice->m_SurfaceParamsC;
    };

    // Update depth stencil surfaces.
    for (auto& rSurface : pRenderTargetContainer->m_mspDepthStencil_1_4)
    {
        const auto pSurfaceName = rSurface.first.c_str();

        if (!buffers.contains(pSurfaceName))
            continue;

        const auto& rBufferInfo = buffers[pSurfaceName];

        auto pSurfaceParams = getSurfaceParams(rBufferInfo.Index, rBufferInfo.Flags);
        auto pNewSurface = CreateSurface(rBufferInfo.Width, rBufferInfo.Height, pFormatConfigs[rBufferInfo.Index].SurfaceFormat, 0, reinterpret_cast<GuestSurfaceCreateParams*>(pSurfaceParams));

        GuestToHostFunction<void>(sub_82592E98, rSurface.second.get(), pNewSurface, rBufferInfo.Width, rBufferInfo.Height);
    }

    // Update frame buffer textures.
    for (auto& rTexture : pRenderTargetContainer->m_mspFrameBuffer)
    {
        const auto pTextureName = rTexture.first.c_str();

        if (!buffers.contains(pTextureName))
            continue;

        const auto& rBufferInfo = buffers[pTextureName];

        CreateTextureLocal(rTexture.second.get(), rBufferInfo.Width, rBufferInfo.Height, 1, 1, pFormatConfigs[rBufferInfo.Index].Usage, pFormatConfigs[rBufferInfo.Index].TextureFormat, 0, 3);

        const auto surfaceFormatIndex = (rBufferInfo.Flags & 4) == 0
            ? rBufferInfo.Index
            : 3;

        auto pSurfaceParams = getSurfaceParams(surfaceFormatIndex, rBufferInfo.Flags);
        auto pNewSurface = CreateSurface(rBufferInfo.Width, rBufferInfo.Height, pFormatConfigs[surfaceFormatIndex].SurfaceFormat, 0, reinterpret_cast<GuestSurfaceCreateParams*>(pSurfaceParams));

        GuestToHostFunction<void>(sub_82592E98, rTexture.second->m_aspSurfaces[0].get(), pNewSurface, rBufferInfo.Width, rBufferInfo.Height);
    }

    // Clear post-process buffers.
    pRenderTargetContainer->m_mspDepthStencil_256.clear();
    pRenderTargetContainer->m_mspPostEffect.clear();
    pRenderTargetContainer->m_mspPostEffectAfter.clear();

    Sonicteam::HUDRaderMap* pHUDRaderMap{};

    auto& rmTextureResources = pResourceManager->m_mResources[pTextureMgr->m_MgrIndex];

    if (const auto pGame = pApp->GetGame(); rmTextureResources.find("radermap") != rmTextureResources.end())
    {
        if (const auto pPopupScreenTask = pGame->m_rPopupScreenTask.m_pRef)
        {
            pHUDRaderMap = pPopupScreenTask->GetHUDPopupScreen<Sonicteam::HUDRaderMap>();

            // Release "radermap" textures.
            pHUDRaderMap->m_pMainTexture.reset();
            pHUDRaderMap->m_pMaskTexture.reset();
        }
    }

    const auto sfxMatrices1 = pDoc->m_pSFXAgent->m_aSFXMatrices1;
    const auto sfxMatrices2 = pDoc->m_pSFXAgent->m_aSFXMatrices2;
    pDoc->m_pSFXAgent->m_aSFXMatrices1 = 0;
    pDoc->m_pSFXAgent->m_aSFXMatrices2 = 0;

    // Cache render processes.
    CacheRenderProcess(pDoc->m_pRenderScheduler, "GE1Particle");
    CacheRenderProcess(pDoc->m_pRenderScheduler, "Postprocess");
    CacheRenderProcess(pDoc->m_pRenderScheduler, "Spanverse");

    // Reload current render script.
    GuestToHostFunction<void>(sub_8260DF88, pDoc, Sonicteam::Globals::ms_pCurrentRenderScript, 1);

    // Drop cached render processes, no longer needed after reloading render script.
    g_renderProcessCache.clear();

    if (pDoc->m_pSFXAgent->m_aSFXMatrices1)
        g_userHeap.Free(pDoc->m_pSFXAgent->m_aSFXMatrices1->GetArray());

    if (pDoc->m_pSFXAgent->m_aSFXMatrices2)
        g_userHeap.Free(pDoc->m_pSFXAgent->m_aSFXMatrices2->GetArray());

    pDoc->m_pSFXAgent->m_aSFXMatrices1 = sfxMatrices1;
    pDoc->m_pSFXAgent->m_aSFXMatrices2 = sfxMatrices2;

    if (pHUDRaderMap)
    {
        const auto setTexture = [&](const char* pName, auto* spTexture)
        {
            if (auto it = rmTextureResources.find(pName); it != rmTextureResources.end())
                *spTexture = reinterpret_cast<Sonicteam::MyTexture*>(it->second.get());
        };

        // Set new "radermap" textures after Lua script reload.
        setTexture("radermap", &pHUDRaderMap->m_pMainTexture);
        setTexture("radermap_mask", &pHUDRaderMap->m_pMaskTexture);
    }

    g_resizePending = false;
    LOGFN("Resized buffers: back buffer {}x{}, scene {}x{} (resolution scale {}).",
        backBufferWidth, backBufferHeight, width, height, Config::ResolutionScale.Value);
}

void Video::StartPipelinePrecompilation()
{
    LoadingPatches::Events.push_back(&g_loadingPipelineCompilationEvent);

    EnqueuePipelineTask(PipelineTaskType::PrecompilePipelines);
}

static void SetRootDescriptor(const UploadAllocation& allocation, size_t index)
{
    auto& commandList = g_commandLists[g_frame];

    if (g_backend != Backend::D3D12)
        commandList->setGraphicsPushConstants(0, &allocation.deviceAddress, 8 * index, 8);
    else
        commandList->setGraphicsRootDescriptor(allocation.buffer->at(allocation.offset), index);
}

template<bool TBufferConstants>
static void UploadDrawConstants()
{
    auto& allocator = g_uploadAllocators[g_frame];
    bool uploadVertex = g_dirtyStates.vertexShaderConstants;
    bool uploadPixel = g_dirtyStates.pixelShaderConstants;
    bool uploadShared = g_dirtyStates.sharedConstants;

    if constexpr (TBufferConstants)
    {
        // All three offsets address the same buffer. Refresh unchanged constants on a new page.
        const bool newPage = g_drawConstantBuffer == nullptr || allocator.index != g_drawConstantPageIndex;
        uploadVertex |= newPage;
        uploadPixel |= newPage;
        uploadShared |= newPage;
        static_assert(sizeof(g_vertexShaderConstants) % SHADER_CONSTANT_ALIGNMENT == 0);
        static_assert(sizeof(g_pixelShaderConstants) % SHADER_CONSTANT_ALIGNMENT == 0);
        const uint32_t requiredBytes = (uploadVertex ? sizeof(g_vertexShaderConstants) : 0) +
            (uploadPixel ? sizeof(g_pixelShaderConstants) : 0) + (uploadShared ? sizeof(g_sharedConstants) : 0);
        if (requiredBytes != 0)
            allocator.prepareAllocation(requiredBytes, SHADER_CONSTANT_ALIGNMENT);
        if (newPage || allocator.index != g_drawConstantPageIndex)
        {
            uploadVertex = uploadPixel = uploadShared = true;
            g_drawConstantPageIndex = allocator.index;
            g_drawConstantBuffer = nullptr;
        }
    }

    [[maybe_unused]] uint32_t pushWrites = 0;
    const auto setConstant = [&](const UploadAllocation& allocation, size_t index)
    {
        if constexpr (TBufferConstants)
        {
            assert(g_drawConstantBuffer == nullptr || allocation.buffer == g_drawConstantBuffer);
            g_drawConstantBuffer = allocation.buffer;
            g_drawConstantOffsets[index] = uint32_t(allocation.offset);
        }
        else if (g_optimizeDrawConstants)
        {
            g_drawConstantAddresses[index] = allocation.deviceAddress;
        }
        else
        {
            SetRootDescriptor(allocation, index);
            ++pushWrites;
        }
    };

    if (uploadVertex)
    {
        const auto allocation = g_optimizeDrawConstants ?
            allocator.allocate<false>(g_vertexShaderConstants, sizeof(g_vertexShaderConstants), SHADER_CONSTANT_ALIGNMENT) :
            allocator.allocate<true>(g_vertexShaderConstants, sizeof(g_vertexShaderConstants), SHADER_CONSTANT_ALIGNMENT);
        setConstant(allocation, 0);
    }

    if (uploadPixel)
    {
        const auto allocation = g_optimizeDrawConstants ?
            allocator.allocate<false>(g_pixelShaderConstants, sizeof(g_pixelShaderConstants), SHADER_CONSTANT_ALIGNMENT) :
            allocator.allocate<true>(g_pixelShaderConstants, sizeof(g_pixelShaderConstants), SHADER_CONSTANT_ALIGNMENT);
        setConstant(allocation, 1);
    }

    if (uploadShared)
        setConstant(allocator.allocate<false>(&g_sharedConstants, sizeof(g_sharedConstants), SHADER_CONSTANT_ALIGNMENT), 2);

    const bool uploaded = uploadVertex || uploadPixel || uploadShared;
    if constexpr (TBufferConstants)
    {
        if (uploaded || !g_optimizeDrawConstants)
        {
            g_commandLists[g_frame]->setGraphicsPushConstants(0, g_drawConstantOffsets, 0, sizeof(g_drawConstantOffsets));
            ++pushWrites;
        }
    }
    else if (g_optimizeDrawConstants && (uploaded || g_dirtyStates.vertexShaderConstantAddress))
    {
        // Copy shaders may overwrite the first address without changing its buffer.
        // Rebind it with any changed addresses in one contiguous push.
        constexpr uint32_t addressSize = sizeof(g_drawConstantAddresses[0]);
        const bool pushVertex = uploadVertex || g_dirtyStates.vertexShaderConstantAddress;
        const uint32_t first = pushVertex ? 0 : uploadPixel ? 1 : 2;
        const uint32_t last = uploadShared ? 2 : uploadPixel ? 1 : 0;
        g_commandLists[g_frame]->setGraphicsPushConstants(0, &g_drawConstantAddresses[first], first * addressSize,
            (last - first + 1) * addressSize);
        ++pushWrites;
    }

#ifdef __ANDROID__
    if (g_passQueryPools[g_frame] != nullptr)
    {
        auto& stats = g_drawConstantFrameStats[g_frame];
        stats.uploadedBytes += (uploadVertex ? sizeof(g_vertexShaderConstants) : 0) +
            (uploadPixel ? sizeof(g_pixelShaderConstants) : 0) + (uploadShared ? sizeof(g_sharedConstants) : 0);
        stats.pushWrites += pushWrites;
    }
#endif
}

#ifdef __ANDROID__
static RenderRect GetGammaPassScissor(int32_t width, int32_t height, uint32_t rotation, const RenderRect& viewport,
    GammaPassConstants& constants)
{
    RenderRect output = viewport;
    switch (rotation)
    {
    case 90:
        output = RenderRect(height - viewport.bottom, viewport.left, height - viewport.top, viewport.right);
        constants.sourceStep[1] = -1;
        break;
    case 180:
        output = RenderRect(width - viewport.right, height - viewport.bottom, width - viewport.left, height - viewport.top);
        constants.sourceStep[0] = -1;
        constants.sourceStep[1] = -1;
        break;
    case 270:
        output = RenderRect(viewport.top, width - viewport.right, viewport.bottom, width - viewport.left);
        constants.sourceStep[0] = -1;
        break;
    }

    // Use the original rectangle for source coordinates even when the output is clipped to the image.
    const RenderRect sourceAxes = constants.transpose ?
        RenderRect(output.top, output.left, output.bottom, output.right) : output;
    constants.sourceOffset[0] = constants.sourceStep[0] > 0 ? -sourceAxes.left : sourceAxes.right - 1;
    constants.sourceOffset[1] = constants.sourceStep[1] > 0 ? -sourceAxes.top : sourceAxes.bottom - 1;

    const int32_t imageWidth = constants.transpose ? height : width;
    const int32_t imageHeight = constants.transpose ? width : height;
    return RenderRect(std::clamp(output.left, 0, imageWidth), std::clamp(output.top, 0, imageHeight),
        std::clamp(output.right, 0, imageWidth), std::clamp(output.bottom, 0, imageHeight));
}
#endif

static void DrawGammaCorrection(RenderTexture* swapChainTexture)
{
    const float gamma = 1.0f / std::clamp(0.85f + (Config::Brightness - 0.5f) * 1.2f, 0.1f, 4.0f);
    const int32_t width = g_swapChain->getWidth();
    const int32_t height = g_swapChain->getHeight();
    const int32_t viewportWidth = Video::s_viewportWidth;
    const int32_t viewportHeight = Video::s_viewportHeight;
    const int32_t offsetX = (width - viewportWidth) / 2;
    const int32_t offsetY = (height - viewportHeight) / 2;
    const uint32_t preRotation = g_swapChain->getPreRotation();
    const bool transposed = (preRotation == 90) || (preRotation == 270);
    const int32_t imageWidth = transposed ? height : width;
    const int32_t imageHeight = transposed ? width : height;
    const RenderRect wholeImage(0, 0, imageWidth, imageHeight);

#ifdef __ANDROID__
    GammaPassConstants constants{ gamma, uint32_t(transposed), {}, { 1, 1 } };
    const RenderRect viewport(offsetX, offsetY, offsetX + viewportWidth, offsetY + viewportHeight);
    const RenderRect scissor = GetGammaPassScissor(width, height, preRotation, viewport, constants);
#else
    struct
    {
        float gamma;
        uint32_t textureDescriptorIndex;
        int32_t viewportOffsetX, viewportOffsetY;
        int32_t viewportWidth, viewportHeight;
        int32_t swapChainWidth, swapChainHeight;
        uint32_t preRotation;
    } constants{ gamma, g_intermediaryBackBufferTextureDescriptorIndex, offsetX, offsetY,
        viewportWidth, viewportHeight, width, height, preRotation };
    const auto allocation = g_uploadAllocators[g_frame].allocate<false>(&constants, sizeof(constants), SHADER_CONSTANT_ALIGNMENT);
    const RenderRect& scissor = wholeImage;
#endif

    auto& framebuffer = g_backBuffer->framebuffers[swapChainTexture];
    if (!framebuffer)
    {
        RenderFramebufferDesc desc;
        desc.colorAttachments = const_cast<const RenderTexture**>(&swapChainTexture);
        desc.colorAttachmentsCount = 1;
        framebuffer = g_device->createFramebuffer(desc);
    }

    const RenderTextureBarrier srcBarriers[] =
    {
        RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ),
        RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE)
    };

    auto& commandList = g_commandLists[g_frame];
    commandList->barriers(RenderBarrierStage::GRAPHICS, srcBarriers, std::size(srcBarriers));
#ifdef __ANDROID__
    g_fixedDrawDescriptorState.bound = {};
    commandList->setGraphicsPipelineLayout(g_gammaPipelineLayout.get());
    commandList->setGraphicsDescriptorSet(g_gammaSourceDescriptorSet.get(), 0);
    commandList->setGraphicsPushConstants(0, &constants);
#else
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    SetRootDescriptor(allocation, 2);
#endif
    commandList->setPipeline(g_gammaCorrectionPipeline.get());
    commandList->setFramebuffer(framebuffer.get());

    if (scissor == wholeImage)
        commandList->discardColorAttachment(0);
    else
        commandList->clearColor(0, RenderColor(0.0f, 0.0f, 0.0f, 0.0f));

    if (!scissor.isEmpty())
    {
        commandList->setViewports(RenderViewport(scissor.left, scissor.top, scissor.right - scissor.left,
            scissor.bottom - scissor.top));
        commandList->setScissors(scissor);
        commandList->drawInstanced(3, 1, 0, 0);
    }

    commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));
}

static void ProcExecuteCommandList(const RenderCommand& cmd)
{
    g_commandLists[g_frame]->setRenderPassLabel(GpuPassGamma);

    if (g_swapChainValid)
    {
        auto swapChainTexture = g_swapChain->getTexture(g_backBufferIndex);
        const bool needsGammaPass = g_backBuffer->texture == g_intermediaryBackBufferTexture.get();
        if (needsGammaPass)
        {
            DrawGammaCorrection(swapChainTexture);
        }
        else
        {
            AddBarrier(g_backBuffer, RenderTextureLayout::PRESENT);
            FlushBarriers();
        }
    }

    auto &commandList = g_commandLists[g_frame];
#ifdef __ANDROID__
    if (g_passQueryPools[g_frame] != nullptr)
        commandList->setRenderPassTimestamps(nullptr, 0, 0);
#endif
    commandList->writeTimestamp(g_queryPools[g_frame].get(), 1);
    commandList->end();

    if (g_swapChainValid)
    {
        const RenderCommandList *commandLists[] = { commandList.get() };
        RenderCommandSemaphore *waitSemaphores[] = { g_acquireSemaphores[g_frame].get() };
        RenderCommandSemaphore *signalSemaphores[] = { g_renderSemaphores[g_backBufferIndex].get() };

        g_queue->executeCommandLists(
            commandLists, std::size(commandLists),
            waitSemaphores, std::size(waitSemaphores),
            signalSemaphores, std::size(signalSemaphores),
            g_commandFences[g_frame].get());
    }
    else
    {
        g_queue->executeCommandLists(commandList.get(), g_commandFences[g_frame].get());
    }

#ifdef __ANDROID__
    auto& timing = g_renderThreadFrameTimings[g_frame];
    if (timing.beginCpuMs > 0.0)
    {
        const double endCpuMs = AndroidFrameDiagnostics::CpuMilliseconds();
        if (endCpuMs >= timing.beginCpuMs)
            timing.renderCpuMs = endCpuMs - timing.beginCpuMs;
    }
#endif
    g_commandListStates[g_frame] = true;

    g_executedCommandList = true;
    g_executedCommandList.notify_one();
}

static void ProcBeginCommandList(const RenderCommand& cmd)
{
#ifdef __ANDROID__
    if (g_depthCopyTraceRemaining != 0)
    {
        ++g_depthCopyTraceFrame;
        g_depthCopyTrace = {};
    }
#endif

    g_renderFrame = g_frame;
    DestructTempResources();
    BeginCommandList();
}

static GuestSurface* GetBackBuffer()
{
    if (g_backBuffer)
        g_backBuffer->AddRef();

    return g_backBuffer;
}

static GuestSurface* GetDepthStencil() 
{
    if (g_depthStencil)
        g_depthStencil->AddRef();

    return g_depthStencil;
}

std::pair<uint32_t, uint32_t> Video::GetOutputSize()
{
    return { g_swapChain->getWidth(), g_swapChain->getHeight() };
}

void Video::ComputeViewportDimensions()
{
    uint32_t width = g_swapChain->getWidth();
    uint32_t height = g_swapChain->getHeight();
    float aspectRatio = float(width) / float(height);

    switch (Config::AspectRatio)
    {
        case EAspectRatio::Original:
        {
            if (aspectRatio > WIDE_ASPECT_RATIO)
            {
                s_viewportWidth = height * 16 / 9;
                s_viewportHeight = height;
            }
            else
            {
                s_viewportWidth = width;
                s_viewportHeight = width * 9 / 16;
            }

            break;
        }

        default:
            s_viewportWidth = width;
            s_viewportHeight = height;
            break;
    }

    AspectRatioPatches::ComputeOffsets();
}

static RenderFormat ConvertFormat(uint32_t format)
{
    switch (format)
    {
    case D3DFMT_A16B16G16R16F:
    case D3DFMT_A16B16G16R16F_2:
    case D3DFMT_A16B16G16R16F_EXPAND:
        return RenderFormat::R16G16B16A16_FLOAT;
    case D3DFMT_LIN_A8R8G8B8:
        return RenderFormat::B8G8R8A8_UNORM;
    case D3DFMT_A8B8G8R8:
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_LE_X8R8G8B8:
        return RenderFormat::R8G8B8A8_UNORM;
    case D3DFMT_R32F:
        return RenderFormat::R32_FLOAT;
    case D3DFMT_D24FS8:
    case D3DFMT_D24S8:
        return g_depthStencilFormat;
    case D3DFMT_G16R16F:
    case D3DFMT_G16R16F_2:
        return RenderFormat::R16G16_FLOAT;
    case D3DFMT_INDEX16:
        return RenderFormat::R16_UINT;
    case D3DFMT_INDEX32:
        return RenderFormat::R32_UINT;
    case D3DFMT_A8:
    case D3DFMT_L8:
    case D3DFMT_L8_2:
        return RenderFormat::R8_UNORM;
    case D3DFMT_DXT1:
        return RenderFormat::BC1_UNORM;
    case D3DFMT_DXT4:
        return RenderFormat::BC3_UNORM;
    default:
        LOGF_WARNING("{:x}\n", format);
        assert(false && "Unknown format");
        return RenderFormat::R16G16B16A16_FLOAT;
    }
}

static void DiscardTexture(GuestBaseTexture* texture, RenderTextureLayout layout)
{
    if (g_backend == Backend::D3D12)
    {
        std::lock_guard lock(g_discardMutex);

        g_discardCommandList->begin();
        auto& layoutRef = GetLayoutRef(texture);
        if (layoutRef != layout)
        {
            g_discardCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(texture->texture, layout));
            layoutRef = layout;
        }

        g_discardCommandList->discardTexture(texture->texture);
        g_discardCommandList->end();

        g_queue->executeCommandLists(g_discardCommandList.get(), g_discardCommandFence.get());
        g_queue->waitForCommandFence(g_discardCommandFence.get());
    }
}

static GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, uint32_t type) 
{
    ResourceType resourceType;

    switch (type)
    {
    case 17:
        resourceType = ResourceType::VolumeTexture;
        break;
    case 19:
        resourceType = ResourceType::ArrayTexture;
        break;
    default:
        resourceType = ResourceType::Texture;
        break;
    }

    const auto texture = g_userHeap.AllocPhysical<GuestTexture>(resourceType);

    RenderTextureDesc desc;
    desc.dimension = texture->type == ResourceType::VolumeTexture ? RenderTextureDimension::TEXTURE_3D : RenderTextureDimension::TEXTURE_2D;
    desc.width = width;
    desc.height = height;
    desc.mipLevels = levels;
    desc.format = ConvertFormat(format);
    texture->uploadFormat = desc.format;

#ifdef __ANDROID__
    if (!g_capabilities.textureCompressionBC &&
        (desc.format == RenderFormat::BC1_UNORM || desc.format == RenderFormat::BC3_UNORM))
    {
        static bool s_warned;
        if (!s_warned)
        {
            s_warned = true;
            LOGF("Guest BC texture (format 0x{:X}) will be decoded to RGBA8 on upload.", format);
        }

        desc.format = RenderFormat::R8G8B8A8_UNORM;
    }
#endif

    if (texture->type == ResourceType::ArrayTexture) {
        desc.arraySize = depth;
        desc.depth = 1;
    } else {
        desc.depth = depth;
        desc.arraySize = 1;
    }

    if (RenderFormatIsDepth(desc.format))
        desc.flags = RenderTextureFlag::DEPTH_TARGET;
    else if (usage != 0)
        desc.flags = RenderTextureFlag::RENDER_TARGET;
    else
        desc.flags = RenderTextureFlag::NONE;

    texture->textureHolder = g_device->createTexture(desc);
    texture->texture = texture->textureHolder.get();

    RenderTextureViewDesc viewDesc;
    viewDesc.format = desc.format;
    viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
    if (texture->type == ResourceType::VolumeTexture)
        viewDesc.dimension = RenderTextureViewDimension::TEXTURE_3D;
    else if (g_backend == Backend::VULKAN && texture->type == ResourceType::ArrayTexture && desc.arraySize > 1)
        viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
    viewDesc.mipLevels = levels;

    switch (format)
    {
    case D3DFMT_D24FS8:
    case D3DFMT_D24S8:
    case D3DFMT_L8:
    case D3DFMT_L8_2:
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::ONE);
        break;

    case D3DFMT_X8R8G8B8:
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::G, RenderSwizzle::B, RenderSwizzle::A, RenderSwizzle::ONE);
        break;
    }

    texture->textureView = texture->texture->createTextureView(viewDesc);

    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->format = desc.format;
    texture->mipLevels = viewDesc.mipLevels;
    texture->viewDimension = viewDesc.dimension;
    texture->colorRenderTarget = (desc.flags & RenderTextureFlag::RENDER_TARGET) != 0;
    texture->descriptorIndex = g_textureDescriptorAllocator.allocate();

    SetTextureDescriptor(texture->descriptorIndex, texture->texture, RenderTextureLayout::SHADER_READ, texture->textureView.get());
    CreateArrayTextureView(*texture, viewDesc);

#ifdef _DEBUG 
    texture->texture->setName(fmt::format("Texture {:X}", g_memory.MapVirtual(texture)));
#endif

    if (desc.flags != RenderTextureFlag::NONE)
    {
        DiscardTexture(texture, desc.flags == RenderTextureFlag::RENDER_TARGET ?
            RenderTextureLayout::COLOR_WRITE : RenderTextureLayout::DEPTH_WRITE);
    }

    // printf("CreateTexture: w: %d, h: %d, depth: %d, levels: %d, usage: %d, format: %d, pool: %d, type: %d - %x\n", width, height, depth, levels, usage, format, pool, type, texture);
    return texture;
}

static RenderHeapType GetBufferHeapType()
{
    return g_capabilities.gpuUploadHeap ? RenderHeapType::GPU_UPLOAD : RenderHeapType::DEFAULT;
}

static GuestBuffer* CreateVertexBuffer(uint32_t length) 
{
    auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::VertexBuffer);
    buffer->buffer = g_device->createBuffer(RenderBufferDesc::VertexBuffer(length, GetBufferHeapType(), RenderBufferFlag::INDEX));
    buffer->dataSize = length;
#ifdef _DEBUG 
    buffer->buffer->setName(fmt::format("Vertex Buffer {:X}", g_memory.MapVirtual(buffer)));
#endif
    return buffer;
}

static GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t, uint32_t format)
{
    auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::IndexBuffer);
    buffer->buffer = g_device->createBuffer(RenderBufferDesc::IndexBuffer(length, GetBufferHeapType()));
    buffer->dataSize = length;
    buffer->format = ConvertFormat(format);
    buffer->guestFormat = format;
#ifdef _DEBUG 
    buffer->buffer->setName(fmt::format("Index Buffer {:X}", g_memory.MapVirtual(buffer)));
#endif
    return buffer;
}

static RenderTextureDesc MakeSurfaceTextureDesc(uint32_t width, uint32_t height, RenderFormat format, RenderSampleCounts sampleCount)
{
    RenderTextureDesc desc;
    desc.dimension = RenderTextureDimension::TEXTURE_2D;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.multisampling.sampleCount = sampleCount;
    desc.format = format;
    desc.flags = RenderFormatIsDepth(format) ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;
    return desc;
}

const char* g_surfaceCreationName;

static SurfaceVariant* AcquireSurfaceVariant(uint32_t base, uint32_t width, uint32_t height,
    uint32_t guestFormat, RenderSampleCounts sampleCount, bool& created)
{
    std::lock_guard lock(g_surfaceVariantMutex);

    for (auto& variant : g_surfaceVariants)
    {
        if (variant->base == base &&
            variant->width == width &&
            variant->height == height &&
            variant->guestFormat == guestFormat &&
            variant->sampleCount == sampleCount)
        {
            variant->refCount++;
            created = false;

#ifdef _DEBUG
            LOGF_UTILITY("[surface] shared variant: base {} {}x{} format {:X} refs {} name {}",
                base, width, height, guestFormat, variant->refCount,
                g_surfaceCreationName ? g_surfaceCreationName : "<none>");
#endif

            return variant.get();
        }
    }

#ifdef _DEBUG
    LOGF_UTILITY("[surface] new variant: base {} {}x{} format {:X} samples {} name {}",
        base, width, height, guestFormat, uint32_t(sampleCount),
        g_surfaceCreationName ? g_surfaceCreationName : "<none>");
#endif

    auto& variant = g_surfaceVariants.emplace_back(std::make_unique<SurfaceVariant>());
    variant->base = base;
    variant->width = width;
    variant->height = height;
    variant->guestFormat = guestFormat;
    variant->sampleCount = sampleCount;
    variant->format = ConvertFormat(guestFormat);
    variant->refCount = 1;

    const auto desc = MakeSurfaceTextureDesc(width, height, variant->format, sampleCount);
    variant->textureHolder = g_device->createTexture(desc);

    RenderTextureViewDesc viewDesc;
    viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
    viewDesc.format = variant->format;
    viewDesc.mipLevels = 1;
    variant->textureView = variant->textureHolder->createTextureView(viewDesc);
    variant->descriptorIndex = g_textureDescriptorAllocator.allocate();
    SetTextureDescriptor(variant->descriptorIndex, variant->textureHolder.get(), RenderTextureLayout::SHADER_READ, variant->textureView.get());

#ifdef _DEBUG
    variant->textureHolder->setName(fmt::format("{} EDRAM {} {}x{} {}x",
        RenderFormatIsDepth(variant->format) ? "Depth Stencil" : "Render Target",
        base, width, height, int32_t(sampleCount)));
#endif

    created = true;
    return variant.get();
}

static GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample, GuestSurfaceCreateParams* params)
{
#ifdef __ANDROID__
    // The Android renderer only creates single-sample surfaces, including guest requests
    // that bypass the game's SetMSAALevel hook.
    (void)multiSample;
    const auto sampleCount = RenderSampleCount::COUNT_1;
#else
    const auto sampleCount = multiSample == 0 ? RenderSampleCount::COUNT_1 :
        (multiSample == 1 ? RenderSampleCount::COUNT_2 : RenderSampleCount::COUNT_4);
#endif

    const RenderFormat renderFormat = ConvertFormat(format);
    const bool isDepthStencil = RenderFormatIsDepth(renderFormat);

    const auto surface = g_userHeap.AllocPhysical<GuestSurface>(isDepthStencil ?
        ResourceType::DepthStencil : ResourceType::RenderTarget);

    surface->width = width;
    surface->height = height;
    surface->format = renderFormat;
    surface->guestFormat = format;
    surface->sampleCount = sampleCount;

    bool created = false;

    if (params != nullptr)
    {
        // The view shares the variant's host resources without owning them.
        surface->variant = AcquireSurfaceVariant(params->base, width, height, format, sampleCount, created);
        surface->texture = surface->variant->textureHolder.get();
        surface->descriptorIndex = surface->variant->descriptorIndex;
    }
    else
    {
        // No EDRAM placement information: the surface owns its host texture.
        const auto desc = MakeSurfaceTextureDesc(width, height, renderFormat, sampleCount);
        surface->textureHolder = g_device->createTexture(desc);
        surface->texture = surface->textureHolder.get();

        RenderTextureViewDesc viewDesc;
        viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
        viewDesc.format = renderFormat;
        viewDesc.mipLevels = 1;
        surface->textureView = surface->textureHolder->createTextureView(viewDesc);
        surface->descriptorIndex = g_textureDescriptorAllocator.allocate();
        SetTextureDescriptor(surface->descriptorIndex, surface->textureHolder.get(), RenderTextureLayout::SHADER_READ, surface->textureView.get());

    #ifdef _DEBUG
        surface->texture->setName(fmt::format("{} {:X}", isDepthStencil ? "Depth Stencil" : "Render Target", g_memory.MapVirtual(surface)));
    #endif

        created = true;
    }

    if (created)
    {
        DiscardTexture(surface, isDepthStencil ?
            RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE);
    }

    return surface;
}

static void FlushViewport()
{
    auto& commandList = g_commandLists[g_frame];

    if (g_dirtyStates.viewport)
    {
        auto viewport = g_viewport;

        // if (viewport.minDepth > viewport.maxDepth)
        //     std::swap(viewport.minDepth, viewport.maxDepth);

        commandList->setViewports(viewport);

        g_dirtyStates.viewport = false;
    }

    if (g_dirtyStates.scissorRect)
    {
        auto scissorRect = g_scissorTestEnable ? g_scissorRect : RenderRect(
            g_viewport.x,
            g_viewport.y,
            g_viewport.x + g_viewport.width,
            g_viewport.y + g_viewport.height);

        commandList->setScissors(scissorRect);

        g_dirtyStates.scissorRect = false;
    }
}

static void StretchRect(uint32_t flags, GuestTexture* texture, uint32_t destSliceOrFace, uint32_t guestReturnAddress = 0)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::StretchRect;
    cmd.stretchRect.flags = flags;
    cmd.stretchRect.texture = texture;
    cmd.stretchRect.destSliceOrFace = destSliceOrFace;
    cmd.stretchRect.guestReturnAddress = guestReturnAddress;
    EnqueueRenderCommand(cmd);
}

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture);
static void SetSurface(uint32_t index, GuestSurface* surface);

static void ProcStretchRect(const RenderCommand& cmd)
{
    const auto& args = cmd.stretchRect;

    const bool isDepthStencil = (args.flags & 0x4) != 0;
    const auto surface = isDepthStencil ? g_depthStencil : g_renderTarget;

#ifdef __ANDROID__
    if (isDepthStencil && g_depthCopyTraceRemaining != 0)
    {
        if (args.texture->type == ResourceType::ArrayTexture)
        {
            const auto previous = args.texture->sourceSurface;
            const bool replacesPending = previous != nullptr && previous->destinationTextures.contains(args.texture);
            const auto& trace = g_depthCopyTrace;
            const auto destinationAddress = g_memory.MapVirtual(args.texture);
            const auto& planned = trace.plannedDestination;
            const bool planMatches = planned.textureAddress == destinationAddress &&
                planned.layer == args.destSliceOrFace;
            LOGF("[depth-copy] frame={} guestReturn=0x{:08X} source=0x{:08X} edram={} sourceSize={}x{} "
                "destination=0x{:08X} layer={}/{} destinationSize={}x{} draws={} stencilDraws={} "
                "depthClears={} stencilClears={} depthClearedBeforeDraw={} replacesPending={} "
                "plannedDestination=0x{:08X} plannedLayer={} planMatches={} "
                "destinationBoundDuringDraw={} colorAttachmentDuringDraw={}",
                g_depthCopyTraceFrame, args.guestReturnAddress, g_memory.MapVirtual(surface),
                surface->variant != nullptr ? fmt::format("0x{:X}", surface->variant->base) : "none",
                surface->width, surface->height, destinationAddress, args.destSliceOrFace,
                args.texture->depth, args.texture->width, args.texture->height, trace.draws, trace.stencilDraws,
                trace.depthClears, trace.stencilClears, trace.depthClearedBeforeDraw, replacesPending,
                planned.textureAddress, planned.layer, planMatches, trace.destinationBoundDuringDraw,
                trace.colorAttachmentDuringDraw);

            if (--g_depthCopyTraceRemaining == 0)
                LOG("[depth-copy] Trace limit reached.");
        }

        g_depthCopyTrace = {};
    }
#endif

    // Already rendered into the destination slice, so only the texture slots need rebinding.
    if (surface->sliceTexture == args.texture && surface->slice == args.destSliceOrFace)
    {
#ifdef __ANDROID__
        ++g_directCsmSliceSkips[g_frame];
#endif
        for (uint32_t i = 0; i < std::size(g_textures); i++)
        {
            if (g_textures[i] == args.texture)
                SetTextureInRenderThread(i, args.texture);
        }

        return;
    }

    if (surface->sliceTexture != nullptr)
    {
        surface->sliceTexture->sliceSurfaces.erase(surface);
        ClearSliceRedirect(surface);
    }

    // Erase previous pending command so it doesn't cause the texture to be overriden.
    if (args.texture->sourceSurface != nullptr)
        args.texture->sourceSurface->destinationTextures.erase(args.texture);

    args.texture->sourceSurface = surface;
    // printf("ProcStretchRect: surface - %x %x ? (%x : %x)\n", surface, isDepthStencil, g_depthStencil, g_renderTarget);
    surface->destinationTextures.emplace(args.texture, args.destSliceOrFace);

    // If the texture is assigned to any slots, set it again. This'll also push the barrier.
    for (uint32_t i = 0; i < std::size(g_textures); i++)
    {
        if (g_textures[i] == args.texture)
        {
            // Set the original texture for MSAA and surface-to-array textures as they always get resolved.
            if (surface->sampleCount != RenderSampleCount::COUNT_1 ||
                args.texture->type == ResourceType::ArrayTexture)
            {
                SetTextureInRenderThread(i, args.texture);
                g_pendingResolves.emplace(surface);
            }
            else
            {
                SetSurface(i, surface);
            }
        }
    }

    // Remember to clear later.
    g_pendingSurfaceCopies.emplace(surface);
}

static void SetDefaultViewport(GuestDevice* device, GuestSurface* surface)
{
    if (surface != nullptr)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::SetViewport;
        cmd.setViewport.x = 0.0f;
        cmd.setViewport.y = 0.0f;
        cmd.setViewport.width = float(surface->width);
        cmd.setViewport.height = float(surface->height);
        cmd.setViewport.minDepth = 0.0f;
        cmd.setViewport.maxDepth = 1.0f;
        EnqueueRenderCommand(cmd);

        device->viewport.x = 0.0f;
        device->viewport.y = 0.0f;
        device->viewport.width = float(surface->width);
        device->viewport.height = float(surface->height);
        device->viewport.minZ = 0.0f;
        device->viewport.maxZ = 1.0f;
    }
}

static void SetRenderTarget(GuestDevice* device, uint32_t index, GuestSurface* renderTarget) 
{
    if (index == 0)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::SetRenderTarget;
        cmd.setRenderTarget.renderTarget = renderTarget;
        EnqueueRenderCommand(cmd);

        SetDefaultViewport(device, renderTarget);
    }
    else
    {
        // Multiple targets are not currently handled. Make sure any attempt to set them is nullptr.
        assert(renderTarget == nullptr);
    }
}

static void ProcSetRenderTarget(const RenderCommand& cmd)
{
    const auto& args = cmd.setRenderTarget;
    SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, args.renderTarget);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat, args.renderTarget != nullptr ? args.renderTarget->format : RenderFormat::UNKNOWN);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount, args.renderTarget != nullptr ? args.renderTarget->sampleCount : RenderSampleCount::COUNT_1);

    // When alpha to coverage is enabled, update the alpha test mode as it's dependent on sample count.
    SetAlphaTestMode((g_pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);
}

static void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil, DepthCopyDestination plannedDestination)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetDepthStencilSurface;
    cmd.setDepthStencilSurface.depthStencil = depthStencil;
    cmd.setDepthStencilSurface.plannedDestination = plannedDestination;
    EnqueueRenderCommand(cmd);

    SetDefaultViewport(device, depthStencil);
}

static void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil)
{
    SetDepthStencilSurface(device, depthStencil, {});
}

static void ProcSetDepthStencilSurface(const RenderCommand& cmd)
{
    const auto& args = cmd.setDepthStencilSurface;

#ifdef __ANDROID__
    if (g_depthCopyTraceRemaining != 0)
    {
        const auto& previous = g_depthCopyTrace.plannedDestination;
        const auto& next = args.plannedDestination;
        if (g_depthStencil != args.depthStencil ||
            (next.textureAddress != 0 && (next.textureAddress != previous.textureAddress || next.layer != previous.layer)))
        {
            g_depthCopyTrace = {};
        }

        if (next.textureAddress != 0)
            g_depthCopyTrace.plannedDestination = next;
    }
#endif

    SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, args.depthStencil);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat, args.depthStencil != nullptr ? args.depthStencil->format : RenderFormat::UNKNOWN);
}

static bool IsFullSurfaceCopy(const GuestSurface* surface, const GuestTexture* texture)
{
    return surface->sampleCount == RenderSampleCount::COUNT_1 &&
        surface->texture != nullptr && texture->texture != nullptr && surface->texture != texture->texture &&
        surface->format == texture->format && surface->width == texture->width && surface->height == texture->height;
}

static bool IsFullDepthCopy(const GuestSurface* surface, const GuestTexture* texture)
{
    return RenderFormatIsDepth(surface->format) && IsFullSurfaceCopy(surface, texture);
}

static bool IsFullDepthSliceCopy(const GuestSurface* surface, const GuestTexture* texture, uint32_t slice)
{
    return IsFullDepthCopy(surface, texture) &&
        ((texture->viewDimension == RenderTextureViewDimension::TEXTURE_2D_ARRAY && slice < texture->depth) ||
         (texture->viewDimension == RenderTextureViewDimension::TEXTURE_2D && slice == 0));
}

static bool CanTransferColorCopy(const GuestSurface* surface, const GuestTexture* texture, uint32_t slice)
{
    // The shader reads the surface's descriptor. A rebound backbuffer can point at
    // another host image while the descriptor still describes its original storage.
    const auto descriptorTexture = surface->variant != nullptr ?
        surface->variant->textureHolder.get() : surface->textureHolder.get();
    const auto variant = surface->variant;
    const bool descriptorMatches = surface->texture == descriptorTexture &&
        surface->descriptorIndex >= TEXTURE_DESCRIPTOR_NULL_COUNT &&
        (variant == nullptr || (variant->width == surface->width && variant->height == surface->height &&
            variant->format == surface->format));

    const bool isRgba8 = surface->format == RenderFormat::R8G8B8A8_UNORM ||
        surface->format == RenderFormat::B8G8R8A8_UNORM;
    return isRgba8 && IsFullSurfaceCopy(surface, texture) && descriptorMatches &&
        texture->colorRenderTarget && texture->texture == texture->textureHolder.get() &&
        texture->type == ResourceType::Texture && texture->viewDimension == RenderTextureViewDimension::TEXTURE_2D &&
        texture->depth == 1 && texture->mipLevels == 1 && slice == 0;
}

static bool PopulateBarriersForStretchRect(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    bool addedAny = false;

    for (const auto surface : { renderTarget, depthStencil })
    {
        if (surface != nullptr && !surface->destinationTextures.empty())
        {
            const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;
            const bool isDepthStencil = RenderFormatIsDepth(surface->format);
            addedAny = true;

            // Transfer and shader copies can be mixed in the same queue. Their transitions
            // are chosen at the copy, in order, to avoid a speculative layout change.
            if (!multiSampling && (isDepthStencil ? g_useDepthTransfer : g_useColorTransfer))
                continue;

            RenderTextureLayout srcLayout;
            RenderTextureLayout dstLayout;
            bool shaderResolve = true;

            if (multiSampling)
            {
                if (!isDepthStencil || g_capabilities.resolveModes)
                {
                    srcLayout = RenderTextureLayout::RESOLVE_SOURCE;
                    dstLayout = RenderTextureLayout::RESOLVE_DEST;
                    shaderResolve = false;
                }
            }

            if (shaderResolve)
            {
                srcLayout = RenderTextureLayout::SHADER_READ;
                dstLayout = isDepthStencil ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE;
            }

            for (const auto [texture, _] : surface->destinationTextures)
            {
                if (texture->sourceSurface != surface)
                    continue;

                AddBarrier(surface, srcLayout);
                AddBarrier(texture, dstLayout);
            }
        }
    }

    return addedAny;
}

static RenderFramebuffer* GetTextureFramebuffer(GuestTexture* texture, uint32_t slice)
{
    auto& framebuffer = texture->framebuffers[slice];
    if (framebuffer == nullptr)
    {
        if (RenderFormatIsDepth(texture->format))
        {
            RenderTextureViewDesc viewDesc;
            viewDesc.format = texture->format;
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
            viewDesc.mipLevels = 1;
            viewDesc.arrayIndex = slice;
            viewDesc.arraySize = 1;
            auto& view = texture->framebufferViews.emplace_back(texture->texture->createTextureView(viewDesc));

            RenderFramebufferDesc desc;
            desc.depthAttachmentView = view.get();
            framebuffer = g_device->createFramebuffer(desc);
        }
        else
        {
            RenderFramebufferDesc desc;
            desc.colorAttachments = const_cast<const RenderTexture**>(&texture->texture);
            desc.colorAttachmentsCount = 1;
            framebuffer = g_device->createFramebuffer(desc);
        }
    }

    return framebuffer.get();
}

static bool ExecutePendingStretchRectCommands(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    auto& commandList = g_commandLists[g_frame];

    for (const auto surface : { renderTarget, depthStencil })
    {
        if (surface != nullptr && !surface->destinationTextures.empty())
        {
            const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;
            const bool isDepthStencil = RenderFormatIsDepth(surface->format);
            const bool shaderResolve = !multiSampling || (isDepthStencil && !g_capabilities.resolveModes);
            const bool selectTransfer = !multiSampling && (isDepthStencil ? g_useDepthTransfer : g_useColorTransfer);
            RenderDescriptorSet* fixedSet = nullptr;
            if (shaderResolve && g_useFixedDescriptors && !selectTransfer)
            {
                fixedSet = GetFixedTextureSetForPass(surface->descriptorIndex);
                if (fixedSet == nullptr)
                    return false;
            }

            for (const auto [texture, slice] : surface->destinationTextures)
            {
                if (texture->sourceSurface != surface)
                    continue;

                const bool transfer = selectTransfer &&
                    (isDepthStencil ? IsFullDepthSliceCopy(surface, texture, slice) : CanTransferColorCopy(surface, texture, slice));
                if (selectTransfer)
                {
                    if (!transfer && g_useFixedDescriptors && fixedSet == nullptr)
                    {
                        fixedSet = GetFixedTextureSetForPass(surface->descriptorIndex);
                        if (fixedSet == nullptr)
                            return false;
                    }

                    // Layouts are tracked per image, not per layer. Revalidate even for a
                    // shader copy when another copy could have transitioned either image.
                    AddBarrier(surface, transfer ? RenderTextureLayout::COPY_SOURCE : RenderTextureLayout::SHADER_READ);
                    const auto attachmentLayout = isDepthStencil ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE;
                    AddBarrier(texture, transfer ? RenderTextureLayout::COPY_DEST : attachmentLayout,
                        transfer); // A previous transfer can have written the same layer.
                    FlushBarriers();
                }

                if (transfer)
                {
                    if (isDepthStencil)
                    {
                        commandList->copyTextureRegion(RenderTextureCopyLocation::DepthSubresource(texture->texture, 0, slice),
                            RenderTextureCopyLocation::DepthSubresource(surface->texture));
                    }
                    else
                    {
                        commandList->copyTextureRegion(RenderTextureCopyLocation::Subresource(texture->texture, 0, 0),
                            RenderTextureCopyLocation::Subresource(surface->texture, 0, 0));
                    }
                }
                else if (!shaderResolve)
                {
                    if (isDepthStencil)
                        commandList->resolveTextureRegion(texture->texture, 0, 0, surface->texture, nullptr, RenderResolveMode::MIN);
                    else
                        commandList->resolveTexture(texture->texture, surface->texture);
                }
                else
                {
                    RenderPipeline* pipeline = nullptr;

                    if (multiSampling)
                    {
                        uint32_t pipelineIndex = 0;

                        switch (surface->sampleCount)
                        {
                        case RenderSampleCount::COUNT_2:
                            pipelineIndex = 0;
                            break;
                        case RenderSampleCount::COUNT_4:
                            pipelineIndex = 1;
                            break;
                        case RenderSampleCount::COUNT_8:
                            pipelineIndex = 2;
                            break;
                        default:
                            assert(false && "Unsupported MSAA sample count");
                            break;
                        }

                        if (isDepthStencil)
                        {
                            pipeline = g_resolveMsaaDepthPipelines[pipelineIndex].get();
                        }
                        else
                        {
                            auto& resolveMsaaColorPipeline = g_resolveMsaaColorPipelines[surface->format][pipelineIndex];
                            if (resolveMsaaColorPipeline == nullptr)
                            {
                                RenderGraphicsPipelineDesc desc;
                                desc.pipelineLayout = g_pipelineLayout.get();
                                desc.vertexShader = g_copyShader.get();
                                desc.pixelShader = g_resolveMsaaColorShaders[pipelineIndex].get();
                                desc.renderTargetFormat[0] = texture->format;
                                desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
                                desc.renderTargetCount = 1;
                                resolveMsaaColorPipeline = g_device->createGraphicsPipeline(desc);
                                CheckBuiltInPipeline(resolveMsaaColorPipeline.get(), "resolve MSAA color");
                            }

                            pipeline = resolveMsaaColorPipeline.get();
                        }
                    }
                    else
                    {
                        if (isDepthStencil)
                        {
                            pipeline = g_copyDepthPipeline.get();
                        }
                        else
                        {
                            auto& copyColorPipeline = g_copyColorPipelines[texture->format];
                            if (copyColorPipeline == nullptr)
                            {
                                RenderGraphicsPipelineDesc desc;
                                desc.pipelineLayout = g_pipelineLayout.get();
                                desc.vertexShader = g_copyShader.get();
                                desc.pixelShader = g_copyColorShader.get();
                                desc.renderTargetFormat[0] = texture->format;
                                desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
                                desc.renderTargetCount = 1;
                                copyColorPipeline = g_device->createGraphicsPipeline(desc);
                                CheckBuiltInPipeline(copyColorPipeline.get(), "copy color");
                            }

                            pipeline = copyColorPipeline.get();
                        }
                    }

                    if (pipeline == nullptr || !pipeline->isValid())
                    {
                        static bool loggedInvalidCopyPipeline = false;
                        if (!loggedInvalidCopyPipeline)
                        {
                            LOGF_ERROR("Surface copy pipeline unavailable (depth={}, source samples={}).",
                                isDepthStencil, surface->sampleCount);
                            loggedInvalidCopyPipeline = true;
                        }
                        std::erase_if(surface->destinationTextures, [surface](const auto& entry)
                        {
                            return entry.first->sourceSurface != surface;
                        });
                        return false;
                    }

                    const auto framebuffer = GetTextureFramebuffer(texture, slice);

                    if (g_framebuffer != framebuffer)
                    {
                        commandList->setFramebuffer(framebuffer);
                        g_framebuffer = framebuffer;
                    }

                    if (framebuffer != nullptr &&
                        framebuffer->getWidth() == texture->width && framebuffer->getHeight() == texture->height)
                    {
                        if (!isDepthStencil)
                        {
                            // The color copy covers the entire attachment without blending.
                            commandList->discardColorAttachment(0);
                        }
                        else if (g_discardDepthCopyLoad && IsFullDepthCopy(surface, texture))
                        {
                            // The shader writes every destination depth texel. Stencil is preserved.
                            commandList->discardDepthAttachment();
                        }
                    }

                    commandList->setPipeline(pipeline);
                    commandList->setViewports(RenderViewport(0.0f, 0.0f, float(texture->width), float(texture->height), 0.0f, 1.0f));
                    commandList->setScissors(RenderRect(0, 0, texture->width, texture->height));
                    if (fixedSet != nullptr)
                        BindFixedDrawDescriptorSet(*commandList, fixedSet, 0);
                    else
                        commandList->setGraphicsPushConstants(0, &surface->descriptorIndex, 0, sizeof(uint32_t));
                    commandList->setRenderPassLabel(isDepthStencil ? GpuPassDepthCopy : GpuPassColorCopy);
                    commandList->drawInstanced(3, 1, 0, 0);
                    commandList->setRenderPassLabel(GpuPassGame);

                    g_dirtyStates.renderTargetAndDepthStencil = true;
                    g_dirtyStates.viewport = true;
                    g_dirtyStates.pipelineState = true;
                    g_dirtyStates.scissorRect = true;

                    if (g_backend != Backend::D3D12)
                    {
                        if (!g_optimizeDrawConstants)
                            g_dirtyStates.vertexShaderConstants = true;
                        else if (fixedSet == nullptr)
                            g_dirtyStates.vertexShaderConstantAddress = true;
                        g_dirtyStates.depthBias = true; // Static depth bias in copy pipeline invalidates dynamic depth bias.
                    }
                }

                texture->sourceSurface = nullptr;

                // Check if any texture slots had this texture assigned, and make it point back at the original texture.
                for (uint32_t i = 0; i < std::size(g_textures); i++)
                {
                    if (g_textures[i] == texture)
                        SetTextureInRenderThread(i, texture);
                }
            }

            // The slice now holds the surface's contents, so keep rendering there and skip future copies.
            if (surface->destinationTextures.size() == 1)
            {
                const auto [texture, slice] = *surface->destinationTextures.begin();

                if (CanRenderToSlice(surface, texture))
                    SetSliceRedirect(surface, texture, slice);
            }

            surface->destinationTextures.clear();
        }
    }

    return true;
}

static void ProcExecutePendingStretchRectCommands(const RenderCommand& cmd)
{
    bool foundAny = false;

    for (const auto surface : g_pendingSurfaceCopies)
    {
        // Depth stencil textures in this game are guaranteed to be transient.
        if (!RenderFormatIsDepth(surface->format))
            foundAny |= PopulateBarriersForStretchRect(surface, nullptr);
    }

    if (foundAny)
    {
        FlushBarriers();

        for (const auto surface : g_pendingSurfaceCopies)
        {
            if (!RenderFormatIsDepth(surface->format) && !ExecutePendingStretchRectCommands(surface, nullptr))
                return;

            for (const auto [texture, _] : surface->destinationTextures)
                texture->sourceSurface = nullptr;

            surface->destinationTextures.clear();
        }
    }

    g_pendingSurfaceCopies.clear();
    g_pendingResolves.clear();
}

static void SetFramebuffer(GuestSurface* renderTarget, GuestSurface* depthStencil, bool settingForClear)
{
    if (settingForClear || g_dirtyStates.renderTargetAndDepthStencil)
    {
        // printf("SetFramebuffer %x %x\n", renderTarget, depthStencil);
        GuestSurface* framebufferContainer = nullptr;
        RenderTexture* framebufferKey = nullptr;

        if (renderTarget != nullptr && depthStencil != nullptr)
        {
            framebufferContainer = depthStencil; // Backbuffer texture changes per frame so we can't use the depth stencil as the key.
            framebufferKey = renderTarget->texture;
        }
        else if (renderTarget != nullptr && depthStencil == nullptr)
        {
            framebufferContainer = renderTarget;
            framebufferKey = renderTarget->texture; // Backbuffer texture changes per frame so we can't assume nullptr for it.
        }
        else if (renderTarget == nullptr && depthStencil != nullptr)
        {
            framebufferContainer = depthStencil;
            framebufferKey = nullptr;
        }

        auto& commandList = g_commandLists[g_frame];

        if (depthStencil != nullptr && depthStencil->sliceTexture != nullptr)
        {
            assert(renderTarget == nullptr && "Rendering to a texture slice only supports depth-only framebuffers.");

            const auto framebuffer = GetTextureFramebuffer(depthStencil->sliceTexture, depthStencil->slice);

            if (g_framebuffer != framebuffer)
            {
                commandList->setFramebuffer(framebuffer);
                g_framebuffer = framebuffer;
            }
        }
        else if (framebufferContainer != nullptr)
        {
            auto& framebuffers = framebufferContainer->variant != nullptr ?
                framebufferContainer->variant->framebuffers : framebufferContainer->framebuffers;

            auto& framebuffer = framebuffers[framebufferKey];

            if (framebuffer == nullptr)
            {
                RenderFramebufferDesc desc;

                if (renderTarget != nullptr)
                {
                    desc.colorAttachments = const_cast<const RenderTexture**>(&renderTarget->texture);
                    desc.colorAttachmentsCount = 1;
                }

                if (depthStencil != nullptr)
                    desc.depthAttachment = depthStencil->texture;

                // With ResolutionScale the scene depth is smaller than the full-size back
                // buffer. A depth attachment smaller than the colour attachment is invalid in
                // Vulkan, so bind such a pair without depth (the game's back buffer passes are 2D).
                if (renderTarget != nullptr && depthStencil != nullptr &&
                    (depthStencil->width < renderTarget->width || depthStencil->height < renderTarget->height))
                {
                    static bool s_loggedSmallDepth;
                    if (!s_loggedSmallDepth)
                    {
                        LOGF_WARNING("Depth stencil {}x{} is smaller than render target {}x{}; binding without depth.",
                            depthStencil->width, depthStencil->height, renderTarget->width, renderTarget->height);
                        s_loggedSmallDepth = true;
                    }

                    desc.depthAttachment = nullptr;
                }

                framebuffer = g_device->createFramebuffer(desc);
            }

            if (g_framebuffer != framebuffer.get())
            {
                commandList->setFramebuffer(framebuffer.get());
                g_framebuffer = framebuffer.get();
            }
        }
        else if (g_framebuffer != nullptr)
        {
            commandList->setFramebuffer(nullptr);
            g_framebuffer = nullptr;
        }

        if (g_framebuffer != nullptr)
        {
            SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetX, 1.0f / float(g_framebuffer->getWidth()));
            SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetY, -1.0f / float(g_framebuffer->getHeight()));
        }

        g_dirtyStates.renderTargetAndDepthStencil = settingForClear;
    }
}

static void Clear(GuestDevice* device, uint32_t flags, uint32_t, be<float>* color, double z, uint32_t stencil)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::Clear;
    cmd.clear.flags = flags;
    cmd.clear.color[0] = color[0];
    cmd.clear.color[1] = color[1];
    cmd.clear.color[2] = color[2];
    cmd.clear.color[3] = color[3];
    cmd.clear.z = float(z);
    cmd.clear.stencil = stencil;
    EnqueueRenderCommand(cmd);
}

static bool SharesStorage(const GuestSurface* lhs, const GuestSurface* rhs)
{
    return lhs != nullptr && rhs != nullptr && lhs->texture == rhs->texture;
}

static bool FlushPendingCopiesForAliasedStorage(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    bool foundAny = false;

    for (const auto surface : g_pendingSurfaceCopies)
    {
        if (SharesStorage(surface, renderTarget) || SharesStorage(surface, depthStencil))
        {
            const bool isDepthStencil = RenderFormatIsDepth(surface->format);
            foundAny |= PopulateBarriersForStretchRect(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
        }
    }

    if (foundAny)
    {
        FlushBarriers();

        for (const auto surface : g_pendingSurfaceCopies)
        {
            if (SharesStorage(surface, renderTarget) || SharesStorage(surface, depthStencil))
            {
                const bool isDepthStencil = RenderFormatIsDepth(surface->format);
                if (!ExecutePendingStretchRectCommands(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr))
                    return false;
            }
        }
    }

    return true;
}

static void ProcClear(const RenderCommand& cmd)
{
    const auto& args = cmd.clear;

    if (!FlushPendingCopiesForAliasedStorage(g_renderTarget, g_depthStencil))
        return;

    AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
    FlushBarriers();

    bool canClearInOnePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
        (g_renderTarget->width == g_depthStencil->width && g_renderTarget->height == g_depthStencil->height);

    if (canClearInOnePass)
    {
        SetFramebuffer(g_renderTarget, g_depthStencil, true);
    }

    auto& commandList = g_commandLists[g_frame];

    bool clearColor = (args.flags & D3DCLEAR_TARGET) != 0;
    bool clearDepth = (args.flags & D3DCLEAR_ZBUFFER) != 0;
    bool clearStencil = (args.flags & D3DCLEAR_STENCIL) != 0;

    // Try to propogate clears to aliased texture variants.
    // Note, this contains a bit of a hack to avoid the main depth buf
    // getting cleared to a value that makes every pixel fail depth test.
    if (g_renderTarget != nullptr && g_renderTarget->sliceTexture == nullptr && g_renderTarget->variant != nullptr &&
        (g_renderTarget->variant->deferredClearFlags & D3DCLEAR_TARGET) != 0)
    {
        clearColor = true;
        g_renderTarget->variant->deferredClearFlags &= ~D3DCLEAR_TARGET;
    }

    if (g_depthStencil != nullptr && g_depthStencil->sliceTexture == nullptr && g_depthStencil->variant != nullptr &&
        (g_depthStencil->variant->deferredClearFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)) != 0)
    {
        clearDepth |= (g_depthStencil->variant->deferredClearFlags & D3DCLEAR_ZBUFFER) != 0;
        clearStencil |= (g_depthStencil->variant->deferredClearFlags & D3DCLEAR_STENCIL) != 0;
        g_depthStencil->variant->deferredClearFlags &= ~(D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
    }

    if (g_renderTarget != nullptr && clearColor)
    {
        if (!canClearInOnePass) {
            SetFramebuffer(g_renderTarget, nullptr, true);
        }

        commandList->clearColor(0, RenderColor(args.color[0], args.color[1], args.color[2], args.color[3]));
    }

    if (g_depthStencil != nullptr && (clearDepth || clearStencil))
    {
        if (!canClearInOnePass) {
            SetFramebuffer(nullptr, g_depthStencil, true);
        }

        commandList->clearDepthStencil(clearDepth, clearStencil, args.z, args.stencil);
#ifdef __ANDROID__
        if (g_depthCopyTraceRemaining != 0)
            g_depthCopyTrace.recordClear(clearDepth, clearStencil);
#endif
    }

    {
        std::lock_guard lock(g_surfaceVariantMutex);

        for (const auto boundSurface : { g_renderTarget, g_depthStencil })
        {
            if (boundSurface == nullptr || boundSurface->sliceTexture != nullptr || boundSurface->variant == nullptr)
                continue;

            const bool boundIsDepth = RenderFormatIsDepth(boundSurface->format);
            const uint32_t flags = args.flags & (boundIsDepth ? (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) : D3DCLEAR_TARGET);
            if (flags == 0)
                continue;

            // The variant the game just cleared itself needs no deferred clear.
            boundSurface->variant->deferredClearFlags &= ~flags;

            for (auto& variant : g_surfaceVariants)
            {
                if (variant.get() == boundSurface->variant ||
                    variant->base != boundSurface->variant->base ||
                    RenderFormatIsDepth(variant->format) != boundIsDepth ||
                    variant->width > boundSurface->variant->width ||
                    variant->height > boundSurface->variant->height)
                {
                    continue;
                }

                variant->deferredClearFlags |= flags;
                variant->deferredClearColor[0] = args.color[0];
                variant->deferredClearColor[1] = args.color[1];
                variant->deferredClearColor[2] = args.color[2];
                variant->deferredClearColor[3] = args.color[3];
                variant->deferredClearZ = args.z;
                variant->deferredClearStencil = args.stencil;
            }
        }
    }
}

static void ApplyDeferredVariantClears(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    auto& commandList = g_commandLists[g_frame];

    if (renderTarget != nullptr && renderTarget->sliceTexture == nullptr && renderTarget->variant != nullptr &&
        (renderTarget->variant->deferredClearFlags & D3DCLEAR_TARGET) != 0)
    {
        auto variant = renderTarget->variant;

        commandList->clearColor(0, RenderColor(variant->deferredClearColor[0], variant->deferredClearColor[1],
            variant->deferredClearColor[2], variant->deferredClearColor[3]));

        variant->deferredClearFlags &= ~D3DCLEAR_TARGET;
    }

    if (depthStencil != nullptr && depthStencil->sliceTexture == nullptr && depthStencil->variant != nullptr &&
        (depthStencil->variant->deferredClearFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)) != 0)
    {
        auto variant = depthStencil->variant;
        const bool clearDepth = (variant->deferredClearFlags & D3DCLEAR_ZBUFFER) != 0;
        const bool clearStencil = (variant->deferredClearFlags & D3DCLEAR_STENCIL) != 0;

        commandList->clearDepthStencil(clearDepth, clearStencil, variant->deferredClearZ, variant->deferredClearStencil);
#ifdef __ANDROID__
        if (g_depthCopyTraceRemaining != 0)
            g_depthCopyTrace.recordClear(clearDepth, clearStencil);
#endif

        variant->deferredClearFlags &= ~(D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
    }
}

static void SetViewport(GuestDevice* device, GuestViewport* viewport)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetViewport;
    cmd.setViewport.x = viewport->x;
    cmd.setViewport.y = viewport->y;
    cmd.setViewport.width = viewport->width;
    cmd.setViewport.height = viewport->height;
    cmd.setViewport.minDepth = viewport->minZ;
    cmd.setViewport.maxDepth = viewport->maxZ;
    EnqueueRenderCommand(cmd);

    device->viewport.x = float(viewport->x);
    device->viewport.y = float(viewport->y);
    device->viewport.width = float(viewport->width);
    device->viewport.height = float(viewport->height);
    device->viewport.minZ = viewport->minZ;
    device->viewport.maxZ = viewport->maxZ;
}

static void ProcSetViewport(const RenderCommand& cmd)
{
    const auto& args = cmd.setViewport;

    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, args.x);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, args.y);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, args.width);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, args.height);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, args.minDepth);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, args.maxDepth);
    
    uint32_t specConstants = g_pipelineState.specConstants;
    if (args.minDepth > args.maxDepth)
        specConstants |= SPEC_CONSTANT_REVERSE_Z;
    else 
        specConstants &= ~SPEC_CONSTANT_REVERSE_Z;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);

    g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}

static void SetTexture(GuestDevice* device, uint32_t index, GuestTexture* texture) 
{
    // printf("SetTexture: %x %d %x\n", device, index, texture);

    if (Config::GetPreferredControllerIcons() == EControllerIcons::PlayStation &&
        texture != nullptr && texture->patchedTexture != nullptr)
    {
        texture = texture->patchedTexture.get();
    }

    RenderCommand cmd;
    cmd.type = RenderCommandType::SetTexture;
    cmd.setTexture.index = index;
    cmd.setTexture.texture = texture;
    EnqueueRenderCommand(cmd);
}

static void UpdateGuestSampler(uint32_t slot)
{
    const RenderSamplerDesc* samplerDesc = &g_samplerDescs[slot];
#ifdef __ANDROID__
    RenderSamplerDesc pointSamplerDesc;
    const RenderFormat pointFormat = g_pointSamplerFormats[slot];
    const bool needsLinearFilter = samplerDesc->minFilter == RenderFilter::LINEAR ||
        samplerDesc->magFilter == RenderFilter::LINEAR || samplerDesc->mipmapMode == RenderMipmapMode::LINEAR ||
        samplerDesc->anisotropyEnabled;
    if (pointFormat != RenderFormat::UNKNOWN && needsLinearFilter)
    {
        // Keep the cached descriptor's object representation, including padding, for hashing.
        memcpy(&pointSamplerDesc, samplerDesc, sizeof(pointSamplerDesc));
        pointSamplerDesc.minFilter = RenderFilter::NEAREST;
        pointSamplerDesc.magFilter = RenderFilter::NEAREST;
        pointSamplerDesc.mipmapMode = RenderMipmapMode::NEAREST;
        pointSamplerDesc.anisotropyEnabled = false;
        pointSamplerDesc.maxAnisotropy = 1;
        samplerDesc = &pointSamplerDesc;

        static bool s_reportedPointFilter;
        if (!s_reportedPointFilter)
        {
            LOGF("GPU sampling: point filtering for {} (guest slot {}).", magic_enum::enum_name(pointFormat), slot);
            s_reportedPointFilter = true;
        }
    }
#endif

    auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(samplerDesc, sizeof(*samplerDesc))];
    if (descriptorIndex == 0)
    {
        descriptorIndex = g_samplerStates.size();
        sampler = g_device->createSampler(*samplerDesc);

        if (g_useFixedDescriptors)
            g_fixedSamplers.push_back(sampler.get());
        else
            g_samplerDescriptorSet->setSampler(descriptorIndex - 1 + g_samplerSlotOffset, sampler.get());
    }

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.samplerIndices[slot], descriptorIndex - 1);
}

#ifdef __ANDROID__
static void SetSamplerTextureFormat(uint32_t slot, RenderFormat format)
{
    const bool needsPointFilter = (format == RenderFormat::R32_FLOAT && g_pointFilterR32) ||
        (format == g_depthStencilFormat && g_pointFilterDepthStencil);
    const RenderFormat pointFormat = needsPointFilter ? format : RenderFormat::UNKNOWN;
    if (pointFormat != g_pointSamplerFormats[slot])
    {
        g_pointSamplerFormats[slot] = pointFormat;
        UpdateGuestSampler(slot);
    }
}
#endif

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture)
{
    AddBarrier(texture, RenderTextureLayout::SHADER_READ);
#ifdef __ANDROID__
    SetSamplerTextureFormat(index, texture != nullptr ? texture->format : RenderFormat::UNKNOWN);
#endif

    auto viewDimension = texture != nullptr ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index],
        viewDimension == RenderTextureViewDimension::TEXTURE_2D ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D);

    uint32_t arrayDescriptorIndex = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY;
    if (viewDimension == RenderTextureViewDimension::TEXTURE_2D_ARRAY)
        arrayDescriptorIndex = texture->descriptorIndex;
    else if (viewDimension == RenderTextureViewDimension::TEXTURE_2D)
        arrayDescriptorIndex = texture->arrayDescriptorIndex != NULL ? texture->arrayDescriptorIndex : texture->descriptorIndex;

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DArrayIndices[index], arrayDescriptorIndex);

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], texture != nullptr &&
        viewDimension == RenderTextureViewDimension::TEXTURE_CUBE ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE);
}

static void SetSurface(uint32_t index, GuestSurface* surface)
{
    AddBarrier(surface, RenderTextureLayout::SHADER_READ);
#ifdef __ANDROID__
    SetSamplerTextureFormat(index, surface->format);
#endif

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index], surface->descriptorIndex);
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DArrayIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D_ARRAY));
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE));
}

static void ProcSetTexture(const RenderCommand& cmd)
{
    const auto& args = cmd.setTexture;

    // If a pending copy operation is detected, set the source surface. The indices will be fixed later if flushing is necessary.
    bool shouldSetTexture = true;
    if (args.texture != nullptr && args.texture->sourceSurface != nullptr)
    {
        // MSAA surfaces or surface-to-array need to be resolved and cannot be used directly.
        if (args.texture->sourceSurface->sampleCount != RenderSampleCount::COUNT_1 ||
            args.texture->type == ResourceType::ArrayTexture)
        {
            g_pendingResolves.emplace(args.texture->sourceSurface);
        }
        else
        {
            SetSurface(args.index, args.texture->sourceSurface);
            shouldSetTexture = false;
        }
    }
    
    if (shouldSetTexture)
        SetTextureInRenderThread(args.index, args.texture);
    
    g_textures[args.index] = args.texture;
}

static void SetScissorRect(GuestDevice* device, GuestRect* rect)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetScissorRect;
    cmd.setScissorRect.top = rect->top;
    cmd.setScissorRect.left = rect->left;
    cmd.setScissorRect.bottom = rect->bottom;
    cmd.setScissorRect.right = rect->right;
    EnqueueRenderCommand(cmd);
}

static void ProcSetScissorRect(const RenderCommand& cmd)
{
    const auto& args = cmd.setScissorRect;

    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.top, args.top);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.left, args.left);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.bottom, args.bottom);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.right, args.right);
}

static RenderShader* GetReplacementShader(GuestShader* guestShader)
{
    if (guestShader == nullptr)
        return nullptr;

    if (guestShader->shaderCacheEntry != nullptr)
    {
        switch (guestShader->shaderCacheEntry->hash)
        {
        case 0x3687D038CE7D0BEA:
        case 0xB4DA7A442DBB16CC:
            if (Config::GetEffectiveRadialBlur() == ERadialBlur::Enhanced)
                return g_enhancedBurnoutBlurVSShader.get();
            break;
        case 0xDA58F0110A8595D9:
        case 0x845A4EF989446C01:
            if (Config::GetEffectiveRadialBlur() == ERadialBlur::Enhanced)
                return g_enhancedBurnoutBlurPSShader.get();
            break;
        case 0x88EB0AA82E4257B0:
            return g_MoviePSShader.get();
        default:
            break;
        }
    }

    return nullptr;
}

static RenderShader* GetOrLinkShader(GuestShader* guestShader, uint32_t specConstants)
{
    auto* replacementShader = GetReplacementShader(guestShader);
    if (replacementShader != nullptr)
        return replacementShader;

    if (g_backend != Backend::D3D12 ||
        guestShader->shaderCacheEntry == nullptr ||
        guestShader->shaderCacheEntry->specConstantsMask == 0)
    {
        std::lock_guard lock(guestShader->mutex);

        if (guestShader->shader == nullptr)
        {
            assert(guestShader->shaderCacheEntry != nullptr);

            switch (g_backend) {
            case Backend::VULKAN:
            {
                const auto& entry = *guestShader->shaderCacheEntry;
                uint32_t offset = entry.spirvOffset;
                uint32_t size = entry.spirvSize;
                if (g_useBufferConstants)
                {
                    const unsigned variant = BUFFER_CONSTANTS_SHADER_VARIANT + unsigned(g_useSoftwareClip);
                    offset = entry.androidSpirvOffsets[variant];
                    size = entry.androidSpirvSizes[variant];
                }
                else if (const unsigned variant = unsigned(g_useFixedDescriptors) + 2 * unsigned(g_useSoftwareClip); variant != 0)
                {
                    offset = entry.androidSpirvOffsets[variant - 1];
                    size = entry.androidSpirvSizes[variant - 1];
                }

                const auto* compressedSpirvData = g_shaderCache.get() + offset;
                std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, size));
                if (!smolv::Decode(compressedSpirvData, size, decoded.data(), decoded.size()))
                {
                    LOGF_ERROR("Could not decode SPIR-V for shader {:016X}.", entry.hash);
                    return nullptr;
                }

                guestShader->shader = CreateSpirvShader(decoded.data(), decoded.size());
                break;
            }
            case Backend::D3D12:
            {
                guestShader->shader = g_device->createShader(g_shaderCache.get() + guestShader->shaderCacheEntry->dxilOffset,
                    guestShader->shaderCacheEntry->dxilSize, "shaderMain", RenderShaderFormat::DXIL);
                break;
            }
            case Backend::METAL:
            {
                guestShader->shader = g_device->createShader(g_shaderCache.get() + guestShader->shaderCacheEntry->airOffset,
                    guestShader->shaderCacheEntry->airSize, "shaderMain", RenderShaderFormat::METAL);
                break;
            }
            }

#ifdef _DEBUG
            guestShader->shader->setName(fmt::format("{}:{:x}", guestShader->shaderCacheEntry->filename, guestShader->shaderCacheEntry->hash));
#endif
        }

        return guestShader->shader.get();
    }

    specConstants &= guestShader->shaderCacheEntry->specConstantsMask;

    RenderShader* shader;
    {
        std::lock_guard lock(guestShader->mutex);
        shader = guestShader->linkedShaders[specConstants].get();
    }

#ifdef MARATHON_RECOMP_D3D12
    if (shader == nullptr)
    {
        static Mutex g_compiledSpecConstantLibraryBlobMutex;
        static ankerl::unordered_dense::map<uint32_t, ComPtr<IDxcBlob>> g_compiledSpecConstantLibraryBlobs;

        thread_local ComPtr<IDxcCompiler3> s_dxcCompiler;
        thread_local ComPtr<IDxcLinker> s_dxcLinker;
        thread_local ComPtr<IDxcUtils> s_dxcUtils;

        wchar_t specConstantsLibName[0x100];
        swprintf_s(specConstantsLibName, L"SpecConstants_%d", specConstants);

        ComPtr<IDxcBlob> specConstantLibraryBlob;
        {
            std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
            specConstantLibraryBlob = g_compiledSpecConstantLibraryBlobs[specConstants];
        }

        if (specConstantLibraryBlob == nullptr)
        {
            if (s_dxcCompiler == nullptr)
            {
                HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_dxcCompiler.GetAddressOf()));
                assert(SUCCEEDED(hr) && s_dxcCompiler != nullptr);
            }

            char libraryHlsl[0x100];
            sprintf_s(libraryHlsl, "export uint g_SpecConstants() { return %d; }", specConstants);

            DxcBuffer buffer{};
            buffer.Ptr = libraryHlsl;
            buffer.Size = strlen(libraryHlsl);

            const wchar_t* args[1];
            args[0] = L"-T lib_6_3";

            ComPtr<IDxcResult> result;
            HRESULT hr = s_dxcCompiler->Compile(&buffer, args, std::size(args), nullptr, IID_PPV_ARGS(result.GetAddressOf()));
            assert(SUCCEEDED(hr) && result != nullptr);

            hr = result->GetResult(specConstantLibraryBlob.GetAddressOf());
            assert(SUCCEEDED(hr) && specConstantLibraryBlob != nullptr);

            std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
            g_compiledSpecConstantLibraryBlobs.emplace(specConstants, specConstantLibraryBlob);
        }

        if (s_dxcLinker == nullptr)
        {
            HRESULT hr = DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(s_dxcLinker.GetAddressOf()));
            assert(SUCCEEDED(hr) && s_dxcLinker != nullptr);
        }

        s_dxcLinker->RegisterLibrary(specConstantsLibName, specConstantLibraryBlob.Get());

        wchar_t shaderLibName[0x100];
        swprintf_s(shaderLibName, L"Shader_%d", guestShader->shaderCacheEntry->dxilOffset);

        ComPtr<IDxcBlobEncoding> shaderLibraryBlob;
        {
            std::lock_guard lock(guestShader->mutex);
            shaderLibraryBlob = guestShader->libraryBlob;
        }

        if (shaderLibraryBlob == nullptr)
        {
            if (s_dxcUtils == nullptr)
            {
                HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_dxcUtils.GetAddressOf()));
                assert(SUCCEEDED(hr) && s_dxcUtils != nullptr);
            }

            HRESULT hr = s_dxcUtils->CreateBlobFromPinned(
                g_shaderCache.get() + guestShader->shaderCacheEntry->dxilOffset,
                guestShader->shaderCacheEntry->dxilSize,
                DXC_CP_ACP,
                shaderLibraryBlob.GetAddressOf());

            assert(SUCCEEDED(hr) && shaderLibraryBlob != nullptr);

            std::lock_guard lock(guestShader->mutex);
            guestShader->libraryBlob = shaderLibraryBlob;
        }

        s_dxcLinker->RegisterLibrary(shaderLibName, shaderLibraryBlob.Get());

        const wchar_t* libraryNames[] = { specConstantsLibName, shaderLibName };

        ComPtr<IDxcOperationResult> result;
        HRESULT hr = s_dxcLinker->Link(L"shaderMain", guestShader->type == ResourceType::VertexShader ? L"vs_6_0" : L"ps_6_0",
            libraryNames, std::size(libraryNames), nullptr, 0, result.GetAddressOf());

        assert(SUCCEEDED(hr) && result != nullptr);

        ComPtr<IDxcBlob> blob;
        hr = result->GetResult(blob.GetAddressOf());
        assert(SUCCEEDED(hr) && blob != nullptr);

        {
            std::lock_guard lock(guestShader->mutex);

            auto& linkedShader = guestShader->linkedShaders[specConstants];
            if (linkedShader == nullptr)
            {
                linkedShader = g_device->createShader(blob->GetBufferPointer(), blob->GetBufferSize(), "shaderMain", RenderShaderFormat::DXIL);
                guestShader->shaderBlobs.push_back(std::move(blob));
            }

            shader = linkedShader.get();

#ifdef _DEBUG
            shader->setName(fmt::format("{}:{:x}", guestShader->shaderCacheEntry->filename, guestShader->shaderCacheEntry->hash));
#endif
        }        
    }
#endif

    return shader;
}

static XXH64_hash_t GetPipelineStateHash(const PipelineState& pipelineState)
{
    XXH64_hash_t seed = 0;

    // If shaders need to be replaced, make sure they result in a different pipeline hash.
    struct {
        RenderShader* vertex;
        RenderShader* pixel;
    } replacements(GetReplacementShader(pipelineState.vertexShader), GetReplacementShader(pipelineState.pixelShader));
    if (replacements.vertex != nullptr || replacements.pixel != nullptr)
        seed = XXH3_64bits(&replacements, sizeof(replacements));

    return XXH3_64bits_withSeed(&pipelineState, sizeof(pipelineState), seed);
}

static void SanitizePipelineState(PipelineState& pipelineState)
{
    if (!pipelineState.zEnable && !pipelineState.stencilEnable)
    {
        pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
    }
    else if (RenderFormatIsStencil(pipelineState.depthStencilFormat))
    {
        pipelineState.depthStencilFormat = g_depthStencilFormat;
    }

    if (!pipelineState.zEnable)
    {
        pipelineState.zWriteEnable = false;
        pipelineState.zFunc = RenderComparisonFunction::LESS;
        pipelineState.slopeScaledDepthBias = 0.0f;
        pipelineState.depthBias = 0;
    }

    if (!pipelineState.stencilEnable)
    {
        pipelineState.stencilTwoSided = false;
        pipelineState.stencilFunc = RenderComparisonFunction::ALWAYS;
        pipelineState.stencilFail = RenderStencilOp::KEEP;
        pipelineState.stencilZFail = RenderStencilOp::KEEP;
        pipelineState.stencilPass = RenderStencilOp::KEEP;
        pipelineState.stencilMask = 0xFFFFFFFF;
        pipelineState.stencilWriteMask = 0xFFFFFFFF;
        pipelineState.stencilRef = 0;
    }

    if (!pipelineState.stencilTwoSided)
    {
        pipelineState.stencilFuncCCW = pipelineState.stencilFunc;
        pipelineState.stencilFailCCW = pipelineState.stencilFail;
        pipelineState.stencilZFailCCW = pipelineState.stencilZFail;
        pipelineState.stencilPassCCW = pipelineState.stencilPass;
    }

    if (pipelineState.slopeScaledDepthBias == 0.0f)
        pipelineState.slopeScaledDepthBias = 0.0f; // Remove sign.

    if (!pipelineState.colorWriteEnable)
    {
        pipelineState.alphaBlendEnable = false;
        pipelineState.renderTargetFormat = RenderFormat::UNKNOWN;
    }

    if (!pipelineState.alphaBlendEnable)
    {
        pipelineState.srcBlend = RenderBlend::ONE;
        pipelineState.destBlend = RenderBlend::ZERO;
        pipelineState.blendOp = RenderBlendOperation::ADD;
        pipelineState.srcBlendAlpha = RenderBlend::ONE;
        pipelineState.destBlendAlpha = RenderBlend::ZERO;
        pipelineState.blendOpAlpha = RenderBlendOperation::ADD;
    }

    for (size_t i = 0; i < 16; i++)
    {
        if (!pipelineState.vertexDeclaration->vertexStreams[i])
            pipelineState.vertexStrides[i] = 0;
    }

    uint32_t specConstantsMask = g_useSoftwareClip ? SPEC_CONSTANT_CLIP_PLANE : 0;
    if (pipelineState.vertexShader->shaderCacheEntry != nullptr)
        specConstantsMask |= pipelineState.vertexShader->shaderCacheEntry->specConstantsMask;

    if (pipelineState.pixelShader != nullptr && pipelineState.pixelShader->shaderCacheEntry != nullptr)
        specConstantsMask |= pipelineState.pixelShader->shaderCacheEntry->specConstantsMask;

    pipelineState.specConstants &= specConstantsMask;
}

#ifdef __ANDROID__
// The driver may explain a rejected pipeline only in logcat, which testers without adb cannot
// read. An app may read its own logcat lines, so the first failure copies them into log.txt.
static void DumpOwnLogcatOnce(const char* reason)
{
    static std::atomic<bool> s_dumped{ false };
    if (s_dumped.exchange(true))
        return;

    char command[128];
    snprintf(command, sizeof(command), "logcat -d -v brief -t 400 --pid=%d 2>/dev/null", int(getpid()));

    FILE* pipe = popen(command, "r");
    if (pipe == nullptr)
    {
        LOGF_WARNING("Could not read logcat after {}: {}", reason, strerror(errno));
        return;
    }

    LOGF("Logcat lines of this process after {} (own log lines skipped):", reason);

    char line[1024];
    uint32_t printed = 0;
    while (printed < 150 && fgets(line, sizeof(line), pipe) != nullptr)
    {
        if (strstr(line, "MarathonRecomp") != nullptr)
            continue;

        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';

        if (length == 0)
            continue;

        LOGF("[logcat] {}", line);
        ++printed;
    }

    pclose(pipe);
    LOGF("End of logcat lines ({}).", printed);
}

static void DescribeShader(std::string& out, const char* label, const GuestShader* shader)
{
    out += label;
    if (shader == nullptr)
    {
        out += "none";
    }
    else if (shader->shaderCacheEntry != nullptr)
    {
        out += fmt::format("{:016X} \"{}\"", shader->shaderCacheEntry->hash, shader->shaderCacheEntry->filename);
    }
    else
    {
        out += "built-in";
    }
}

static void DescribeGamePipeline(std::string& text, const PipelineState& pipelineState, const RenderGraphicsPipelineDesc& desc)
{
    DescribeShader(text, " vs=", pipelineState.vertexShader);
    DescribeShader(text, " ps=", pipelineState.enableConditionalSurvey ? g_conditionalSurveyPSShader.get() : pipelineState.pixelShader);
    text += fmt::format(" spec=0x{:X} rt={} depth={} samples={} a2c={} blend={} topology={} cull={}",
        pipelineState.specConstants,
        magic_enum::enum_name(pipelineState.renderTargetFormat), magic_enum::enum_name(pipelineState.depthStencilFormat),
        uint32_t(pipelineState.sampleCount), pipelineState.enableAlphaToCoverage, pipelineState.alphaBlendEnable,
        magic_enum::enum_name(desc.primitiveTopology), magic_enum::enum_name(pipelineState.cullMode));

    text += " inputs=[";
    for (uint32_t i = 0; i < desc.inputElementsCount; i++)
    {
        const RenderInputElement& element = desc.inputElements[i];
        text += fmt::format("{}{}{}:{}@{}+{}#{}", i > 0 ? " " : "", element.semanticName != nullptr ? element.semanticName : "?",
            element.semanticIndex, magic_enum::enum_name(element.format), element.slotIndex, element.alignedByteOffset, element.location);
    }
    text += "]";
}

static void ReportGamePipelineFailure(const PipelineState& pipelineState, const RenderGraphicsPipelineDesc& desc)
{
    static std::atomic<uint32_t> s_failureCount{ 0 };
    const uint32_t failureIndex = ++s_failureCount;

    std::string text = fmt::format("Pipeline creation failed (#{}):", failureIndex);
    DescribeGamePipeline(text, pipelineState, desc);

    LOGF_ERROR("{}", text);
    DumpOwnLogcatOnce("the first pipeline failure");
}

static void CheckBuiltInPipeline(const RenderPipeline* pipeline, const char* name)
{
    if (pipeline != nullptr && !pipeline->isValid())
    {
        LOGF_ERROR("Built-in pipeline \"{}\" could not be created.", name);
        DumpOwnLogcatOnce("the first pipeline failure");
    }
}
#else
static void CheckBuiltInPipeline(const RenderPipeline*, const char*)
{
}
#endif

static std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& pipelineState)
{
#ifdef ASYNC_PSO_DEBUG
    ++g_pipelinesCurrentlyCompiling;
#endif

    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = GetOrLinkShader(pipelineState.vertexShader, pipelineState.specConstants);
    auto* pixelShader = pipelineState.enableConditionalSurvey && g_hasConditionalSurvey ?
        g_conditionalSurveyPSShader.get() : pipelineState.pixelShader;
    desc.pixelShader = pixelShader != nullptr ? GetOrLinkShader(pixelShader, pipelineState.specConstants) : nullptr;
    if (g_compactFixedDescriptors && (desc.vertexShader == nullptr || (pixelShader != nullptr && desc.pixelShader == nullptr)))
    {
#ifdef ASYNC_PSO_DEBUG
        --g_pipelinesCurrentlyCompiling;
#endif
        return nullptr;
    }
    desc.depthFunction = pipelineState.zFunc;
    desc.depthEnabled = pipelineState.zEnable;
    desc.depthWriteEnabled = pipelineState.zWriteEnable;
    desc.depthBias = pipelineState.depthBias;
    desc.stencilEnabled = pipelineState.stencilEnable;
    desc.stencilReadMask = pipelineState.stencilMask;
    desc.stencilWriteMask = pipelineState.stencilWriteMask;
    desc.stencilReference = pipelineState.stencilRef;
    desc.stencilFrontFace.compareFunction = pipelineState.stencilFunc;
    desc.stencilFrontFace.failOp = pipelineState.stencilFail;
    desc.stencilFrontFace.depthFailOp = pipelineState.stencilZFail;
    desc.stencilFrontFace.passOp = pipelineState.stencilPass;
    if (pipelineState.stencilTwoSided) {
        desc.stencilBackFace.compareFunction = pipelineState.stencilFuncCCW;
        desc.stencilBackFace.failOp = pipelineState.stencilFailCCW;
        desc.stencilBackFace.depthFailOp = pipelineState.stencilZFailCCW;
        desc.stencilBackFace.passOp = pipelineState.stencilPassCCW;
    } else {
        desc.stencilBackFace = desc.stencilFrontFace;
    }
    desc.slopeScaledDepthBias = pipelineState.slopeScaledDepthBias;
    desc.dynamicDepthBiasEnabled = g_capabilities.dynamicDepthBias;
    desc.depthClipEnabled = true;
    desc.primitiveTopology = pipelineState.primitiveTopology;
    if (!g_capabilities.triangleFan && desc.primitiveTopology == RenderPrimitiveTopology::TRIANGLE_FAN) {
        desc.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
    }
    desc.cullMode = pipelineState.cullMode;
    desc.frontFace = pipelineState.frontFace;
    desc.renderTargetFormat[0] = pipelineState.renderTargetFormat;
    desc.renderTargetBlend[0].blendEnabled = pipelineState.alphaBlendEnable;
    desc.renderTargetBlend[0].srcBlend = pipelineState.srcBlend;
    desc.renderTargetBlend[0].dstBlend = pipelineState.destBlend;
    desc.renderTargetBlend[0].blendOp = pipelineState.blendOp;
    desc.renderTargetBlend[0].srcBlendAlpha = pipelineState.srcBlendAlpha;
    desc.renderTargetBlend[0].dstBlendAlpha = pipelineState.destBlendAlpha;
    desc.renderTargetBlend[0].blendOpAlpha = pipelineState.blendOpAlpha;
    desc.renderTargetBlend[0].renderTargetWriteMask = pipelineState.colorWriteEnable;
    desc.renderTargetCount = pipelineState.renderTargetFormat != RenderFormat::UNKNOWN ? 1 : 0;
    desc.depthTargetFormat = pipelineState.depthStencilFormat;
    desc.multisampling.sampleCount = pipelineState.sampleCount;
    desc.alphaToCoverageEnabled = pipelineState.enableAlphaToCoverage;
    desc.inputElements = pipelineState.vertexDeclaration->inputElements.get();
    desc.inputElementsCount = pipelineState.vertexDeclaration->inputElementCount;

#ifdef __ANDROID__
    // Translate the declaration to the vertex shader's renumbered locations, dropping the
    // elements it does not read (see g_compactVertexLocations).
    std::vector<RenderInputElement> remappedElements;
    if (g_compactVertexLocations)
    {
        std::array<uint8_t, 32> remap{};
        bool found = false;
        {
            std::lock_guard lock(g_vertexLocationRemapMutex);
            auto it = g_vertexLocationRemaps.find(desc.vertexShader);
            if (it != g_vertexLocationRemaps.end())
            {
                remap = it->second;
                found = true;
            }
        }

        if (found)
        {
            for (uint32_t i = 0; i < desc.inputElementsCount; i++)
            {
                RenderInputElement element = desc.inputElements[i];
                if (element.location < remap.size() && remap[element.location] != 0xFF)
                {
                    element.location = remap[element.location];
                    remappedElements.push_back(element);
                }
            }

            desc.inputElements = remappedElements.data();
            desc.inputElementsCount = uint32_t(remappedElements.size());
        }
    }
#endif
    
    RenderSpecConstant specConstant{};
    specConstant.value = pipelineState.specConstants;
    
    if (pipelineState.specConstants != 0)
    {
        desc.specConstants = &specConstant;
        desc.specConstantsCount = 1;
    }
    
    RenderInputSlot inputSlots[16]{};
    uint32_t inputSlotIndices[16]{};
    uint32_t inputSlotCount = 0;
    
    for (size_t i = 0; i < desc.inputElementsCount; i++)
    {
        auto& inputElement = desc.inputElements[i];
        auto& inputSlotIndex = inputSlotIndices[inputElement.slotIndex];
    
        if (inputSlotIndex == NULL)
            inputSlotIndex = ++inputSlotCount;
    
        auto& inputSlot = inputSlots[inputSlotIndex - 1];
        inputSlot.index = inputElement.slotIndex;
        inputSlot.stride = pipelineState.vertexStrides[inputElement.slotIndex];
        inputSlot.classification = RenderInputSlotClassification::PER_VERTEX_DATA;
    }

    desc.inputSlots = inputSlots;
    desc.inputSlotsCount = inputSlotCount;

#ifdef __ANDROID__
    // If the driver crashes while compiling, the crash report names this pipeline.
    {
        std::string context = "compiling pipeline";
        DescribeGamePipeline(context, pipelineState, desc);
        os::android::SetCrashContext(context.c_str());
    }
#endif

#ifdef __ANDROID__
    const auto compileStart = std::chrono::steady_clock::now();
#endif

    auto pipeline = g_device->createGraphicsPipeline(desc);

#ifdef __ANDROID__
    os::android::SetCrashContext(nullptr);

    {
        const uint64_t us = uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - compileStart).count());
        if (g_isRenderThread && !App::s_isLoading)
        {
            g_psoGameplayCount++;
            g_psoGameplayMicroseconds += us;
            uint64_t previous = g_psoGameplayMaxMicroseconds.load();
            while (us > previous && !g_psoGameplayMaxMicroseconds.compare_exchange_weak(previous, us))
            {
            }
        }
        else
        {
            g_psoOtherCount++;
            g_psoOtherMicroseconds += us;
        }
        g_pipelinesSinceCacheSave++;
    }

    if (pipeline != nullptr && !pipeline->isValid())
        ReportGamePipelineFailure(pipelineState, desc);
#endif

#ifdef ASYNC_PSO_DEBUG
    --g_pipelinesCurrentlyCompiling;
#endif

    return pipeline;
}

static RenderPipeline* CreateGraphicsPipelineInRenderThread(PipelineState pipelineState)
{
    SanitizePipelineState(pipelineState);

    XXH64_hash_t hash = GetPipelineStateHash(pipelineState);
    auto& pipeline = g_pipelines[hash];
    if (pipeline == nullptr)
    {
        pipeline = CreateGraphicsPipeline(pipelineState);
        if (pipeline == nullptr)
            return nullptr;

#ifdef ASYNC_PSO_DEBUG
        if (App::s_isLoading)
            ++g_pipelinesCreatedAsynchronously;
        else
            ++g_pipelinesCreatedInRenderThread;

        pipeline->setName(fmt::format("{} {} {} {:X}", App::s_isLoading ? "ASYNC" : "",
            pipelineState.vertexShader->name, pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>", hash));
        
        if (!App::s_isLoading)
        {
            std::lock_guard lock(g_debugMutex);
            g_pipelineDebugText = fmt::format(
                "{:X}\n"
                "    vertexShader: {}\n"
                "    pixelShader: {}\n"
                "    vertexDeclaration: {:X}\n"
                "    zEnable: {}\n"
                "    zWriteEnable: {}\n"
                "    stencilEnable: {}\n"
                "    stencilTwoSided: {}\n"
                "    srcBlend: {}\n"
                "    destBlend: {}\n"
                "    cullMode: {}\n"
                "    frontFace: {}\n"
                "    zFunc: {}\n"
                "    stencilFunc: {}\n"
                "    stencilFail: {}\n"
                "    stencilZFail: {}\n"
                "    stencilPass: {}\n"
                "    stencilFuncCCW: {}\n"
                "    stencilFailCCW: {}\n"
                "    stencilZFailCCW: {}\n"
                "    stencilPassCCW: {}\n"
                "    stencilMask: {}\n"
                "    stencilWriteMask: {}\n"
                "    stencilRef: {}\n"
                "    alphaBlendEnable: {}\n"
                "    blendOp: {}\n"
                "    slopeScaledDepthBias: {}\n"
                "    depthBias: {}\n"
                "    srcBlendAlpha: {}\n"
                "    destBlendAlpha: {}\n"
                "    blendOpAlpha: {}\n"
                "    colorWriteEnable: {:X}\n"
                "    primitiveTopology: {}\n"
                "    vertexStrides[0]: {}\n"
                "    vertexStrides[1]: {}\n"
                "    vertexStrides[2]: {}\n"
                "    vertexStrides[3]: {}\n"
                "    renderTargetFormat: {}\n"
                "    depthStencilFormat: {}\n"
                "    sampleCount: {}\n"
                "    enableAlphaToCoverage: {}\n"
                "    enableConditionalSurvey: {}\n"
                "    specConstants: {:X}\n\n",
                hash,
                pipelineState.vertexShader->name,
                pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>",
                reinterpret_cast<size_t>(pipelineState.vertexDeclaration),
                pipelineState.zEnable,
                pipelineState.zWriteEnable,
                pipelineState.stencilEnable,
                pipelineState.stencilTwoSided,
                magic_enum::enum_name(pipelineState.srcBlend),
                magic_enum::enum_name(pipelineState.destBlend),
                magic_enum::enum_name(pipelineState.cullMode),
                magic_enum::enum_name(pipelineState.frontFace),
                magic_enum::enum_name(pipelineState.zFunc),
                magic_enum::enum_name(pipelineState.stencilFunc),
                magic_enum::enum_name(pipelineState.stencilFail),
                magic_enum::enum_name(pipelineState.stencilZFail),
                magic_enum::enum_name(pipelineState.stencilPass),
                magic_enum::enum_name(pipelineState.stencilFuncCCW),
                magic_enum::enum_name(pipelineState.stencilFailCCW),
                magic_enum::enum_name(pipelineState.stencilZFailCCW),
                magic_enum::enum_name(pipelineState.stencilPassCCW),
                pipelineState.stencilMask,
                pipelineState.stencilWriteMask,
                pipelineState.stencilRef,
                pipelineState.alphaBlendEnable,
                magic_enum::enum_name(pipelineState.blendOp),
                pipelineState.slopeScaledDepthBias,
                pipelineState.depthBias,
                magic_enum::enum_name(pipelineState.srcBlendAlpha),
                magic_enum::enum_name(pipelineState.destBlendAlpha),
                magic_enum::enum_name(pipelineState.blendOpAlpha),
                pipelineState.colorWriteEnable,
                magic_enum::enum_name(pipelineState.primitiveTopology),
                pipelineState.vertexStrides[0],
                pipelineState.vertexStrides[1],
                pipelineState.vertexStrides[2],
                pipelineState.vertexStrides[3],
                magic_enum::enum_name(pipelineState.renderTargetFormat),
                magic_enum::enum_name(pipelineState.depthStencilFormat),
                pipelineState.sampleCount,
                pipelineState.enableAlphaToCoverage,
                pipelineState.enableConditionalSurvey,
                pipelineState.specConstants)
                + g_pipelineDebugText;
        }
#endif

#ifdef PSO_CACHING
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.emplace(hash, pipelineState);
#endif
    }
    
    return pipeline.get();
}

static RenderTextureAddressMode ConvertTextureAddressMode(size_t value)
{
    switch (value)
    {
    case D3DTADDRESS_WRAP:
        return RenderTextureAddressMode::WRAP;
    case D3DTADDRESS_MIRROR:
        return RenderTextureAddressMode::MIRROR;
    case D3DTADDRESS_CLAMP:
        return RenderTextureAddressMode::CLAMP;
    case D3DTADDRESS_MIRRORONCE:
        return RenderTextureAddressMode::MIRROR_ONCE;
    case D3DTADDRESS_BORDER:
        return RenderTextureAddressMode::BORDER;
    default:
        assert(false && "Unknown texture address mode");
        return RenderTextureAddressMode::UNKNOWN;
    }
}

static RenderFilter ConvertTextureFilter(uint32_t value)
{
    switch (value)
    {
    case D3DTEXF_POINT:
    case D3DTEXF_NONE:
        return RenderFilter::NEAREST;
    case D3DTEXF_LINEAR:
        return RenderFilter::LINEAR;
    default:
        assert(false && "Unknown texture filter");
        return RenderFilter::UNKNOWN;
    }
}

static RenderBorderColor ConvertBorderColor(uint32_t value)
{
    switch (value)
    {
    case 0:
        return RenderBorderColor::TRANSPARENT_BLACK;
    case 1:
        return RenderBorderColor::OPAQUE_WHITE;
    default:
        assert(false && "Unknown border color");
        return RenderBorderColor::UNKNOWN;
    }
}

struct LocalRenderCommandQueue
{
    RenderCommand commands[20];
    uint32_t count = 0;

    RenderCommand& enqueue()
    {
        assert(count < std::size(commands));
        return commands[count++];
    }

    void submit()
    {
        EnqueueRenderCommands(commands, count);
    }
};

static ShaderConstantRange GetDirtyShaderConstantRange(uint64_t flags)
{
    if (flags == 0)
        return {};

    const uint32_t firstGroup = std::countl_zero(flags);
    const uint32_t endGroup = 64 - std::countr_zero(flags);
    return { firstGroup * 16, (endGroup - firstGroup) * 64 };
}

static void EnqueueSamplerState(const GuestDevice* device, LocalRenderCommandQueue& queue, uint32_t index)
{
    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::SetSamplerState;
    cmd.setSamplerState.index = index;
    cmd.setSamplerState.data0 = device->samplerStates[index].data[0];
    cmd.setSamplerState.data3 = device->samplerStates[index].data[3];
    cmd.setSamplerState.data5 = device->samplerStates[index].data[5];
}

static void FlushRenderStateForMainThread(GuestDevice* device, LocalRenderCommandQueue& queue)
{
    constexpr size_t BOOL_MASK = 0x2ull;
    if ((device->dirtyFlags[3].get() & BOOL_MASK) != 0)
    {
        auto& cmd = queue.enqueue();
        cmd.type = RenderCommandType::SetBooleans;
        cmd.setBooleans.booleans = (device->vertexShaderBoolConstants[0].get() & 0xFF) | ((device->pixelShaderBoolConstants[0].get() & 0xFF) << 16);

        device->dirtyFlags[3] = device->dirtyFlags[3].get() & ~BOOL_MASK;
    }

    constexpr uint32_t firstSamplerBit = 20;
    if (g_batchDrawState)
    {
        constexpr uint64_t samplerMask = 0xFFFFull << (64 - firstSamplerBit - 16);
        const uint64_t dirtySamplers = device->dirtyFlags[2].get() & samplerMask;
        for (uint64_t pending = dirtySamplers; pending != 0;)
        {
            const uint32_t bit = std::countl_zero(pending);
            EnqueueSamplerState(device, queue, bit - firstSamplerBit);
            pending &= ~(0x8000000000000000ull >> bit);
        }
        if (dirtySamplers != 0)
            device->dirtyFlags[2] = device->dirtyFlags[2].get() & ~dirtySamplers;
    }
    else
    {
        for (uint32_t index = 0; index < 16; index++)
        {
            const uint64_t mask = 0x8000000000000000ull >> (index + firstSamplerBit);
            if ((device->dirtyFlags[2].get() & mask) != 0)
            {
                EnqueueSamplerState(device, queue, index);
                device->dirtyFlags[2] = device->dirtyFlags[2].get() & ~mask;
            }
        }
    }

    // The guest array is larger than the 56 groups exposed to the pixel shader.
    constexpr uint64_t pixelRegisterMask = ~0ull << (64 - sizeof(g_pixelShaderConstants) / 64);
    const auto vertexFlags = device->dirtyFlags[0].get();
    const auto pixelFlags = device->dirtyFlags[1].get();
    const auto vertex = GetDirtyShaderConstantRange(vertexFlags);
    const auto pixel = GetDirtyShaderConstantRange(pixelFlags & pixelRegisterMask);
    if (g_batchDrawState && vertex.size != 0 && pixel.size != 0)
    {
        auto* memory = g_intermediaryUploadAllocator.allocate(vertex.size + pixel.size);
        memcpy(memory, &device->vertexShaderFloatConstants[vertex.index], vertex.size);
        memcpy(memory + vertex.size, &device->pixelShaderFloatConstants[pixel.index], pixel.size);
        auto& cmd = queue.enqueue();
        cmd.type = RenderCommandType::SetDrawShaderConstants;
        cmd.setDrawShaderConstants = { memory, vertex, pixel };
    }
    else
    {
        if (vertex.size != 0)
        {
            auto& cmd = queue.enqueue();
            cmd.type = RenderCommandType::SetVertexShaderConstants;
            cmd.setShaderConstants = {
                g_intermediaryUploadAllocator.allocate(&device->vertexShaderFloatConstants[vertex.index], vertex.size), vertex };
        }
        if (pixel.size != 0)
        {
            auto& cmd = queue.enqueue();
            cmd.type = RenderCommandType::SetPixelShaderConstants;
            cmd.setShaderConstants = {
                g_intermediaryUploadAllocator.allocate(&device->pixelShaderFloatConstants[pixel.index], pixel.size), pixel };
        }
    }

    if (vertexFlags != 0)
        device->dirtyFlags[0] = 0;
    if (pixelFlags != 0)
        device->dirtyFlags[1] = 0;
}

static void ProcSetBooleans(const RenderCommand& cmd)
{
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.booleans, cmd.setBooleans.booleans);
}

static void ProcSetSamplerState(const RenderCommand& cmd)
{
    const auto& args = cmd.setSamplerState;

    const auto addressU = ConvertTextureAddressMode((args.data0 >> 10) & 0x7);
    const auto addressV = ConvertTextureAddressMode((args.data0 >> 13) & 0x7);
    const auto addressW = ConvertTextureAddressMode((args.data0 >> 16) & 0x7);
    auto magFilter = ConvertTextureFilter((args.data3 >> 19) & 0x3);
    auto minFilter = ConvertTextureFilter((args.data3 >> 21) & 0x3);
    auto mipFilter = ConvertTextureFilter((args.data3 >> 23) & 0x3);
    const auto borderColor = ConvertBorderColor(args.data5 & 0x3);

    const uint32_t anisotropy = Config::GetEffectiveAnisotropicFiltering();
    bool anisotropyEnabled = anisotropy > 0 && mipFilter == RenderFilter::LINEAR;
    if (anisotropyEnabled)
    {
        magFilter = RenderFilter::LINEAR;
        minFilter = RenderFilter::LINEAR;
    }

    auto& samplerDesc = g_samplerDescs[args.index];

    bool dirty = false;

    SetDirtyValue(dirty, samplerDesc.addressU, addressU);
    SetDirtyValue(dirty, samplerDesc.addressV, addressV);
    SetDirtyValue(dirty, samplerDesc.addressW, addressW);
    SetDirtyValue(dirty, samplerDesc.minFilter, minFilter);
    SetDirtyValue(dirty, samplerDesc.magFilter, magFilter);
    SetDirtyValue(dirty, samplerDesc.mipmapMode, RenderMipmapMode(mipFilter));
    SetDirtyValue(dirty, samplerDesc.maxAnisotropy, anisotropyEnabled ? anisotropy : 1u);
    SetDirtyValue(dirty, samplerDesc.anisotropyEnabled, anisotropyEnabled);
    SetDirtyValue(dirty, samplerDesc.borderColor, borderColor);

    if (dirty)
        UpdateGuestSampler(args.index);
}

static bool StoreNativeShaderConstants(uint32_t* destination, const uint8_t* snapshot, uint32_t bytes)
{
    assert(bytes % sizeof(uint32_t) == 0);
    bool changed = false;
    for (uint32_t offset = 0; offset < bytes; offset += sizeof(uint32_t), ++destination)
    {
        uint32_t word;
        memcpy(&word, snapshot + offset, sizeof(word));
        word = ByteSwap(word);
        changed |= *destination != word;
        *destination = word;
    }
    return changed;
}

static void UpdateShaderConstantBlock(uint32_t* destination, const uint8_t* snapshot, uint32_t bytes, bool& dirty)
{
    if (!g_optimizeDrawConstants)
    {
        memcpy(destination, snapshot, bytes);
        dirty = true;
        return;
    }

    // Keep the host mirror in native byte order. Compare shader values as bits,
    // so repeated NaNs and the sign bit of zero have the expected semantics.
    dirty |= StoreNativeShaderConstants(destination, snapshot, bytes);
#ifdef __ANDROID__
    if (!dirty && g_passQueryPools[g_frame] != nullptr)
        ++g_drawConstantFrameStats[g_frame].repeatedUpdates;
#endif
}

template<size_t N>
static void ApplyShaderConstantSnapshot(uint32_t (&destination)[N], const uint8_t* memory, ShaderConstantRange range, bool& dirty)
{
    assert((range.index * sizeof(uint32_t) + range.size) <= sizeof(destination));
    UpdateShaderConstantBlock(&destination[range.index], memory, range.size, dirty);
}

static void ProcSetVertexShaderConstants(const RenderCommand& cmd)
{
    const auto& args = cmd.setShaderConstants;
    ApplyShaderConstantSnapshot(g_vertexShaderConstants, args.memory, args.range, g_dirtyStates.vertexShaderConstants);
}

static void ProcSetPixelShaderConstants(const RenderCommand& cmd)
{
    const auto& args = cmd.setShaderConstants;
    ApplyShaderConstantSnapshot(g_pixelShaderConstants, args.memory, args.range, g_dirtyStates.pixelShaderConstants);
}

static void ProcSetDrawShaderConstants(const RenderCommand& cmd)
{
    const auto& args = cmd.setDrawShaderConstants;
    ApplyShaderConstantSnapshot(g_vertexShaderConstants, args.memory, args.vertex, g_dirtyStates.vertexShaderConstants);
    ApplyShaderConstantSnapshot(g_pixelShaderConstants, args.memory + args.vertex.size, args.pixel, g_dirtyStates.pixelShaderConstants);
#ifdef __ANDROID__
    if (g_passQueryPools[g_frame] != nullptr)
        ++g_drawConstantFrameStats[g_frame].pairedSnapshots;
#endif
}

static void ProcAddPipeline(const RenderCommand& cmd)
{
    auto& args = cmd.addPipeline;
    auto& pipeline = g_pipelines[args.hash];

    if (pipeline == nullptr)
    {
        pipeline = std::unique_ptr<RenderPipeline>(args.pipeline);
#ifdef ASYNC_PSO_DEBUG
        ++g_pipelinesCreatedAsynchronously;
#endif
    }
    else
    {
#ifdef ASYNC_PSO_DEBUG
        ++g_pipelinesDropped;
#endif
        delete args.pipeline;
    }
}

static constexpr int32_t COMMON_DEPTH_BIAS_VALUE = int32_t((1 << 24) * 0.002f);
static constexpr float COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE = 1.0f;

// The game sometimes draws while no vertex declaration (or vertex shader) is set; seen as a
// DrawPrimitiveUP while a level loaded on a Helio G99. Building a pipeline for that state reads
// through the null pointer and crashes the render thread, so the draw is skipped instead. Each
// kind of draw is logged once.
static bool CanDrawWithPipelineState(const char* draw, std::atomic<uint32_t>& skipped)
{
    if (g_skipSurveyDraws)
        return false;
    if (g_pipelineState.vertexDeclaration != nullptr && g_pipelineState.vertexShader != nullptr)
        return true;

    if (skipped.fetch_add(1, std::memory_order_relaxed) == 0)
    {
        LOGFN_WARNING("{} skipped: no {} is set (later skips of this kind are not logged).", draw,
            g_pipelineState.vertexDeclaration == nullptr ? "vertex declaration" : "vertex shader");
    }

    return false;
}

static bool FlushRenderStateForRenderThread()
{
    auto renderTarget = g_pipelineState.colorWriteEnable ? g_renderTarget : nullptr;
    auto depthStencil = g_pipelineState.zEnable || g_pipelineState.stencilEnable ? g_depthStencil : nullptr;

    // With ResolutionScale the scene depth is smaller than the full-size back buffer, and such a
    // pair cannot form a framebuffer. The draw then runs without depth (the game's back buffer
    // passes are 2D), and its pipeline is built without the depth and stencil tests too: a
    // pipeline with a depth format is not compatible with a render pass that has no depth.
    static bool s_depthDropped = false;
    const bool depthDropped = renderTarget != nullptr && depthStencil != nullptr &&
        (depthStencil->width < renderTarget->width || depthStencil->height < renderTarget->height);

    if (depthDropped)
    {
        static bool s_loggedDepthDropped;
        if (!s_loggedDepthDropped)
        {
            LOGF_WARNING("Depth stencil {}x{} is smaller than render target {}x{}; drawing without depth and stencil tests.",
                depthStencil->width, depthStencil->height, renderTarget->width, renderTarget->height);
            s_loggedDepthDropped = true;
        }

        depthStencil = nullptr;
    }

    // The targets can change without any pipeline state changing, so the switch itself is dirty.
    if (depthDropped != s_depthDropped)
    {
        s_depthDropped = depthDropped;
        g_dirtyStates.pipelineState = true;
    }

    bool foundAny = PopulateBarriersForStretchRect(renderTarget, depthStencil);

    auto aliasesBoundStorage = [&](GuestSurface* surface)
    {
        return surface != renderTarget && surface != depthStencil &&
            (SharesStorage(surface, renderTarget) || SharesStorage(surface, depthStencil));
    };

    for (const auto surface : g_pendingSurfaceCopies)
    {
        if (aliasesBoundStorage(surface))
        {
            bool isDepthStencil = RenderFormatIsDepth(surface->format);
            foundAny |= PopulateBarriersForStretchRect(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
        }
    }

#ifdef __ANDROID__
    const auto independentCopy = g_mergeDepthCopyClears && !foundAny && g_framebuffer != nullptr &&
        g_pendingResolves.size() == 1 ? *g_pendingResolves.begin() : nullptr;
    if (independentCopy != nullptr && independentCopy->destinationTextures.size() == 1)
    {
        const auto [destination, slice] = *independentCopy->destinationTextures.begin();
        const auto sourceStorage = independentCopy->variant != nullptr ?
            independentCopy->variant->textureHolder.get() : independentCopy->textureHolder.get();

        if (destination->sourceSurface == independentCopy && independentCopy->texture == sourceStorage &&
            IsFullDepthSliceCopy(independentCopy, destination, slice))
        {
            PopulateBarriersForStretchRect(nullptr, independentCopy);
            const auto framebuffer = g_framebuffer;
            const bool clearDeferred = FlushBarriers(sourceStorage, destination->texture);
            const bool copied = ExecutePendingStretchRectCommands(nullptr, independentCopy);

            if (clearDeferred)
            {
                // Only the copy's destination can have queued a new transition. Record
                // that transition while the clear is still deferred.
                FlushBarriers();
                g_commandLists[g_frame]->restoreFramebufferClear(copied);
                g_framebuffer = framebuffer;
            }

            if (!copied)
                return false;
            g_pendingResolves.clear();
        }
    }
#endif

    for (const auto surface : g_pendingResolves)
    {
        bool isDepthStencil = RenderFormatIsDepth(surface->format);
        foundAny |= PopulateBarriersForStretchRect(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
    }

    if (foundAny)
    {
        FlushBarriers();
        if (!ExecutePendingStretchRectCommands(renderTarget, depthStencil))
            return false;

        for (const auto surface : g_pendingSurfaceCopies)
        {
            if (aliasesBoundStorage(surface))
            {
                bool isDepthStencil = RenderFormatIsDepth(surface->format);
                if (!ExecutePendingStretchRectCommands(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr))
                    return false;
            }
        }

        for (const auto surface : g_pendingResolves)
        {
            bool isDepthStencil = RenderFormatIsDepth(surface->format);
            if (!ExecutePendingStretchRectCommands(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr))
                return false;
        }
    }

    if (!g_pendingResolves.empty())
        g_pendingResolves.clear();

    AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);

    FlushBarriers();

    SetFramebuffer(renderTarget, depthStencil, false);
    ApplyDeferredVariantClears(renderTarget, depthStencil);
    FlushViewport();

    auto& commandList = g_commandLists[g_frame];

    // D3D12 resets depth bias values to the pipeline values, even if they are dynamic.
    // We can reduce unnecessary calls by making common depth bias values part of the pipeline.
    if (g_capabilities.dynamicDepthBias && g_backend == Backend::D3D12)
    {
        bool useDepthBias = (g_depthBias != 0) || (g_slopeScaledDepthBias != 0.0f);

        int32_t depthBias = useDepthBias ? COMMON_DEPTH_BIAS_VALUE : 0;
        float slopeScaledDepthBias = useDepthBias ? COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE : 0.0f;

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, depthBias);
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, slopeScaledDepthBias);
    }

    if (g_dirtyStates.pipelineState)
    {
        PipelineState pipelineState = g_pipelineState;
        if (depthDropped)
        {
            pipelineState.zEnable = false;
            pipelineState.stencilEnable = false;
        }

        auto* pipeline = CreateGraphicsPipelineInRenderThread(pipelineState);
        if (pipeline == nullptr)
            return false;
        commandList->setPipeline(pipeline);

        // D3D12 resets the depth bias values. Check if they need to be set again.
        if (g_capabilities.dynamicDepthBias && g_backend == Backend::D3D12)
            g_dirtyStates.depthBias = (g_depthBias != g_pipelineState.depthBias) || (g_slopeScaledDepthBias != g_pipelineState.slopeScaledDepthBias);
    }

    if (g_dirtyStates.depthBias && g_capabilities.dynamicDepthBias)
        commandList->setDepthBias(g_depthBias, 0.0f, g_slopeScaledDepthBias);

    if (g_useBufferConstants)
        UploadDrawConstants<true>();
    else
        UploadDrawConstants<false>();

    if (g_useFixedDescriptors && !BindFixedDrawDescriptorSets(*commandList))
        return false;

    if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast)
    {
        commandList->setVertexBuffers(
            g_dirtyStates.vertexStreamFirst,
            g_vertexBufferViews + g_dirtyStates.vertexStreamFirst,
            g_dirtyStates.vertexStreamLast - g_dirtyStates.vertexStreamFirst + 1,
            g_inputSlots + g_dirtyStates.vertexStreamFirst);
    }

    if (g_dirtyStates.indices && (g_backend == Backend::D3D12 || g_indexBufferView.buffer.ref != nullptr))
        commandList->setIndexBuffer(&g_indexBufferView);

    g_dirtyStates = DirtyStates(false);

#ifdef __ANDROID__
    if (g_depthCopyTraceRemaining != 0 && depthStencil != nullptr)
    {
        auto& trace = g_depthCopyTrace;
        ++trace.draws;
        trace.stencilDraws += g_pipelineState.stencilEnable;
        trace.colorAttachmentDuringDraw |= renderTarget != nullptr;

        if (!trace.destinationBoundDuringDraw && trace.plannedDestination.textureAddress != 0)
        {
            const auto destination = g_memory.Translate(trace.plannedDestination.textureAddress);
            trace.destinationBoundDuringDraw = std::find(std::begin(g_textures), std::end(g_textures),
                destination) != std::end(g_textures);
        }
    }
#endif

    return true;
}

static RenderPrimitiveTopology ConvertPrimitiveType(uint32_t primitiveType)
{
    switch (primitiveType)
    {
    case D3DPT_POINTLIST:
        return RenderPrimitiveTopology::POINT_LIST;
    case D3DPT_LINELIST:
        return RenderPrimitiveTopology::LINE_LIST;
    case D3DPT_LINESTRIP:
        return RenderPrimitiveTopology::LINE_STRIP;
    case D3DPT_TRIANGLELIST:
    case D3DPT_QUADLIST:
        return RenderPrimitiveTopology::TRIANGLE_LIST;
    case D3DPT_TRIANGLESTRIP:
        return RenderPrimitiveTopology::TRIANGLE_STRIP;
    case D3DPT_TRIANGLEFAN:
        return RenderPrimitiveTopology::TRIANGLE_FAN;
    default:
        assert(false && "Unknown primitive type");
        return RenderPrimitiveTopology::UNKNOWN;
    }
}

static void SetPrimitiveType(uint32_t primitiveType)
{
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, ConvertPrimitiveType(primitiveType));
}

static void DrawPrimitive(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex, uint32_t primitiveCount) 
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawPrimitive;
    cmd.drawPrimitive.primitiveType = primitiveType;
    cmd.drawPrimitive.startVertex = startVertex;
    cmd.drawPrimitive.primitiveCount = primitiveCount;

    queue.submit();
}

static void ProcDrawPrimitive(const RenderCommand& cmd)
{
    const auto& args = cmd.drawPrimitive;

    static std::atomic<uint32_t> s_skipped;
    if (!CanDrawWithPipelineState("DrawPrimitive", s_skipped))
        return;

    SetPrimitiveType(args.primitiveType);

    if (!FlushRenderStateForRenderThread())
        return;

    auto& commandList = g_commandLists[g_frame];
    commandList->drawInstanced(args.primitiveCount, 1, args.startVertex, 0);
}

static void DrawIndexedPrimitive(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex, uint32_t startIndex, uint32_t primCount)
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawIndexedPrimitive;
    cmd.drawIndexedPrimitive.primitiveType = primitiveType;
    cmd.drawIndexedPrimitive.baseVertexIndex = baseVertexIndex;
    cmd.drawIndexedPrimitive.startIndex = startIndex;
    cmd.drawIndexedPrimitive.primCount = primCount;

    queue.submit();
}

static void ProcDrawIndexedPrimitive(const RenderCommand& cmd)
{
    const auto& args = cmd.drawIndexedPrimitive;

    static std::atomic<uint32_t> s_skipped;
    if (!CanDrawWithPipelineState("DrawIndexedPrimitive", s_skipped))
        return;

    SetPrimitiveType(args.primitiveType);
    if (!FlushRenderStateForRenderThread())
        return;

    g_commandLists[g_frame]->drawIndexedInstanced(args.primCount, 1, args.startIndex, args.baseVertexIndex, 0);
}

static void DrawPrimitiveUP(GuestDevice* device, uint32_t primitiveType, uint32_t primitiveCount, void* vertexStreamZeroData, uint32_t vertexStreamZeroStride)
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawPrimitiveUP;
    cmd.drawPrimitiveUP.primitiveType = primitiveType;
    cmd.drawPrimitiveUP.primitiveCount = primitiveCount;
    cmd.drawPrimitiveUP.vertexStreamZeroData = g_intermediaryUploadAllocator.allocate(vertexStreamZeroData, primitiveCount * vertexStreamZeroStride);
    cmd.drawPrimitiveUP.vertexStreamZeroSize = primitiveCount * vertexStreamZeroStride;
    cmd.drawPrimitiveUP.vertexStreamZeroStride = vertexStreamZeroStride;
    
    queue.submit();
}

static void ProcDrawPrimitiveUP(const RenderCommand& cmd)
{
    const auto& args = cmd.drawPrimitiveUP;

    static std::atomic<uint32_t> s_skipped;
    if (!CanDrawWithPipelineState("DrawPrimitiveUP", s_skipped))
        return;

    SetPrimitiveType(args.primitiveType);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0], uint8_t(args.vertexStreamZeroStride));

    auto allocation = g_uploadAllocators[g_frame].allocate<true>(reinterpret_cast<const uint32_t*>(args.vertexStreamZeroData), args.vertexStreamZeroSize, 0x4);

    auto& vertexBufferView = g_vertexBufferViews[0];
    vertexBufferView.size = args.primitiveCount * args.vertexStreamZeroStride;
    vertexBufferView.buffer = allocation.buffer->at(allocation.offset);
    g_inputSlots[0].stride = args.vertexStreamZeroStride;
    g_dirtyStates.vertexStreamFirst = 0;

    uint32_t indexCount = 0;

    if (args.primitiveType == D3DPT_QUADLIST)
        indexCount = g_quadIndexData.prepare(args.primitiveCount);
    else if (!g_capabilities.triangleFan && args.primitiveType == D3DPT_TRIANGLEFAN)
        indexCount = g_triangleFanIndexData.prepare(args.primitiveCount);

    if (!FlushRenderStateForRenderThread())
        return;

    if (indexCount != 0)
        g_commandLists[g_frame]->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
    else
        g_commandLists[g_frame]->drawInstanced(args.primitiveCount, 1, 0, 0);
}

static const char* ConvertDeclUsage(uint32_t usage)
{
    switch (usage)
    {
    case D3DDECLUSAGE_POSITION:
        return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT:
        return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES:
        return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL:
        return "NORMAL";
    case D3DDECLUSAGE_PSIZE:
        return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD:
        return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT:
        return "TANGENT";
    case D3DDECLUSAGE_BINORMAL:
        return "BINORMAL";
    case D3DDECLUSAGE_TESSFACTOR:
        return "TESSFACTOR";
    case D3DDECLUSAGE_POSITIONT:
        return "POSITIONT";
    case D3DDECLUSAGE_COLOR:
        return "COLOR";
    case D3DDECLUSAGE_FOG:
        return "FOG";
    case D3DDECLUSAGE_DEPTH:
        return "DEPTH";
    case D3DDECLUSAGE_SAMPLE:
        return "SAMPLE";
    default:
        assert(false && "Unknown usage");
        return "UNKNOWN";
    }
}

static RenderFormat ConvertDeclType(uint32_t type)
{
    switch (type)
    {
    case D3DDECLTYPE_FLOAT1:
        return RenderFormat::R32_FLOAT;
    case D3DDECLTYPE_FLOAT2:
        return RenderFormat::R32G32_FLOAT;
    case D3DDECLTYPE_FLOAT3:
        return RenderFormat::R32G32B32_FLOAT;
    case D3DDECLTYPE_FLOAT4:
        return RenderFormat::R32G32B32A32_FLOAT;
    case D3DDECLTYPE_D3DCOLOR:
        return RenderFormat::B8G8R8A8_UNORM;
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2:
        return RenderFormat::R8G8B8A8_UINT;
    case D3DDECLTYPE_SHORT2:
        return RenderFormat::R16G16_SINT;
    case D3DDECLTYPE_SHORT4:
        return RenderFormat::R16G16B16A16_SINT;
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2:
        return RenderFormat::R8G8B8A8_UNORM;
    case D3DDECLTYPE_SHORT2N:
        return RenderFormat::R16G16_SNORM;
    case D3DDECLTYPE_SHORT4N:
        return RenderFormat::R16G16B16A16_SNORM;
    case D3DDECLTYPE_USHORT2N:
        return RenderFormat::R16G16_UNORM;
    case D3DDECLTYPE_USHORT4N:
        return RenderFormat::R16G16B16A16_UNORM;
    case D3DDECLTYPE_UINT1:
        return RenderFormat::R32_UINT;
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
        return RenderFormat::R32_UINT;
    case D3DDECLTYPE_FLOAT16_2:
        return RenderFormat::R16G16_FLOAT;
    case D3DDECLTYPE_FLOAT16_4:
        return RenderFormat::R16G16B16A16_FLOAT;
    default:
        assert(false && "Unknown type");
        return RenderFormat::UNKNOWN;
    }
}

static GuestVertexDeclaration* CreateVertexDeclarationWithoutAddRef(GuestVertexElement* vertexElements) 
{
    size_t vertexElementCount = 0;
    auto vertexElement = vertexElements;

    while (vertexElement->stream != 0xFF && vertexElement->type != D3DDECLTYPE_UNUSED)
    {
        vertexElement->padding = 0;
        ++vertexElement;
        ++vertexElementCount;
    }

    vertexElement->padding = 0; // Clear the padding in D3DDECL_END() 

    std::lock_guard lock(g_vertexDeclarationMutex);

    XXH64_hash_t hash = XXH3_64bits(vertexElements, vertexElementCount * sizeof(GuestVertexElement));
    auto& vertexDeclaration = g_vertexDeclarations[hash];

    if (vertexDeclaration == nullptr)
    {
        vertexDeclaration = g_userHeap.AllocPhysical<GuestVertexDeclaration>(ResourceType::VertexDeclaration);
        vertexDeclaration->hash = hash;

        static std::vector<RenderInputElement> inputElements;
        inputElements.clear();

        struct Location
        {
            uint32_t usage;
            uint32_t usageIndex;
            uint32_t location;
        };

        // Should match the locations defined in XenosRecomp.
        constexpr Location locations[] =
        {
            { D3DDECLUSAGE_POSITION, 0, 0 },
            { D3DDECLUSAGE_POSITION, 1, 1 },
            { D3DDECLUSAGE_POSITION, 2, 2 },
            { D3DDECLUSAGE_POSITION, 3, 3 },
            { D3DDECLUSAGE_NORMAL, 0, 4 },
            { D3DDECLUSAGE_NORMAL, 1, 5 },
            { D3DDECLUSAGE_NORMAL, 2, 6 },
            { D3DDECLUSAGE_NORMAL, 3, 7 },
            { D3DDECLUSAGE_TANGENT, 0, 8 },
            { D3DDECLUSAGE_TANGENT, 1, 9 },
            { D3DDECLUSAGE_TANGENT, 2, 10 },
            { D3DDECLUSAGE_TANGENT, 3, 11 },
            { D3DDECLUSAGE_BINORMAL, 0, 12 },
            { D3DDECLUSAGE_TEXCOORD, 0, 13 },
            { D3DDECLUSAGE_TEXCOORD, 1, 14 },
            { D3DDECLUSAGE_TEXCOORD, 2, 15 },
            { D3DDECLUSAGE_TEXCOORD, 3, 16 },
            { D3DDECLUSAGE_COLOR, 0, 17 },
            { D3DDECLUSAGE_BLENDINDICES, 0, 18 },
            { D3DDECLUSAGE_BLENDWEIGHT, 0, 19 },
        };

        vertexElement = vertexElements;
        while (vertexElement->stream != 0xFF && vertexElement->type != D3DDECLTYPE_UNUSED)
        {
            uint32_t resolvedLocation = ~0;
            for (auto& location : locations)
            {
                if (location.usage == vertexElement->usage && location.usageIndex == vertexElement->usageIndex)
                {
                    resolvedLocation = location.location;
                    break;
                }
            }

            if (resolvedLocation == ~0)
            {
                // Bound but not used by any guest shaders.
                ++vertexElement;
                continue;
            }

            auto& inputElement = inputElements.emplace_back();
            inputElement.semanticName = ConvertDeclUsage(vertexElement->usage);
            inputElement.semanticIndex = vertexElement->usageIndex;
            inputElement.location = resolvedLocation;
            inputElement.format = ConvertDeclType(vertexElement->type);
            inputElement.slotIndex = vertexElement->stream;
            inputElement.alignedByteOffset = vertexElement->offset;

            switch (vertexElement->usage)
            {
            case D3DDECLUSAGE_NORMAL:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedNormals |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;
            case D3DDECLUSAGE_BINORMAL:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedBinormals |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;
            case D3DDECLUSAGE_TANGENT:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedTangents |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;
            case D3DDECLUSAGE_BLENDWEIGHT:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedBlendWeights |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;

            case D3DDECLUSAGE_TEXCOORD:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedTexcoords |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;
            }

            vertexDeclaration->vertexStreams[vertexElement->stream] = true;

            ++vertexElement;
        }

        auto addInputElement = [&](uint32_t usage, uint32_t usageIndex)
            {
                uint32_t location = ~0;

                for (auto& alsoLocation : locations)
                {
                    if (alsoLocation.usage == usage && alsoLocation.usageIndex == usageIndex)
                    {
                        location = alsoLocation.location;
                        break;
                    }
                }

                assert(location != ~0);

                for (auto& inputElement : inputElements)
                {
                    if (inputElement.location == location)
                        return;
                }

                auto format = RenderFormat::R32_FLOAT;
                switch (usage)
                {
                case D3DDECLUSAGE_NORMAL:
                case D3DDECLUSAGE_TANGENT:
                case D3DDECLUSAGE_BINORMAL:
                case D3DDECLUSAGE_BLENDINDICES:
                    format = RenderFormat::R32G32B32_FLOAT;
                    break;
                }

                inputElements.emplace_back(ConvertDeclUsage(usage), usageIndex, location, format, 15, 0);
            };

        // Assign any unbound usages to null buffer slot.
        for (auto& location : locations)
        {
            addInputElement(location.usage, location.usageIndex);
        }

        vertexDeclaration->inputElements = std::make_unique<RenderInputElement[]>(inputElements.size());
        std::copy(inputElements.begin(), inputElements.end(), vertexDeclaration->inputElements.get());

        vertexDeclaration->vertexElements = std::make_unique<GuestVertexElement[]>(vertexElementCount + 1);
        std::copy(vertexElements, vertexElements + vertexElementCount + 1, vertexDeclaration->vertexElements.get());

        vertexDeclaration->inputElementCount = uint32_t(inputElements.size());
        vertexDeclaration->vertexElementCount = vertexElementCount + 1;
    }

    vertexDeclaration->AddRef();
    return vertexDeclaration;
}

static GuestVertexDeclaration* CreateVertexDeclaration(GuestVertexElement* vertexElements)
{
    auto vertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements);
    vertexDeclaration->AddRef();
    return vertexDeclaration;
}

static void SetVertexDeclaration(GuestDevice* device, GuestVertexDeclaration* vertexDeclaration) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetVertexDeclaration;
    cmd.setVertexDeclaration.vertexDeclaration = vertexDeclaration;
    EnqueueRenderCommand(cmd);

    device->vertexDeclaration = g_memory.MapVirtual(vertexDeclaration);
}

static void ProcSetVertexDeclaration(const RenderCommand& cmd)
{
    auto& args = cmd.setVertexDeclaration;

    if (args.vertexDeclaration != nullptr)
    {
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedTexcoords, args.vertexDeclaration->swappedTexcoords);
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedNormals, args.vertexDeclaration->swappedNormals);
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedBinormals, args.vertexDeclaration->swappedBinormals);
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedTangents, args.vertexDeclaration->swappedTangents);
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedBlendWeights, args.vertexDeclaration->swappedBlendWeights);

        uint32_t specConstants = g_pipelineState.specConstants;
        if (args.vertexDeclaration->hasR11G11B10Normal)
            specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;
        else
            specConstants &= ~SPEC_CONSTANT_R11G11B10_NORMAL;

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
    }
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, args.vertexDeclaration);
}

static ShaderCacheEntry* FindShaderCacheEntry(XXH64_hash_t hash)
{
    auto end = g_shaderCacheEntries + g_shaderCacheEntryCount;
    auto findResult = std::lower_bound(g_shaderCacheEntries, end, hash, [](ShaderCacheEntry& lhs, XXH64_hash_t rhs)
        {
            return lhs.hash < rhs;
        });

    return findResult != end && findResult->hash == hash ? findResult : nullptr;
}

static GuestShader* GetOrCreateShader(XXH64_hash_t hash, ResourceType resourceType) {
    auto findResult = FindShaderCacheEntry(hash);
    GuestShader* shader = nullptr;

    if (findResult == nullptr)
    {
        LOGF_WARNING("Shader not found by hash: {:x}", hash);
        LOG_WARNING("Perhaps the path to the required shader will be printed before this error");
        __builtin_trap();
    }
    if (findResult != nullptr)
    {
        if (findResult->guestShader == nullptr)
        {
            shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);
            shader->shaderCacheEntry = findResult;
#ifdef ASYNC_PSO_DEBUG
            shader->name = findResult->filename;
#endif

            findResult->guestShader = shader;
        }
        else
        {
            shader = findResult->guestShader;
        }
    }

    if (shader == nullptr)
        shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);
    else
        shader->AddRef();

    return shader;
}

static GuestShader* CreateShader(const be<uint32_t>* function, ResourceType resourceType)
{
    XXH64_hash_t hash = XXH3_64bits(function, function[1] + function[2]);
    return GetOrCreateShader(hash, resourceType);
}

static GuestShader* CreateVertexShader(const be<uint32_t>* function) 
{
    return CreateShader(function, ResourceType::VertexShader);
}

static void SetVertexShader(GuestDevice* device, GuestShader* shader)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetVertexShader;
    cmd.setVertexShader.shader = shader;
    EnqueueRenderCommand(cmd);
}

static void ProcSetVertexShader(const RenderCommand& cmd)
{
    GuestShader* shader = cmd.setVertexShader.shader;
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, shader);
}

static void SetStreamSource(GuestDevice* device, uint32_t index, GuestBuffer* buffer, uint32_t offset, uint32_t stride) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetStreamSource;
    cmd.setStreamSource.index = index;
    cmd.setStreamSource.buffer = buffer;
    cmd.setStreamSource.offset = offset;
    cmd.setStreamSource.stride = stride;
    EnqueueRenderCommand(cmd);
}

static void ProcSetStreamSource(const RenderCommand& cmd)
{
    const auto& args = cmd.setStreamSource;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[args.index], uint8_t(args.buffer != nullptr ? args.stride : 0));

    bool dirty = false;

    SetDirtyValue(dirty, g_vertexBufferViews[args.index].buffer, args.buffer != nullptr ? args.buffer->buffer->at(args.offset) : RenderBufferReference{});
    SetDirtyValue(dirty, g_vertexBufferViews[args.index].size, args.buffer != nullptr ? (args.buffer->dataSize - args.offset) : 0u);
    SetDirtyValue(dirty, g_inputSlots[args.index].stride, args.buffer != nullptr ? args.stride : 0u);

    if (dirty)
    {
        g_dirtyStates.vertexStreamFirst = std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, args.index);
        g_dirtyStates.vertexStreamLast = std::max<uint8_t>(g_dirtyStates.vertexStreamLast, args.index);
    }
}

static void SetIndices(GuestDevice* device, GuestBuffer* buffer) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetIndices;
    cmd.setIndices.buffer = buffer;
    EnqueueRenderCommand(cmd);
}

static void ProcSetIndices(const RenderCommand& cmd)
{
    const auto& args = cmd.setIndices;

    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, args.buffer != nullptr ? args.buffer->buffer->at(0) : RenderBufferReference{});
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, args.buffer != nullptr ? args.buffer->format : RenderFormat::R16_UINT);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, args.buffer != nullptr ? args.buffer->dataSize : 0u);
}

static GuestShader* CreatePixelShader(const be<uint32_t>* function)
{
    return CreateShader(function, ResourceType::PixelShader);
}

static void SetPixelShader(GuestDevice* device, GuestShader* shader)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetPixelShader;
    cmd.setPixelShader.shader = shader;
    EnqueueRenderCommand(cmd);
}

static void ProcSetPixelShader(const RenderCommand& cmd)
{
    GuestShader* shader = cmd.setPixelShader.shader;
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, shader);
}

static void BeginConditionalSurvey(GuestDevice* device, uint32_t index)
{
    assert(index < CONDITIONAL_SURVEY_MAX && "Invalid conditional survey index.");

    RenderCommand cmd;
    cmd.type = RenderCommandType::SetConditionalSurvey;
    cmd.setConditionalSurvey.enabled = true;
    cmd.setConditionalSurvey.index = index;
    EnqueueRenderCommand(cmd);
}

static void EndConditionalSurvey(GuestDevice* device)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetConditionalSurvey;
    cmd.setConditionalSurvey.enabled = false;
    cmd.setConditionalSurvey.index = 0;
    EnqueueRenderCommand(cmd);
}

static void ProcSetConditionalSurvey(const RenderCommand& cmd)
{
    if (!g_hasConditionalSurvey)
    {
        g_skipSurveyDraws = cmd.setConditionalSurvey.enabled;
        return;
    }

    if (cmd.setConditionalSurvey.enabled)
    {
        // Clear previous survey result first.
        auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(sizeof(uint32_t)));
        memset(uploadBuffer->map(), 0, sizeof(uint32_t));
        uploadBuffer->unmap();

        auto& commandList = g_commandLists[g_frame];
        commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(g_conditionalSurveyBuffer.get(), RenderBufferAccess::WRITE));
        commandList->copyBufferRegion(g_conditionalSurveyBuffer->at(cmd.setConditionalSurvey.index * sizeof(uint32_t)), uploadBuffer->at(0), sizeof(uint32_t));
        commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(g_conditionalSurveyBuffer.get(), RenderBufferAccess::READ | RenderBufferAccess::WRITE));

        g_tempBuffers[g_frame].emplace_back(std::move(uploadBuffer));
    }
    else if (g_pipelineState.enableConditionalSurvey)
    {
        // The consumer can read a result written by another tile in the survey pass.
        g_commandLists[g_frame]->barriers(RenderBarrierStage::GRAPHICS,
            RenderBufferBarrier(g_conditionalSurveyBuffer.get(), RenderBufferAccess::READ));
    }

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.enableConditionalSurvey, cmd.setConditionalSurvey.enabled);
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.conditionalSurveyIndex, cmd.setConditionalSurvey.index);
}

static void BeginConditionalRendering(GuestDevice* device, uint32_t index)
{
    assert(index < CONDITIONAL_SURVEY_MAX && "Invalid conditional rendering index.");

    RenderCommand cmd;
    cmd.type = RenderCommandType::SetConditionalRendering;
    cmd.setConditionalRendering.enabled = true;
    cmd.setConditionalRendering.index = index;
    EnqueueRenderCommand(cmd);
}

static void EndConditionalRendering(GuestDevice* device)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetConditionalRendering;
    cmd.setConditionalRendering.enabled = false;
    cmd.setConditionalRendering.index = 0;
    EnqueueRenderCommand(cmd);
}

static void ProcSetConditionalRendering(const RenderCommand& cmd)
{
    uint32_t specConstants = g_pipelineState.specConstants;
    if (g_hasConditionalSurvey && cmd.setConditionalRendering.enabled)
        specConstants |= SPEC_CONSTANT_CONDITIONAL_RENDERING;
    else
        specConstants &= ~SPEC_CONSTANT_CONDITIONAL_RENDERING;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.conditionalRenderingIndex, cmd.setConditionalRendering.index);
}

static void SetClipPlane(GuestDevice* device, uint32_t index, const be<float>* plane)
{
    if (index != 0)
        return;

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.clipPlane[0], plane[0].get());
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.clipPlane[1], plane[1].get());
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.clipPlane[2], plane[2].get());
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.clipPlane[3], plane[3].get());
}

static std::thread g_renderThread([]
    {
        os::thread::SetCurrentThreadName("Render Thread");
#ifdef __ANDROID__
        g_isRenderThread = true;
        os::android::overlay::SetThread(os::android::overlay::Role::Render, gettid());
        os::android::hint::SetThread(os::android::hint::Role::Render, gettid());
#endif

#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

        const auto dispatch = [](const RenderCommand& cmd)
        {
            switch (cmd.type)
            {
            case RenderCommandType::SetRenderState:                    ProcSetRenderState(cmd); break;
            case RenderCommandType::DestructResource:                  ProcDestructResource(cmd); break;
            case RenderCommandType::UnlockTextureRect:                 ProcUnlockTextureRect(cmd); break;
            case RenderCommandType::UnlockBuffer:                      ProcUnlockBuffer(cmd); break;
            case RenderCommandType::DrawImGui:                         ProcDrawImGui(cmd); break;
            case RenderCommandType::ExecuteCommandList:                ProcExecuteCommandList(cmd); break;
            case RenderCommandType::BeginCommandList:                  ProcBeginCommandList(cmd); break;
            case RenderCommandType::StretchRect:                       ProcStretchRect(cmd); break;
            case RenderCommandType::SetRenderTarget:                   ProcSetRenderTarget(cmd); break;
            case RenderCommandType::SetDepthStencilSurface:            ProcSetDepthStencilSurface(cmd); break;
            case RenderCommandType::ExecutePendingStretchRectCommands: ProcExecutePendingStretchRectCommands(cmd); break;
            case RenderCommandType::Clear:                             ProcClear(cmd); break;
            case RenderCommandType::SetViewport:                       ProcSetViewport(cmd); break;
            case RenderCommandType::SetTexture:                        ProcSetTexture(cmd); break;
            case RenderCommandType::SetScissorRect:                    ProcSetScissorRect(cmd); break;
            case RenderCommandType::SetSamplerState:                   ProcSetSamplerState(cmd); break;
            case RenderCommandType::SetBooleans:                       ProcSetBooleans(cmd); break;
            case RenderCommandType::SetVertexShaderConstants:          ProcSetVertexShaderConstants(cmd); break;
            case RenderCommandType::SetPixelShaderConstants:           ProcSetPixelShaderConstants(cmd); break;
            case RenderCommandType::SetDrawShaderConstants:            ProcSetDrawShaderConstants(cmd); break;
            case RenderCommandType::AddPipeline:                       ProcAddPipeline(cmd); break;
            case RenderCommandType::DrawPrimitive:                     ProcDrawPrimitive(cmd); break;
            case RenderCommandType::DrawIndexedPrimitive:              ProcDrawIndexedPrimitive(cmd); break;
            case RenderCommandType::DrawPrimitiveUP:                   ProcDrawPrimitiveUP(cmd); break;
            case RenderCommandType::SetVertexDeclaration:              ProcSetVertexDeclaration(cmd); break;
            case RenderCommandType::SetVertexShader:                   ProcSetVertexShader(cmd); break;
            case RenderCommandType::SetStreamSource:                   ProcSetStreamSource(cmd); break;
            case RenderCommandType::SetIndices:                        ProcSetIndices(cmd); break;
            case RenderCommandType::SetPixelShader:                    ProcSetPixelShader(cmd); break;
            case RenderCommandType::SetConditionalSurvey:              ProcSetConditionalSurvey(cmd); break;
            case RenderCommandType::SetConditionalRendering:           ProcSetConditionalRendering(cmd); break;
            default:                                                   assert(false && "Unrecognized render command type."); break;
            }
        };

        RenderCommand commands[32];
        std::vector<RenderCommand> deferredResources;
        bool recording = true; // CreateHostDevice begins the first list before the guest starts.
        bool loggedDeferredUpload = false;

        while (true)
        {
            size_t count = g_renderQueue.wait_dequeue_bulk(commands, std::size(commands));

            for (size_t i = 0; i < count; i++)
            {
                const auto& cmd = commands[i];
                if (g_stageGuestBuffers && !recording)
                {
                    const bool upload = cmd.type == RenderCommandType::UnlockBuffer ||
                        cmd.type == RenderCommandType::UnlockTextureRect;
                    if (upload || cmd.type == RenderCommandType::DestructResource)
                    {
                        // Keep uploads and their resource releases in order across the frame boundary.
                        deferredResources.push_back(cmd);
                        if (upload && !loggedDeferredUpload)
                        {
                            LOG("Deferred a resource upload until the next command list.");
                            loggedDeferredUpload = true;
                        }
                        continue;
                    }
                }

                dispatch(cmd);
                if (cmd.type == RenderCommandType::ExecuteCommandList)
                {
                    recording = false;
                }
                else if (cmd.type == RenderCommandType::BeginCommandList)
                {
                    recording = true;
                    for (const auto& pending : deferredResources)
                        dispatch(pending);
                    deferredResources.clear();
                }
            }
        }
    });

struct GuestPictureData
{
    be<uint32_t> vtable;
    uint8_t flags;
    be<uint32_t> name;
    be<uint32_t> texture;
    be<uint32_t> type;
};

union Bxty {
    char _Buf[16];
    xpointer<uint32_t> _Ptr;
};

struct BEString
{
    char alval[1];
    Bxty bx;
    be<uint32_t> size;
    uint32_t res;

    const char* c_str() {
        uint32_t len = size.get();
        if (len == 0) {
            return alval;
        } else if (len < 16) {
            return bx._Buf;
        } else {
            return reinterpret_cast<const char*>(bx._Ptr.get());
        }
    }
};

struct GuestMyTexture
{
    be<uint32_t> vtable; // 0x0
    be<uint32_t> field0x4; // 0x4
    be<uint32_t> regIndex; // 0x8
    BEString str1; // 0xC
    BEString str2; // 0x28
    BEString str3; // 0x44
    be<uint32_t> byte60; // 0x60
    be<uint32_t> texture; // 0x64
    be<uint32_t> Surface[6]; // 0x64
    be<uint32_t> width; // 0x80
    be<uint32_t> height; // 0x84
    be<uint32_t> graphicsDevice; // 0x88
    be<uint32_t> guestDevice; // 0x8C
};

static RenderTextureDimension ConvertTextureDimension(ddspp::TextureType type)
{
    switch (type) 
    {
    case ddspp::Texture1D:
        return RenderTextureDimension::TEXTURE_1D;
    case ddspp::Texture2D:
    case ddspp::Cubemap:
        return RenderTextureDimension::TEXTURE_2D;
    case ddspp::Texture3D:
        return RenderTextureDimension::TEXTURE_3D;
    default:
        assert(false && "Unknown texture type from DDS.");
        return RenderTextureDimension::UNKNOWN;
    }
}

static RenderTextureViewDimension ConvertTextureViewDimension(ddspp::TextureType type)
{
    switch (type)
    {
    case ddspp::Texture1D:
        return RenderTextureViewDimension::TEXTURE_1D;
    case ddspp::Texture2D:
        return RenderTextureViewDimension::TEXTURE_2D;
    case ddspp::Texture3D:
        return RenderTextureViewDimension::TEXTURE_3D;
    case ddspp::Cubemap:
        return RenderTextureViewDimension::TEXTURE_CUBE;
    default:
        assert(false && "Unknown texture type from DDS.");
        return RenderTextureViewDimension::UNKNOWN;
    }
}

static RenderFormat ConvertDXGIFormat(ddspp::DXGIFormat format) 
{
    switch (format)
    {
    case ddspp::R32G32B32A32_TYPELESS:
        return RenderFormat::R32G32B32A32_TYPELESS;
    case ddspp::R32G32B32A32_FLOAT:
        return RenderFormat::R32G32B32A32_FLOAT;
    case ddspp::R32G32B32A32_UINT:
        return RenderFormat::R32G32B32A32_UINT;
    case ddspp::R32G32B32A32_SINT:
        return RenderFormat::R32G32B32A32_SINT;
    case ddspp::R32G32B32_TYPELESS:
        return RenderFormat::R32G32B32_TYPELESS;
    case ddspp::R32G32B32_FLOAT:
        return RenderFormat::R32G32B32_FLOAT;
    case ddspp::R32G32B32_UINT:
        return RenderFormat::R32G32B32_UINT;
    case ddspp::R32G32B32_SINT:
        return RenderFormat::R32G32B32_SINT;
    case ddspp::R16G16B16A16_TYPELESS:
        return RenderFormat::R16G16B16A16_TYPELESS;
    case ddspp::R16G16B16A16_FLOAT:
        return RenderFormat::R16G16B16A16_FLOAT;
    case ddspp::R16G16B16A16_UNORM:
        return RenderFormat::R16G16B16A16_UNORM;
    case ddspp::R16G16B16A16_UINT:
        return RenderFormat::R16G16B16A16_UINT;
    case ddspp::R16G16B16A16_SNORM:
        return RenderFormat::R16G16B16A16_SNORM;
    case ddspp::R16G16B16A16_SINT:
        return RenderFormat::R16G16B16A16_SINT;
    case ddspp::R32G32_TYPELESS:
        return RenderFormat::R32G32_TYPELESS;
    case ddspp::R32G32_FLOAT:
        return RenderFormat::R32G32_FLOAT;
    case ddspp::R32G32_UINT:
        return RenderFormat::R32G32_UINT;
    case ddspp::R32G32_SINT:
        return RenderFormat::R32G32_SINT;
    case ddspp::R8G8B8A8_TYPELESS:
        return RenderFormat::R8G8B8A8_TYPELESS;
    case ddspp::R8G8B8A8_UNORM:
        return RenderFormat::R8G8B8A8_UNORM;
    case ddspp::R8G8B8A8_UINT:
        return RenderFormat::R8G8B8A8_UINT;
    case ddspp::R8G8B8A8_SNORM:
        return RenderFormat::R8G8B8A8_SNORM;
    case ddspp::R8G8B8A8_SINT:
        return RenderFormat::R8G8B8A8_SINT;
    case ddspp::B8G8R8A8_UNORM:
        return RenderFormat::B8G8R8A8_UNORM;
    case ddspp::B8G8R8X8_UNORM:
        return RenderFormat::B8G8R8A8_UNORM;   
    case ddspp::R16G16_TYPELESS:
        return RenderFormat::R16G16_TYPELESS;
    case ddspp::R16G16_FLOAT:
        return RenderFormat::R16G16_FLOAT;
    case ddspp::R16G16_UNORM:
        return RenderFormat::R16G16_UNORM;
    case ddspp::R16G16_UINT:
        return RenderFormat::R16G16_UINT;
    case ddspp::R16G16_SNORM:
        return RenderFormat::R16G16_SNORM;
    case ddspp::R16G16_SINT:
        return RenderFormat::R16G16_SINT;
    case ddspp::R32_TYPELESS:
        return RenderFormat::R32_TYPELESS;
    case ddspp::D32_FLOAT:
        return RenderFormat::D32_FLOAT;
    case ddspp::R32_FLOAT:
        return RenderFormat::R32_FLOAT;
    case ddspp::R32_UINT:
        return RenderFormat::R32_UINT;
    case ddspp::R32_SINT:
        return RenderFormat::R32_SINT;
    case ddspp::R8G8_TYPELESS:
        return RenderFormat::R8G8_TYPELESS;
    case ddspp::R8G8_UNORM:
        return RenderFormat::R8G8_UNORM;
    case ddspp::R8G8_UINT:
        return RenderFormat::R8G8_UINT;
    case ddspp::R8G8_SNORM:
        return RenderFormat::R8G8_SNORM;
    case ddspp::R8G8_SINT:
        return RenderFormat::R8G8_SINT;
    case ddspp::R16_TYPELESS:
        return RenderFormat::R16_TYPELESS;
    case ddspp::R16_FLOAT:
        return RenderFormat::R16_FLOAT;
    case ddspp::D16_UNORM:
        return RenderFormat::D16_UNORM;
    case ddspp::R16_UNORM:
        return RenderFormat::R16_UNORM;
    case ddspp::R16_UINT:
        return RenderFormat::R16_UINT;
    case ddspp::R16_SNORM:
        return RenderFormat::R16_SNORM;
    case ddspp::R16_SINT:
        return RenderFormat::R16_SINT;
    case ddspp::R8_TYPELESS:
        return RenderFormat::R8_TYPELESS;
    case ddspp::R8_UNORM:
    case ddspp::A8_UNORM:
        return RenderFormat::R8_UNORM;
    case ddspp::R8_UINT:
        return RenderFormat::R8_UINT;
    case ddspp::R8_SNORM:
        return RenderFormat::R8_SNORM;
    case ddspp::R8_SINT:
        return RenderFormat::R8_SINT;
    case ddspp::BC1_TYPELESS:
        return RenderFormat::BC1_TYPELESS;
    case ddspp::BC1_UNORM:
        return RenderFormat::BC1_UNORM;
    case ddspp::BC1_UNORM_SRGB:
        return RenderFormat::BC1_UNORM_SRGB;
    case ddspp::BC2_TYPELESS:
        return RenderFormat::BC2_TYPELESS;
    case ddspp::BC2_UNORM:
        return RenderFormat::BC2_UNORM;
    case ddspp::BC2_UNORM_SRGB:
        return RenderFormat::BC2_UNORM_SRGB;
    case ddspp::BC3_TYPELESS:
        return RenderFormat::BC3_TYPELESS;
    case ddspp::BC3_UNORM:
        return RenderFormat::BC3_UNORM;
    case ddspp::BC3_UNORM_SRGB:
        return RenderFormat::BC3_UNORM_SRGB;
    case ddspp::BC4_TYPELESS:
        return RenderFormat::BC4_TYPELESS;
    case ddspp::BC4_UNORM:
        return RenderFormat::BC4_UNORM;
    case ddspp::BC4_SNORM:
        return RenderFormat::BC4_SNORM;
    case ddspp::BC5_TYPELESS:
        return RenderFormat::BC5_TYPELESS;
    case ddspp::BC5_UNORM:
        return RenderFormat::BC5_UNORM;
    case ddspp::BC5_SNORM:
        return RenderFormat::BC5_SNORM;
    case ddspp::BC6H_TYPELESS:
        return RenderFormat::BC6H_TYPELESS;
    case ddspp::BC6H_UF16:
        return RenderFormat::BC6H_UF16;
    case ddspp::BC6H_SF16:
        return RenderFormat::BC6H_SF16;
    case ddspp::BC7_TYPELESS:
        return RenderFormat::BC7_TYPELESS;
    case ddspp::BC7_UNORM:
        return RenderFormat::BC7_UNORM;
    case ddspp::BC7_UNORM_SRGB:
        return RenderFormat::BC7_UNORM_SRGB;
    default:
        printf("format: %x\n", format);
        assert(false && "Unsupported format from DDS.");
        return RenderFormat::UNKNOWN;
    }
}

#ifdef __ANDROID__
// CPU fallbacks for devices without BC sampled-image support (stock Mali/PowerVR drivers), ported
// from UnleashedRecomp-Android. Preferred path: transcode to ETC2/EAC with etcpak, which keeps the
// BC memory footprint (except transparent BC1, which doubles). When ETC2 is unavailable too,
// decode to plain RGBA8/R8/RG8 at 4-8x the BC footprint.
enum class BCnFallbackMode
{
    None,
    Decode,
    Transcode,
};

struct BCnFallback
{
    BCnFallbackMode mode = BCnFallbackMode::None;
    RenderFormat targetFormat{};
    uint32_t sourceBlockSize = 0;
    uint32_t bytesPerPixel = 0;   // Decode: decoded pixel size.
    uint32_t targetBlockSize = 0; // Transcode: ETC2/EAC bytes per 4x4 block.
    void (*decodeBlock)(const void* compressedBlock, void* decompressedBlock, int destinationPitch) = nullptr;
    void (*encodeBlockRow)(const uint32_t* src, uint64_t* dst, uint32_t blocks, size_t width) = nullptr;
};

// etcpak consumes pixels as b,g,r,a in memory while bcdec emits r,g,b,a: swap R and B
// while decoding into the intermediate buffer.
template<void (*TDecoder)(const void*, void*, int)>
static void DecodeBlockSwapRB(const void* src, void* dst, int pitch)
{
    uint8_t block[4][16];
    TDecoder(src, block, 16);

    for (size_t y = 0; y < 4; y++)
    {
        uint8_t* row = reinterpret_cast<uint8_t*>(dst) + y * pitch;
        for (size_t x = 0; x < 4; x++)
        {
            row[x * 4 + 0] = block[y][x * 4 + 2];
            row[x * 4 + 1] = block[y][x * 4 + 1];
            row[x * 4 + 2] = block[y][x * 4 + 0];
            row[x * 4 + 3] = block[y][x * 4 + 3];
        }
    }
}

static void DecodeBC4BlockForEac(const void* src, void* dst, int pitch)
{
    uint8_t block[4][4];
    bcdec_bc4(src, block, 4);

    for (size_t y = 0; y < 4; y++)
    {
        uint8_t* row = reinterpret_cast<uint8_t*>(dst) + y * pitch;
        for (size_t x = 0; x < 4; x++)
        {
            row[x * 4 + 0] = 0;
            row[x * 4 + 1] = 0;
            row[x * 4 + 2] = block[y][x]; // etcpak's EAC R channel lives in byte 2.
            row[x * 4 + 3] = 0xFF;
        }
    }
}

static void DecodeBC5BlockForEac(const void* src, void* dst, int pitch)
{
    uint8_t block[4][8];
    bcdec_bc5(src, block, 8);

    for (size_t y = 0; y < 4; y++)
    {
        uint8_t* row = reinterpret_cast<uint8_t*>(dst) + y * pitch;
        for (size_t x = 0; x < 4; x++)
        {
            row[x * 4 + 0] = 0;
            row[x * 4 + 1] = block[y][x * 2 + 1];
            row[x * 4 + 2] = block[y][x * 2 + 0];
            row[x * 4 + 3] = 0xFF;
        }
    }
}

// DXT1 stores punch-through alpha in blocks where color0 <= color1 and an index equals 3.
// Scanning the raw blocks decides between ETC2 RGB (opaque, same size as BC1) and ETC2
// RGBA (transparent, double size) without a full decode pass.
static bool BC1HasTransparency(const uint8_t* blocks, size_t blockCount)
{
    for (size_t i = 0; i < blockCount; i++, blocks += 8)
    {
        const uint16_t c0 = blocks[0] | (blocks[1] << 8);
        const uint16_t c1 = blocks[2] | (blocks[3] << 8);
        if (c0 > c1)
            continue;

        const uint32_t indices = blocks[4] | (blocks[5] << 8) | (blocks[6] << 16) | (uint32_t(blocks[7]) << 24);
        for (uint32_t j = 0; j < 16; j++)
        {
            if (((indices >> (j * 2)) & 0x3) == 0x3)
                return true;
        }
    }

    return false;
}

static bool GetBCnFallback(RenderFormat format, const uint8_t* data, size_t dataSize, uint32_t headerSize, BCnFallback& fallback)
{
    const bool etc2 = g_capabilities.textureCompressionETC2;

    switch (format)
    {
    case RenderFormat::BC4_SNORM:
    case RenderFormat::BC5_SNORM:
        LOG_WARNING("Converting SNORM BC texture as UNORM; values will be biased.");
        break;
    case RenderFormat::BC1_UNORM_SRGB:
    case RenderFormat::BC2_UNORM_SRGB:
    case RenderFormat::BC3_UNORM_SRGB:
    case RenderFormat::BC7_UNORM_SRGB:
        // The transcode targets have sRGB variants; only the plain decode loses the automatic
        // sRGB-to-linear conversion (RenderFormat has no R8G8B8A8_UNORM_SRGB).
        if (!etc2)
        {
            static bool s_warnedSrgb;
            if (!s_warnedSrgb)
            {
                s_warnedSrgb = true;
                LOG_WARNING("Decoding sRGB BC texture to UNORM: sampling will return sRGB-encoded values.");
            }
        }
        break;
    default:
        break;
    }

    switch (format)
    {
    case RenderFormat::BC1_TYPELESS:
    case RenderFormat::BC1_UNORM:
    case RenderFormat::BC1_UNORM_SRGB:
    {
        const bool srgb = format == RenderFormat::BC1_UNORM_SRGB;
        fallback.sourceBlockSize = BCDEC_BC1_BLOCK_SIZE;
        if (etc2)
        {
            fallback.mode = BCnFallbackMode::Transcode;
            fallback.decodeBlock = &DecodeBlockSwapRB<&bcdec_bc1>;
            if (BC1HasTransparency(data + headerSize, (dataSize - headerSize) / BCDEC_BC1_BLOCK_SIZE))
            {
                fallback.targetFormat = srgb ? RenderFormat::ETC2_RGBA8_UNORM_SRGB : RenderFormat::ETC2_RGBA8_UNORM;
                fallback.targetBlockSize = 16;
                fallback.encodeBlockRow = [](const uint32_t* src, uint64_t* dst, uint32_t blocks, size_t width) { CompressEtc2Rgba(src, dst, blocks, width, true); };
            }
            else
            {
                fallback.targetFormat = srgb ? RenderFormat::ETC2_RGB8_UNORM_SRGB : RenderFormat::ETC2_RGB8_UNORM;
                fallback.targetBlockSize = 8;
                fallback.encodeBlockRow = [](const uint32_t* src, uint64_t* dst, uint32_t blocks, size_t width) { CompressEtc2Rgb(src, dst, blocks, width, true); };
            }
        }
        else
        {
            fallback.mode = BCnFallbackMode::Decode;
            fallback.targetFormat = RenderFormat::R8G8B8A8_UNORM;
            fallback.bytesPerPixel = 4;
            fallback.decodeBlock = &bcdec_bc1;
        }
        return true;
    }
    case RenderFormat::BC2_TYPELESS:
    case RenderFormat::BC2_UNORM:
    case RenderFormat::BC2_UNORM_SRGB:
    case RenderFormat::BC3_TYPELESS:
    case RenderFormat::BC3_UNORM:
    case RenderFormat::BC3_UNORM_SRGB:
    case RenderFormat::BC7_TYPELESS:
    case RenderFormat::BC7_UNORM:
    case RenderFormat::BC7_UNORM_SRGB:
    {
        const bool srgb = format == RenderFormat::BC2_UNORM_SRGB || format == RenderFormat::BC3_UNORM_SRGB || format == RenderFormat::BC7_UNORM_SRGB;
        const bool bc2 = format == RenderFormat::BC2_TYPELESS || format == RenderFormat::BC2_UNORM || format == RenderFormat::BC2_UNORM_SRGB;
        const bool bc3 = format == RenderFormat::BC3_TYPELESS || format == RenderFormat::BC3_UNORM || format == RenderFormat::BC3_UNORM_SRGB;
        fallback.sourceBlockSize = BCDEC_BC3_BLOCK_SIZE;
        if (etc2)
        {
            fallback.mode = BCnFallbackMode::Transcode;
            fallback.targetFormat = srgb ? RenderFormat::ETC2_RGBA8_UNORM_SRGB : RenderFormat::ETC2_RGBA8_UNORM;
            fallback.targetBlockSize = 16;
            fallback.decodeBlock = bc2 ? &DecodeBlockSwapRB<&bcdec_bc2> : (bc3 ? &DecodeBlockSwapRB<&bcdec_bc3> : &DecodeBlockSwapRB<&bcdec_bc7>);
            fallback.encodeBlockRow = [](const uint32_t* src, uint64_t* dst, uint32_t blocks, size_t width) { CompressEtc2Rgba(src, dst, blocks, width, true); };
        }
        else
        {
            fallback.mode = BCnFallbackMode::Decode;
            fallback.targetFormat = RenderFormat::R8G8B8A8_UNORM;
            fallback.bytesPerPixel = 4;
            fallback.decodeBlock = bc2 ? &bcdec_bc2 : (bc3 ? &bcdec_bc3 : &bcdec_bc7);
        }
        return true;
    }
    case RenderFormat::BC4_TYPELESS:
    case RenderFormat::BC4_UNORM:
    case RenderFormat::BC4_SNORM:
        fallback.sourceBlockSize = BCDEC_BC4_BLOCK_SIZE;
        if (etc2)
        {
            fallback.mode = BCnFallbackMode::Transcode;
            fallback.targetFormat = RenderFormat::EAC_R11_UNORM;
            fallback.targetBlockSize = 8;
            fallback.decodeBlock = &DecodeBC4BlockForEac;
            fallback.encodeBlockRow = &CompressEacR;
        }
        else
        {
            fallback.mode = BCnFallbackMode::Decode;
            fallback.targetFormat = RenderFormat::R8_UNORM;
            fallback.bytesPerPixel = 1;
            fallback.decodeBlock = &bcdec_bc4;
        }
        return true;
    case RenderFormat::BC5_TYPELESS:
    case RenderFormat::BC5_UNORM:
    case RenderFormat::BC5_SNORM:
        fallback.sourceBlockSize = BCDEC_BC5_BLOCK_SIZE;
        if (etc2)
        {
            fallback.mode = BCnFallbackMode::Transcode;
            fallback.targetFormat = RenderFormat::EAC_R11G11_UNORM;
            fallback.targetBlockSize = 16;
            fallback.decodeBlock = &DecodeBC5BlockForEac;
            fallback.encodeBlockRow = &CompressEacRg;
        }
        else
        {
            fallback.mode = BCnFallbackMode::Decode;
            fallback.targetFormat = RenderFormat::R8G8_UNORM;
            fallback.bytesPerPixel = 2;
            fallback.decodeBlock = &bcdec_bc5;
        }
        return true;
    default:
        // BC6H stays unsupported: bcdec only emits 3-channel float output for it.
        return false;
    }
}
#endif

// Android uses single-sample surfaces in CreateSurface, so the PSO cache needs only those variants.
static bool IsMsaaInUse()
{
#ifdef __ANDROID__
    return false;
#else
    return Config::AntiAliasing != EAntiAliasing::Off;
#endif
}

static bool LoadTexture(GuestTexture& texture, const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping)
{
    ddspp::Descriptor ddsDesc;
    if (ddspp::decode_header((unsigned char *)(data), ddsDesc) != ddspp::Error)
    {
        RenderTextureDesc desc;
        desc.dimension = ConvertTextureDimension(ddsDesc.type);
        desc.width = ddsDesc.width;
        desc.height = ddsDesc.height;
        desc.depth = ddsDesc.depth;
        desc.mipLevels = ddsDesc.numMips;
        desc.arraySize = ddsDesc.type == ddspp::TextureType::Cubemap ? ddsDesc.arraySize * 6 : ddsDesc.arraySize;
        desc.format = ConvertDXGIFormat(ddsDesc.format);
        desc.flags = ddsDesc.type == ddspp::TextureType::Cubemap ? RenderTextureFlag::CUBE : RenderTextureFlag::NONE;

#ifdef __ANDROID__
        BCnFallback bcFallback{};
        if (!g_capabilities.textureCompressionBC && ddsDesc.blockWidth > 1)
        {
            if (!GetBCnFallback(desc.format, data, dataSize, ddsDesc.headerSize, bcFallback))
            {
                LOGF_ERROR("Cannot load a BC-compressed texture (RenderFormat {}) on a device without BC texture support.", uint32_t(desc.format));
                return false;
            }

            desc.format = bcFallback.targetFormat;
        }
#endif

        texture.textureHolder = g_device->createTexture(desc);
        texture.texture = texture.textureHolder.get();
        texture.layout = RenderTextureLayout::COPY_DEST;

        RenderTextureViewDesc viewDesc;
        viewDesc.format = desc.format;
        viewDesc.dimension = ConvertTextureViewDimension(ddsDesc.type);
        viewDesc.mipLevels = ddsDesc.numMips;

        if (ddsDesc.format == ddspp::A8_UNORM)
        {
            // Map A8_UNORM to R8_UNORM for compatability
            componentMapping = RenderComponentMapping(RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::R);
        }

        viewDesc.componentMapping = componentMapping;
        texture.textureView = texture.texture->createTextureView(viewDesc);
        texture.descriptorIndex = g_textureDescriptorAllocator.allocate();
        SetTextureDescriptor(texture.descriptorIndex, texture.texture, RenderTextureLayout::SHADER_READ, texture.textureView.get());

        texture.width = ddsDesc.width;
        texture.height = ddsDesc.height;
        texture.mipLevels = viewDesc.mipLevels;
        texture.viewDimension = viewDesc.dimension;
        CreateArrayTextureView(texture, viewDesc);

        struct Slice
        {
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            uint32_t srcOffset;
            uint32_t dstOffset;
            uint32_t srcRowPitch;
            uint32_t dstRowPitch;
            uint32_t rowCount;
            uint32_t dstRowCount;
        };

        std::vector<Slice> slices;
        uint32_t curSrcOffset = 0;
        uint32_t curDstOffset = 0;

        for (uint32_t arraySlice = 0; arraySlice < desc.arraySize; arraySlice++)
        {
            for (uint32_t mipSlice = 0; mipSlice < ddsDesc.numMips; mipSlice++)
            {
                auto& slice = slices.emplace_back();

                slice.width = std::max(1u, ddsDesc.width >> mipSlice);
                slice.height = std::max(1u, ddsDesc.height >> mipSlice);
                slice.depth = std::max(1u, ddsDesc.depth >> mipSlice);
                slice.srcOffset = curSrcOffset;
                slice.dstOffset = curDstOffset;
                uint32_t rowPitch = ((slice.width + ddsDesc.blockWidth - 1) / ddsDesc.blockWidth) * ddsDesc.bitsPerPixelOrBlock;
                slice.srcRowPitch = (rowPitch + 7) / 8;
                slice.dstRowPitch = (slice.srcRowPitch + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
                slice.rowCount = (slice.height + ddsDesc.blockHeight - 1) / ddsDesc.blockHeight;
                slice.dstRowCount = slice.rowCount;

#ifdef __ANDROID__
                if (bcFallback.mode == BCnFallbackMode::Decode)
                {
                    // The upload buffer holds decoded pixels instead of the source BC blocks.
                    slice.dstRowPitch = (slice.width * bcFallback.bytesPerPixel + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
                    slice.dstRowCount = slice.height;
                }
                else if (bcFallback.mode == BCnFallbackMode::Transcode)
                {
                    // The upload buffer holds ETC2/EAC blocks with the same 4x4 geometry.
                    slice.dstRowPitch = (((slice.width + 3) / 4) * bcFallback.targetBlockSize + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
                }
#endif

                curSrcOffset += slice.srcRowPitch * slice.rowCount * slice.depth;
                curDstOffset += (slice.dstRowPitch * slice.dstRowCount * slice.depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
            }
        }

        const auto makeRegion = [&](size_t i)
        {
            const auto& slice = slices[i];
            uint32_t rowWidth = (slice.dstRowPitch * 8) / ddsDesc.bitsPerPixelOrBlock * ddsDesc.blockWidth;
#ifdef __ANDROID__
            if (bcFallback.mode == BCnFallbackMode::Decode)
                rowWidth = slice.dstRowPitch / bcFallback.bytesPerPixel;
            else if (bcFallback.mode == BCnFallbackMode::Transcode)
                rowWidth = slice.dstRowPitch / bcFallback.targetBlockSize * 4;
#endif
            return RenderBufferTextureCopyRegion { slice.dstOffset, slice.width, slice.height, slice.depth,
                rowWidth, uint32_t(i % desc.mipLevels), uint32_t(i / desc.mipLevels) };
        };

        const auto copyRawSlicesToStaging = [&](uint8_t* baseDst)
        {
            for (const auto& slice : slices)
            {
                const uint8_t* srcData = data + ddsDesc.headerSize + slice.srcOffset;
                uint8_t* dstData = baseDst + slice.dstOffset;

                if (slice.srcRowPitch == slice.dstRowPitch)
                {
                    memcpy(dstData, srcData, slice.srcRowPitch * slice.rowCount * slice.depth);
                }
                else
                {
                    for (size_t i = 0; i < slice.rowCount * slice.depth; i++)
                    {
                        memcpy(dstData, srcData, slice.srcRowPitch);
                        srcData += slice.srcRowPitch;
                        dstData += slice.dstRowPitch;
                    }
                }
            }
        };

        thread_local std::vector<RenderBufferTextureCopyRegion> copyRegions;
#ifdef __ANDROID__
        thread_local std::vector<uint8_t> s_transcoded;
#endif

        const bool useMultiBatch = g_batchMultiTextureUploads && App::s_isLoading &&
            curDstOffset > 0 && curDstOffset <= MAX_CACHED_DDS_UPLOAD_BYTES &&
            !slices.empty() && slices.size() <= UINT32_MAX;

        if (useMultiBatch)
        {
#ifdef __ANDROID__
            if (bcFallback.mode != BCnFallbackMode::None)
            {
                const bool transcode = bcFallback.mode == BCnFallbackMode::Transcode;
                const bool useTextureCache = transcode && os::android::texture_cache::IsEnabled();
                os::android::texture_cache::Key textureCacheKey;
                bool fromTextureCache = false;
                s_transcoded.resize(curDstOffset);
                if (useTextureCache)
                {
                    textureCacheKey = os::android::texture_cache::MakeKey(data, dataSize, uint32_t(desc.format), curDstOffset);
                    fromTextureCache = os::android::texture_cache::Read(textureCacheKey, s_transcoded.data(), curDstOffset);
                    if (!fromTextureCache)
                        std::fill(s_transcoded.begin(), s_transcoded.end(), 0);
                }

                if (!fromTextureCache)
                {
                    std::vector<uint8_t> decodeTemp;
                    const int64_t transcodeStart = transcode ? os::android::waits::NowNs() : 0;
                    for (const auto& slice : slices)
                    {
                        const uint8_t* srcData = data + ddsDesc.headerSize + slice.srcOffset;
                        uint8_t* dstData = s_transcoded.data() + slice.dstOffset;
                        const uint32_t blocksX = (slice.width + 3) / 4;
                        const uint32_t blocksY = slice.rowCount;
                        const uint32_t pixelSize = bcFallback.mode == BCnFallbackMode::Decode ? bcFallback.bytesPerPixel : 4;
                        const uint32_t tmpPitch = blocksX * 4 * pixelSize;
                        decodeTemp.resize(size_t(tmpPitch) * blocksY * 4);

                        for (uint32_t z = 0; z < slice.depth; z++)
                        {
                            const uint8_t* srcSlice = srcData + size_t(z) * slice.srcRowPitch * blocksY;
                            for (uint32_t blockY = 0; blockY < blocksY; blockY++)
                            {
                                for (uint32_t blockX = 0; blockX < blocksX; blockX++)
                                {
                                    bcFallback.decodeBlock(
                                        srcSlice + size_t(blockY) * slice.srcRowPitch + size_t(blockX) * bcFallback.sourceBlockSize,
                                        decodeTemp.data() + size_t(blockY) * 4 * tmpPitch + size_t(blockX) * 4 * pixelSize,
                                        int(tmpPitch));
                                }
                            }

                            if (bcFallback.mode == BCnFallbackMode::Decode)
                            {
                                uint8_t* dstSlice = dstData + size_t(z) * slice.dstRowPitch * slice.height;
                                for (uint32_t row = 0; row < slice.height; row++)
                                    memcpy(dstSlice + size_t(row) * slice.dstRowPitch, decodeTemp.data() + size_t(row) * tmpPitch, size_t(slice.width) * pixelSize);
                            }
                            else
                            {
                                uint8_t* dstSlice = dstData + size_t(z) * slice.dstRowPitch * blocksY;
                                for (uint32_t blockY = 0; blockY < blocksY; blockY++)
                                {
                                    bcFallback.encodeBlockRow(
                                        reinterpret_cast<const uint32_t*>(decodeTemp.data() + size_t(blockY) * 4 * tmpPitch),
                                        reinterpret_cast<uint64_t*>(dstSlice + size_t(blockY) * slice.dstRowPitch),
                                        blocksX, size_t(blocksX) * 4);
                                }
                            }
                        }
                    }

                    if (transcode)
                    {
                        os::android::texture_cache::CountTranscode(os::android::waits::NowNs() - transcodeStart, curDstOffset);
                        if (useTextureCache)
                            os::android::texture_cache::Write(textureCacheKey, s_transcoded.data(), curDstOffset);
                    }
                }
            }
#endif

            std::lock_guard lock(g_copyMutex);

            uint32_t baseOffset = (g_ddsCopyBatch.usedBytes + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
            if (g_ddsCopyBatch.recording &&
                (baseOffset + curDstOffset > g_ddsCopyBatch.upload.capacity || g_ddsCopyBatch.texturesInBatch >= MAX_TEXTURES_PER_DDS_BATCH))
            {
                FlushDdsCopyBatchLocked();
                baseOffset = 0;
            }

            bool reusedStaging = true;
            if (!g_ddsCopyBatch.recording)
            {
                auto upload = AcquireDdsUploadBuffer(MAX_CACHED_DDS_UPLOAD_BYTES);
                uint8_t* mapped = upload.buffer != nullptr ? reinterpret_cast<uint8_t*>(upload.buffer->map()) : nullptr;
                if (mapped == nullptr)
                    return false;

                reusedStaging = upload.reused;
                g_copyCommandList->begin();
                g_ddsCopyBatch.upload = std::move(upload);
                g_ddsCopyBatch.mappedMemory = mapped;
                g_ddsCopyBatch.usedBytes = 0;
                g_ddsCopyBatch.texturesInBatch = 0;
                g_ddsCopyBatch.batchId = ++g_nextCopyBatchId;
                g_ddsCopyBatch.recording = true;
                baseOffset = 0;
            }

#ifdef __ANDROID__
            if (bcFallback.mode != BCnFallbackMode::None)
                memcpy(g_ddsCopyBatch.mappedMemory + baseOffset, s_transcoded.data(), curDstOffset);
            else
#endif
                copyRawSlicesToStaging(g_ddsCopyBatch.mappedMemory + baseOffset);

            copyRegions.clear();
            copyRegions.reserve(slices.size());
            for (size_t i = 0; i < slices.size(); i++)
            {
                auto region = makeRegion(i);
                region.bufferOffset += baseOffset;
                copyRegions.push_back(region);
            }

            g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));
            g_copyCommandList->copyBufferToTexture(texture.texture, g_ddsCopyBatch.upload.buffer.get(), desc.format,
                copyRegions.data(), uint32_t(copyRegions.size()));

            g_ddsCopyBatch.usedBytes = baseOffset + curDstOffset;
            ++g_ddsCopyBatch.texturesInBatch;
            texture.pendingCopyBatchId = g_ddsCopyBatch.batchId;

#ifdef __ANDROID__
            if (g_profileTextureUploads)
                g_textureUploadDiagnostics.Add(reusedStaging, slices.size(), slices.size() > 1);
#endif
            return true;
        }

        auto upload = AcquireDdsUploadBuffer(curDstOffset);
        auto& uploadBuffer = upload.buffer;
        uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

#ifdef __ANDROID__
        std::vector<uint8_t> decodeTemp;

        // Transcoded textures: read from the texture cache, or transcode into a CPU buffer (the
        // mapped upload memory may be slow to read back) and store it.
        const bool transcode = bcFallback.mode == BCnFallbackMode::Transcode;
        const bool useTextureCache = transcode && os::android::texture_cache::IsEnabled();
        os::android::texture_cache::Key textureCacheKey;
        bool fromTextureCache = false;
        if (useTextureCache)
        {
            textureCacheKey = os::android::texture_cache::MakeKey(data, dataSize, uint32_t(desc.format), curDstOffset);
            fromTextureCache = os::android::texture_cache::Read(textureCacheKey, mappedMemory, curDstOffset);
            if (!fromTextureCache)
                s_transcoded.assign(curDstOffset, 0);
        }
        uint8_t* const transcodeTarget = useTextureCache ? s_transcoded.data() : mappedMemory;
        const int64_t transcodeStart = transcode ? os::android::waits::NowNs() : 0;
#endif
        for (auto& slice : slices)
        {
#ifdef __ANDROID__
            if (fromTextureCache)
                break;
#endif
            const uint8_t* srcData = data + ddsDesc.headerSize + slice.srcOffset;
            uint8_t* dstData = mappedMemory + slice.dstOffset;
#ifdef __ANDROID__
            if (transcode)
                dstData = transcodeTarget + slice.dstOffset;
#endif

#ifdef __ANDROID__
            if (bcFallback.mode != BCnFallbackMode::None)
            {
                // Decode into a buffer rounded up to whole 4x4 blocks first: edge blocks always
                // write a full 4x4 pixel area, which could otherwise run past this slice's region
                // in the upload buffer. Transcoding decodes to 4-byte pixels, which etcpak expects.
                const uint32_t blocksX = (slice.width + 3) / 4;
                const uint32_t blocksY = slice.rowCount;
                const uint32_t pixelSize = bcFallback.mode == BCnFallbackMode::Decode ? bcFallback.bytesPerPixel : 4;
                const uint32_t tmpPitch = blocksX * 4 * pixelSize;
                decodeTemp.resize(size_t(tmpPitch) * blocksY * 4);

                for (uint32_t z = 0; z < slice.depth; z++)
                {
                    const uint8_t* srcSlice = srcData + size_t(z) * slice.srcRowPitch * blocksY;

                    for (uint32_t blockY = 0; blockY < blocksY; blockY++)
                    {
                        for (uint32_t blockX = 0; blockX < blocksX; blockX++)
                        {
                            bcFallback.decodeBlock(
                                srcSlice + size_t(blockY) * slice.srcRowPitch + size_t(blockX) * bcFallback.sourceBlockSize,
                                decodeTemp.data() + size_t(blockY) * 4 * tmpPitch + size_t(blockX) * 4 * pixelSize,
                                int(tmpPitch));
                        }
                    }

                    if (bcFallback.mode == BCnFallbackMode::Decode)
                    {
                        uint8_t* dstSlice = dstData + size_t(z) * slice.dstRowPitch * slice.height;
                        for (uint32_t row = 0; row < slice.height; row++)
                            memcpy(dstSlice + size_t(row) * slice.dstRowPitch, decodeTemp.data() + size_t(row) * tmpPitch, size_t(slice.width) * pixelSize);
                    }
                    else
                    {
                        uint8_t* dstSlice = dstData + size_t(z) * slice.dstRowPitch * blocksY;
                        for (uint32_t blockY = 0; blockY < blocksY; blockY++)
                        {
                            bcFallback.encodeBlockRow(
                                reinterpret_cast<const uint32_t*>(decodeTemp.data() + size_t(blockY) * 4 * tmpPitch),
                                reinterpret_cast<uint64_t*>(dstSlice + size_t(blockY) * slice.dstRowPitch),
                                blocksX, size_t(blocksX) * 4);
                        }
                    }
                }

                continue;
            }
#endif

            if (slice.srcRowPitch == slice.dstRowPitch)
            {
                memcpy(dstData, srcData, slice.srcRowPitch * slice.rowCount * slice.depth);
            }
            else
            {
                for (size_t i = 0; i < slice.rowCount * slice.depth; i++)
                {
                    memcpy(dstData, srcData, slice.srcRowPitch);
                    srcData += slice.srcRowPitch;
                    dstData += slice.dstRowPitch;
                }
            }
        }

#ifdef __ANDROID__
        if (transcode && !fromTextureCache)
        {
            os::android::texture_cache::CountTranscode(os::android::waits::NowNs() - transcodeStart, curDstOffset);
            if (useTextureCache)
            {
                memcpy(mappedMemory, s_transcoded.data(), curDstOffset);
                os::android::texture_cache::Write(textureCacheKey, s_transcoded.data(), curDstOffset);
            }
        }
#endif
        uploadBuffer->unmap();

        const bool batchCopies = g_batchTextureUploads && slices.size() > 1 && slices.size() <= UINT32_MAX;
        if (batchCopies)
        {
            copyRegions.clear();
            copyRegions.reserve(slices.size());
            for (size_t i = 0; i < slices.size(); i++)
                copyRegions.push_back(makeRegion(i));
        }

        const bool copyCompleted = ExecuteCopyCommandList([&]
            {
                g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));

                if (batchCopies)
                {
                    g_copyCommandList->copyBufferToTexture(texture.texture, uploadBuffer.get(), desc.format,
                        copyRegions.data(), uint32_t(copyRegions.size()));
                    return;
                }

                for (size_t i = 0; i < slices.size(); i++)
                {
                    const auto region = makeRegion(i);
                    g_copyCommandList->copyTextureRegion(
                        RenderTextureCopyLocation::Subresource(texture.texture, region.mipLevel, region.arrayIndex),
                        RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), desc.format, region.width,
                            region.height, region.depth, region.rowWidth, region.bufferOffset));
                }
            });

        if (copyCompleted)
        {
#ifdef __ANDROID__
            if (g_profileTextureUploads)
            {
                g_textureUploadDiagnostics.Add(upload.reused, slices.size(), batchCopies);
                g_textureUploadDiagnostics.AddSubmission();
            }
#endif
            // The synchronous upload fence has released both the image and its staging buffer.
            RecycleDdsUploadBuffer(std::move(upload));
        }

        return true;
    }
    else
    {
        int width, height;
        void* stbImage = stbi_load_from_memory(data, dataSize, &width, &height, nullptr, 4);

        if (stbImage != nullptr)
        {
            texture.textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, RenderFormat::R8G8B8A8_UNORM));
            texture.texture = texture.textureHolder.get();
            texture.viewDimension = RenderTextureViewDimension::TEXTURE_2D;
            texture.layout = RenderTextureLayout::COPY_DEST;

            texture.descriptorIndex = g_textureDescriptorAllocator.allocate();
            SetTextureDescriptor(texture.descriptorIndex, texture.texture, RenderTextureLayout::SHADER_READ);

            RenderTextureViewDesc arrayViewDesc = RenderTextureViewDesc::Texture2D(RenderFormat::R8G8B8A8_UNORM);
            arrayViewDesc.mipLevels = 1;
            CreateArrayTextureView(texture, arrayViewDesc);

            uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
            uint32_t slicePitch = rowPitch * height;

            auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
            uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

            if (rowPitch == (width * 4))
            {
                memcpy(mappedMemory, stbImage, slicePitch);
            }
            else
            {
                auto data = reinterpret_cast<const uint8_t*>(stbImage);

                for (size_t i = 0; i < height; i++)
                {
                    memcpy(mappedMemory, data, width * 4);
                    data += width * 4;
                    mappedMemory += rowPitch;
                }
            }

            uploadBuffer->unmap();

            stbi_image_free(stbImage);

            ExecuteCopyCommandList([&]
                {
                    g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));

                    g_copyCommandList->copyTextureRegion(
                        RenderTextureCopyLocation::Subresource(texture.texture, 0),
                        RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
                });

            return true;
        }
    }

    return false;
}

std::unique_ptr<GuestTexture> LoadTexture(const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping)
{
    GuestTexture texture(ResourceType::Texture);

    if (LoadTexture(texture, data, dataSize, componentMapping))
        return std::make_unique<GuestTexture>(std::move(texture));

    return nullptr;
}

static void DiffPatchTexture(GuestTexture& texture, uint8_t* data, uint32_t dataSize)
{
    auto header = reinterpret_cast<BlockCompressionDiffPatchHeader*>(g_buttonBcDiff.get());
    auto entries = reinterpret_cast<BlockCompressionDiffPatchEntry*>(g_buttonBcDiff.get() + header->entriesOffset);
    auto end = entries + header->entryCount;
    
    auto hash = XXH3_64bits(data, dataSize);

    auto findResult = std::lower_bound(entries, end, hash, [](BlockCompressionDiffPatchEntry& lhs, XXH64_hash_t rhs)
    {
        return lhs.hash < rhs;
    });

    if (findResult != end && findResult->hash == hash)
    {
        auto patch = reinterpret_cast<BlockCompressionDiffPatch*>(g_buttonBcDiff.get() + findResult->patchesOffset);

        for (size_t i = 0; i < findResult->patchCount; i++)
        {
            assert(patch->destinationOffset + patch->patchBytesSize <= dataSize);
            memcpy(data + patch->destinationOffset, g_buttonBcDiff.get() + patch->patchBytesOffset, patch->patchBytesSize);
            ++patch;
        }

        GuestTexture patchedTexture(ResourceType::Texture);

        if (LoadTexture(patchedTexture, data, dataSize, {}))
            texture.patchedTexture = std::make_unique<GuestTexture>(std::move(patchedTexture));
    }
}

static void MakePictureData(GuestMyTexture* pictureData, uint8_t* data, uint32_t dataSize)
{
    if (data != nullptr)
    {
        GuestTexture texture(ResourceType::Texture);

        if (LoadTexture(texture, data, dataSize, {}))
        {
#ifdef _DEBUG
            if (pictureData->str1.size.get() > 0)
                texture.texture->setName(fmt::format("Texture {}", pictureData->str1.c_str()));
#endif
            DiffPatchTexture(texture, data, dataSize);

            pictureData->texture = g_memory.MapVirtual(g_userHeap.AllocPhysical<GuestTexture>(std::move(texture)));
            pictureData->width = texture.width;
            pictureData->height = texture.height;
        }
    }
}

void IndexBufferLengthMidAsmHook(PPCRegister& r3)
{
    r3.u64 *= 2;
}

void SetShadowResolutionMidAsmHook(PPCRegister& r11)
{
    auto res = static_cast<int32_t>(Config::GetEffectiveShadowResolution());

    if (res > 0)
        r11.u64 = res;
}

static void SetResolution(be<uint32_t>* device)
{
    Video::ComputeViewportDimensions();

    // The device (back buffer, 2D/HUD layout) always uses the full viewport size; the scaled
    // scene targets are created by Video::Resize once the game's render targets exist.
    uint32_t width = Video::s_viewportWidth;
    uint32_t height = Video::s_viewportHeight;
    device[46] = width == 0 ? 880 : width;
    device[47] = height == 0 ? 720 : height;

    if (Config::ResolutionScale != 1.0f)
        g_resizePending = true;
}

static GuestShader* g_movieVertexShader;
static GuestShader* g_moviePixelShaderHD;
static GuestShader* g_moviePixelShaderSD;

static int ScreenShaderInit(be<uint32_t>* a1)
{
    if (g_movieVertexShader == nullptr)
    {
        g_movieVertexShader = g_userHeap.AllocPhysical<GuestShader>(ResourceType::VertexShader);
    }

    if (g_moviePixelShaderHD == nullptr)
    {
        g_moviePixelShaderHD = g_userHeap.AllocPhysical<GuestShader>(ResourceType::PixelShader);
    }

    if (g_moviePixelShaderSD == nullptr)
    {
        g_moviePixelShaderSD = g_userHeap.AllocPhysical<GuestShader>(ResourceType::PixelShader);
    }

    g_moviePixelShaderHD->AddRef();
    g_moviePixelShaderSD->AddRef();
    g_movieVertexShader->AddRef();

    a1[0x12] = g_memory.MapVirtual(g_movieVertexShader);
    a1[0x51] = g_memory.MapVirtual(g_moviePixelShaderHD);
    a1[0x52] = g_memory.MapVirtual(g_moviePixelShaderSD);

    return 0;
}

// Needed for correct clearing of index buffer
static bool IsSet() {
    return true;
}

void MovieRendererMidAsmHook(PPCRegister& r3)
{
    auto device = reinterpret_cast<GuestDevice*>(g_memory.Translate(r3.u32));

    // Force linear filtering & clamp addressing
    for (size_t i = 0; i < 3; i++)
    {
        device->samplerStates[i].data[0] = (device->samplerStates[i].data[0].get() & ~0x7fc00) | 0x24800;
        device->samplerStates[i].data[3] = (device->samplerStates[i].data[3].get() & ~0x1f80000) | 0x1280000;
    }

    device->dirtyFlags[3] = device->dirtyFlags[3].get() | 0xe0000000ull;
}

// This is passed to pipeline compilation threads to keep the loading screen busy until
// all of them are finished. A shared pointer makes sure the destructor is called only once.
struct PipelineTaskToken
{
    PipelineTaskType type{};

    PipelineTaskToken()
    {
    }

    PipelineTaskToken(const PipelineTaskToken&) = delete;

    PipelineTaskToken(PipelineTaskToken&& other)
        : type(std::exchange(other.type, PipelineTaskType::Null))
    {
    }

    ~PipelineTaskToken()
    {
        if (type != PipelineTaskType::Null)
        {
            if ((--g_compilingPipelineTaskCount) == 0)
                g_compilingPipelineTaskCount.notify_one();
        }
    }
};

struct PipelineStateQueueItem
{
    XXH64_hash_t pipelineHash;
    PipelineState pipelineState;
    std::shared_ptr<PipelineTaskToken> token;
#ifdef ASYNC_PSO_DEBUG
    std::string pipelineName;
#endif
};

static moodycamel::BlockingConcurrentQueue<PipelineStateQueueItem> g_pipelineStateQueue;

static void CompilePipeline(XXH64_hash_t pipelineHash, const PipelineState& pipelineState
#ifdef ASYNC_PSO_DEBUG
    , const std::string& pipelineName
#endif
)
{
    auto pipeline = CreateGraphicsPipeline(pipelineState);
    if (pipeline == nullptr)
        return;
#ifdef ASYNC_PSO_DEBUG
    pipeline->setName(pipelineName);
#endif

    // Will get dropped in render thread if a different thread already managed to compile this.
    RenderCommand cmd;
    cmd.type = RenderCommandType::AddPipeline;
    cmd.addPipeline.hash = pipelineHash;
    cmd.addPipeline.pipeline = pipeline.release();
    EnqueueRenderCommand(cmd);
}

static void PipelineCompilerThread()
{
    os::thread::SetCurrentThreadName("Pipeline Compiler Thread");

#ifdef _WIN32
    int threadPriority = THREAD_PRIORITY_LOWEST;
    SetThreadPriority(GetCurrentThread(), threadPriority);
#endif

    std::unique_ptr<GuestThreadContext> ctx;

    while (true)
    {
        PipelineStateQueueItem queueItem;
        g_pipelineStateQueue.wait_dequeue(queueItem);

        if (ctx == nullptr)
            ctx = std::make_unique<GuestThreadContext>(0);

#ifdef _WIN32
        int newThreadPriority = threadPriority;

        if (App::s_isLoading)
            newThreadPriority = THREAD_PRIORITY_HIGHEST;
        else
            newThreadPriority = THREAD_PRIORITY_LOWEST;

        if (newThreadPriority != threadPriority)
        {
            SetThreadPriority(GetCurrentThread(), newThreadPriority);
            threadPriority = newThreadPriority;
        }
#endif

        CompilePipeline(queueItem.pipelineHash, queueItem.pipelineState
#ifdef ASYNC_PSO_DEBUG
            , queueItem.pipelineName.c_str()
#endif
        );

        std::this_thread::yield();
    }
}

static std::vector<std::unique_ptr<std::thread>> g_pipelineCompilerThreads = []()
    {
        size_t threadCount = std::max(2u, (std::thread::hardware_concurrency() * 2) / 3);

        std::vector<std::unique_ptr<std::thread>> threads(threadCount);
        for (auto& thread : threads)
            thread = std::make_unique<std::thread>(PipelineCompilerThread);

        return threads;
    }();

static constexpr uint32_t MODEL_DATA_VFTABLE = 0x82073A44;
static constexpr uint32_t TERRAIN_MODEL_DATA_VFTABLE = 0x8211D25C;
static constexpr uint32_t PARTICLE_MATERIAL_VFTABLE = 0x8211F198;

// Allocate the shared pointer only when new compilations are happening.
// If nothing was compiled, the local "token" variable will get destructed with RAII instead.
struct PipelineTaskTokenPair
{
    PipelineTaskToken token;
    std::shared_ptr<PipelineTaskToken> sharedToken;
};

// Having this separate, because I don't want to lock a mutex in the render thread before
// every single draw. Might be worth profiling to see if it actually has an impact and merge them.
static xxHashMap<PipelineState> g_asyncPipelineStates;

static void EnqueueGraphicsPipelineCompilation(
    const PipelineState& pipelineState,
    PipelineTaskTokenPair& tokenPair,
    const char* name,
    bool isPrecompiledPipeline = false)
{
    XXH64_hash_t hash = GetPipelineStateHash(pipelineState);
    bool shouldCompile = g_asyncPipelineStates.emplace(hash, pipelineState).second;

    if (shouldCompile)
    {
        if (!App::s_isLoading && isPrecompiledPipeline)
        {
            // We can just compile here during the logos.
            CompilePipeline(hash, pipelineState
#ifdef ASYNC_PSO_DEBUG
                , fmt::format("CACHE {} {:X}", name, hash)
#endif
            );
        }
        else
        {
            if (tokenPair.sharedToken == nullptr && tokenPair.token.type != PipelineTaskType::Null)
                tokenPair.sharedToken = std::make_shared<PipelineTaskToken>(std::move(tokenPair.token));

            PipelineStateQueueItem queueItem;
            queueItem.pipelineHash = hash;
            queueItem.pipelineState = pipelineState;
            queueItem.token = tokenPair.sharedToken;
#ifdef ASYNC_PSO_DEBUG
            queueItem.pipelineName = fmt::format("ASYNC {} {:X}", name, hash);
#endif
            g_pipelineStateQueue.enqueue(queueItem);
        }
    }

#ifdef PSO_CACHING_CLEANUP
    if (shouldCompile && isPrecompiledPipeline)
    {
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.emplace(hash, pipelineState);
    }
#endif

#ifdef PSO_CACHING
    if (!isPrecompiledPipeline)
    {
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.erase(hash);
    }
#endif
}

template<typename T>
static bool CheckMadeAll(const T& modelData)
{
    if (!modelData.IsMadeOne())
        return false;

    for (auto& meshGroup : modelData.m_NodeGroupModels)
    {
        for (auto& mesh : meshGroup->m_OpaqueMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }     

        for (auto& mesh : meshGroup->m_TransparentMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }    

        for (auto& mesh : meshGroup->m_PunchThroughMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }

        for (auto& specialMeshGroup : meshGroup->m_SpecialMeshGroups)
        {
            for (auto& mesh : specialMeshGroup)
            {
                if (!CheckMadeAll(mesh.get()))
                    return false;
            }
        }
    }

    for (auto& mesh : modelData.m_OpaqueMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    for (auto& mesh : modelData.m_TransparentMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    for (auto& mesh : modelData.m_PunchThroughMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    return true;
}

static void PipelineTaskConsumerThread()
{
    os::thread::SetCurrentThreadName("Pipeline Task Consumer Thread");

#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
#endif

    std::vector<PipelineTask> localPipelineTaskQueue;
    std::unique_ptr<GuestThreadContext> ctx;

    while (true)
    {
        // Wait for tasks to arrive.
        uint32_t pendingPipelineTaskCount;
        while ((pendingPipelineTaskCount = g_pendingPipelineTaskCount.load()) == 0)
            g_pendingPipelineTaskCount.wait(pendingPipelineTaskCount);

        if (ctx == nullptr)
            ctx = std::make_unique<GuestThreadContext>(0);

        {
            std::lock_guard lock(g_pipelineTaskMutex);
            localPipelineTaskQueue.insert(localPipelineTaskQueue.end(), g_pipelineTaskQueue.begin(), g_pipelineTaskQueue.end());
            g_pipelineTaskQueue.clear();
        }

        bool allHandled = true;

        for (auto& [type] : localPipelineTaskQueue)
        {
            switch (type)
            {
            case PipelineTaskType::PrecompilePipelines:
            {
                // Deliberately leaving the type null to account for the enqueue
                // call not incrementing the compiling pipeline task counter.
                PipelineTaskTokenPair tokenPair;

                for (auto vertexElements : g_vertexDeclarationCache)
                    CreateVertexDeclarationWithoutAddRef(reinterpret_cast<GuestVertexElement*>(vertexElements));

                for (auto pipelineState : g_pipelineStateCache)
                {
                    // The hashes were reinterpret casted to pointers in the cache.
                    pipelineState.vertexShader = GetOrCreateShader(reinterpret_cast<XXH64_hash_t>(pipelineState.vertexShader), ResourceType::VertexShader);

                    if (pipelineState.pixelShader != nullptr)
                        pipelineState.pixelShader = GetOrCreateShader(reinterpret_cast<XXH64_hash_t>(pipelineState.pixelShader), ResourceType::PixelShader);

                    {
                        std::lock_guard lock(g_vertexDeclarationMutex);
                        pipelineState.vertexDeclaration = g_vertexDeclarations[reinterpret_cast<XXH64_hash_t>(pipelineState.vertexDeclaration)];
                    }

                    // Zero out depth bias for Vulkan, we only store common values for D3D12.
                    if (g_capabilities.dynamicDepthBias && g_backend != Backend::D3D12)
                    {
                        pipelineState.depthBias = 0;
                        pipelineState.slopeScaledDepthBias = 0.0f;
                    }

                    auto createGraphicsPipeline = [&](PipelineState& pipelineStateToCreate, const char* name)
                        {
                            SanitizePipelineState(pipelineStateToCreate);
                            EnqueueGraphicsPipelineCompilation(pipelineStateToCreate, tokenPair, name, true);
                        };

                    // Compile both MSAA and non MSAA variants to work with reflection maps. The render formats are an assumption but it should hold true.
                    if (IsMsaaInUse() &&
                        pipelineState.renderTargetFormat == RenderFormat::R16G16B16A16_FLOAT &&
                        RenderFormatIsStencil(pipelineState.depthStencilFormat))
                    {
                        auto msaaPipelineState = pipelineState;
                        msaaPipelineState.sampleCount = int32_t(Config::AntiAliasing.Value);

                        if (Config::TransparencyAntiAliasing && (msaaPipelineState.specConstants & SPEC_CONSTANT_ALPHA_TEST) != 0)
                        {
                            msaaPipelineState.enableAlphaToCoverage = true;
                            msaaPipelineState.specConstants &= ~SPEC_CONSTANT_ALPHA_TEST;
                            msaaPipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                        }

                        createGraphicsPipeline(msaaPipelineState, "Precompiled Pipeline MSAA");
                    }

                    createGraphicsPipeline(pipelineState, "Precompiled Pipeline");
                }

                type = PipelineTaskType::Null;
                --g_pendingPipelineTaskCount;

                break;
            }

            case PipelineTaskType::RecompilePipelines:
            {
                PipelineTaskTokenPair tokenPair;
                tokenPair.token.type = type;

                auto asyncPipelines = g_asyncPipelineStates.values();

                for (auto& [hash, pipelineState] : asyncPipelines)
                {
                    bool alphaTest = (pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0;
                    bool msaa = pipelineState.sampleCount != 1 || (pipelineState.renderTargetFormat == RenderFormat::R16G16B16A16_FLOAT && RenderFormatIsStencil(pipelineState.depthStencilFormat));

                    pipelineState.sampleCount = 1;
                    pipelineState.enableAlphaToCoverage = false;
                    pipelineState.specConstants &= ~(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE);

                    if (msaa && IsMsaaInUse())
                    {
                        pipelineState.sampleCount = int32_t(Config::AntiAliasing.Value);

                        if (alphaTest)
                        {
                            if (Config::TransparencyAntiAliasing)
                            {
                                pipelineState.enableAlphaToCoverage = true;
                                pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                            }
                            else
                            {
                                pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                            }
                        }
                    }
                    else if (alphaTest)
                    {
                        pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                    }

                    SanitizePipelineState(pipelineState);
                    EnqueueGraphicsPipelineCompilation(pipelineState, tokenPair, "Recompiled Pipeline State");
                }

                type = PipelineTaskType::Null;
                --g_pendingPipelineTaskCount;

                break;
            }
            }
        }

        if (allHandled)
            localPipelineTaskQueue.clear();

        std::this_thread::yield();
    }
}

static std::thread g_pipelineTaskConsumerThread(PipelineTaskConsumerThread);

#ifdef PSO_CACHING
class SDLEventListenerForPSOCaching : public SDLEventListener
{
public:
    bool OnSDLEvent(SDL_Event* event) override 
    {
        if (event->type != SDL_EVENT_QUIT)
            return false;

        std::lock_guard lock(g_pipelineCacheMutex);
        if (g_pipelineStatesToCache.empty())
            return false;

        std::string path = (GetUserPath() / "pso_caching.txt").string();
        FILE* f = fopen(path.c_str(), "ab");
        if (f != nullptr)
        {
            ankerl::unordered_dense::set<GuestVertexDeclaration*> vertexDeclarations;
            xxHashMap<PipelineState> pipelineStatesToCache;

            for (auto& [hash, pipelineState] : g_pipelineStatesToCache)
            {
                if (pipelineState.vertexShader->shaderCacheEntry == nullptr ||
                    (pipelineState.pixelShader != nullptr && pipelineState.pixelShader->shaderCacheEntry == nullptr))
                {
                    continue;
                }

                vertexDeclarations.emplace(pipelineState.vertexDeclaration);

                // Mask out the config options.
                pipelineState.sampleCount = 1;
                pipelineState.enableAlphaToCoverage = false;

                if ((pipelineState.specConstants & SPEC_CONSTANT_ALPHA_TO_COVERAGE) != 0)
                {
                    pipelineState.specConstants &= ~SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                    pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                }

                pipelineStatesToCache.emplace(XXH3_64bits(&pipelineState, sizeof(pipelineState)), pipelineState);
            }

            for (auto vertexDeclaration : vertexDeclarations)
            {
                fmt::print(f, "static uint8_t g_vertexElements_{:016X}[] = {{", vertexDeclaration->hash);

                auto bytes = reinterpret_cast<uint8_t*>(vertexDeclaration->vertexElements.get());
                for (size_t i = 0; i < vertexDeclaration->vertexElementCount * sizeof(GuestVertexElement); i++)
                    fmt::print(f, "0x{:X},", bytes[i]);

                fmt::println(f, "}};");
            }

            for (auto& [pipelineHash, pipelineState] : pipelineStatesToCache)
            {
                fmt::println(f, "{{ "
                    "reinterpret_cast<GuestShader*>(0x{:X}),"
                    "reinterpret_cast<GuestShader*>(0x{:X}),"
                    "reinterpret_cast<GuestVertexDeclaration*>(0x{:X}),"
                    "{},"
                    "{},"
                    "{},"
                    "{},"
                    "RenderBlend::{},"
                    "RenderBlend::{},"
                    "RenderCullMode::{},"
                    "RenderFrontFace::{},"
                    "RenderComparisonFunction::{},"
                    "RenderComparisonFunction::{},"
                    "RenderStencilOp::{},"
                    "RenderStencilOp::{},"
                    "RenderStencilOp::{},"
                    "RenderComparisonFunction::{},"
                    "RenderStencilOp::{},"
                    "RenderStencilOp::{},"
                    "RenderStencilOp::{},"
                    "{},"
                    "{},"
                    "{},"
                    "{},"
                    "RenderBlendOperation::{},"
                    "{},"
                    "{},"
                    "RenderBlend::{},"
                    "RenderBlend::{},"
                    "RenderBlendOperation::{},"
                    "0x{:X},"
                    "RenderPrimitiveTopology::{},"
                    "{{ {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{} }},"
                    "RenderFormat::{},"
                    "RenderFormat::{},"
                    "{},"
                    "{},"
                    "{},"
                    "0x{:X} }},",
                    pipelineState.vertexShader->shaderCacheEntry->hash,
                    pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->shaderCacheEntry->hash : 0,
                    pipelineState.vertexDeclaration->hash,
                    pipelineState.zEnable,
                    pipelineState.zWriteEnable,
                    pipelineState.stencilEnable,
                    pipelineState.stencilTwoSided,
                    magic_enum::enum_name(pipelineState.srcBlend),
                    magic_enum::enum_name(pipelineState.destBlend),
                    magic_enum::enum_name(pipelineState.cullMode),
                    magic_enum::enum_name(pipelineState.frontFace),
                    magic_enum::enum_name(pipelineState.zFunc),
                    magic_enum::enum_name(pipelineState.stencilFunc),
                    magic_enum::enum_name(pipelineState.stencilFail),
                    magic_enum::enum_name(pipelineState.stencilZFail),
                    magic_enum::enum_name(pipelineState.stencilPass),
                    magic_enum::enum_name(pipelineState.stencilFuncCCW),
                    magic_enum::enum_name(pipelineState.stencilFailCCW),
                    magic_enum::enum_name(pipelineState.stencilZFailCCW),
                    magic_enum::enum_name(pipelineState.stencilPassCCW),
                    pipelineState.stencilMask,
                    pipelineState.stencilWriteMask,
                    pipelineState.stencilRef,
                    pipelineState.alphaBlendEnable,
                    magic_enum::enum_name(pipelineState.blendOp),
                    pipelineState.slopeScaledDepthBias,
                    pipelineState.depthBias,
                    magic_enum::enum_name(pipelineState.srcBlendAlpha),
                    magic_enum::enum_name(pipelineState.destBlendAlpha),
                    magic_enum::enum_name(pipelineState.blendOpAlpha),
                    pipelineState.colorWriteEnable,
                    magic_enum::enum_name(pipelineState.primitiveTopology),
                    pipelineState.vertexStrides[0],
                    pipelineState.vertexStrides[1],
                    pipelineState.vertexStrides[2],
                    pipelineState.vertexStrides[3],
                    pipelineState.vertexStrides[4],
                    pipelineState.vertexStrides[5],
                    pipelineState.vertexStrides[6],
                    pipelineState.vertexStrides[7],
                    pipelineState.vertexStrides[8],
                    pipelineState.vertexStrides[9],
                    pipelineState.vertexStrides[10],
                    pipelineState.vertexStrides[11],
                    pipelineState.vertexStrides[12],
                    pipelineState.vertexStrides[13],
                    pipelineState.vertexStrides[14],
                    pipelineState.vertexStrides[15],
                    magic_enum::enum_name(pipelineState.renderTargetFormat),
                    magic_enum::enum_name(pipelineState.depthStencilFormat),
                    pipelineState.sampleCount,
                    pipelineState.enableAlphaToCoverage,
                    pipelineState.enableConditionalSurvey,
                    pipelineState.specConstants);
            }

            fclose(f);
        }

        return false;
    }
};
SDLEventListenerForPSOCaching g_sdlEventListenerForPSOCaching;
#endif

void VideoConfigValueChangedCallback(IConfigDef* config)
{
    // Config options that require internal resolution resize
    Video::s_needsResize |=
        config == &Config::AspectRatio ||
        config == &Config::ResolutionScale ||
        config == &Config::AntiAliasing ||
        config == &Config::ShadowResolution ||
        config == &Config::ReflectionResolution;

    if (Video::s_needsResize)
        Video::ComputeViewportDimensions();

    // Config options that require pipeline recompilation
    bool shouldRecompile =
        config == &Config::AntiAliasing ||
        config == &Config::TransparencyAntiAliasing ||
        config == &Config::RadialBlur;

    if (shouldRecompile)
        EnqueuePipelineTask(PipelineTaskType::RecompilePipelines);
}

GUEST_FUNCTION_HOOK(sub_8253EC98, CreateDevice);

GUEST_FUNCTION_HOOK(sub_8253AE98, DestructResource);

GUEST_FUNCTION_HOOK(sub_8253A740, LockTextureRect);
GUEST_FUNCTION_HOOK(sub_82538D30, UnlockTextureRect);

GUEST_FUNCTION_HOOK(sub_8253B5D0, LockVertexBuffer);
GUEST_FUNCTION_HOOK(sub_8253B630, UnlockVertexBuffer);
// GUEST_FUNCTION_HOOK(sub_82BE61D0, GetVertexBufferDesc);

GUEST_FUNCTION_HOOK(sub_8253B6F0, LockIndexBuffer);
GUEST_FUNCTION_HOOK(sub_8253B750, UnlockIndexBuffer);
// GUEST_FUNCTION_HOOK(sub_82BE6200, GetIndexBufferDesc);

GUEST_FUNCTION_HOOK(sub_8253AB20, GetSurfaceDesc);

GUEST_FUNCTION_HOOK(sub_825471F8, GetVertexDeclaration);
// GUEST_FUNCTION_HOOK(sub_82BE0530, HashVertexDeclaration);

GUEST_FUNCTION_HOOK(sub_825586B0, Video::Present);
GUEST_FUNCTION_HOOK(sub_82543B58, GetBackBuffer);
GUEST_FUNCTION_HOOK(sub_82543BA0, GetDepthStencil);

GUEST_FUNCTION_HOOK(sub_8253A8D8, CreateTexture);
GUEST_FUNCTION_HOOK(sub_8253B508, CreateVertexBuffer);
GUEST_FUNCTION_HOOK(sub_8253B640, CreateIndexBuffer);
GUEST_FUNCTION_HOOK(sub_8253A9F8, CreateSurface);

PPC_FUNC(sub_825575B8)
{
    // The guest passes flags, the texture pointer and the destination slice in r4, r6 and r9.
    StretchRect(ctx.r4.u32, ArgTranslator::GetValue<GuestTexture*>(ctx, base, 3), ctx.r9.u32,
        static_cast<uint32_t>(ctx.lr));
}

GUEST_FUNCTION_HOOK(sub_82543EE0, SetRenderTarget);
GUEST_FUNCTION_HOOK(sub_825444F0, SetRenderTarget);
PPC_FUNC(sub_82544210)
{
    DepthCopyDestination destination{};
#ifdef __ANDROID__
    // The normal SoX framebuffer setup calls this at 0x828B3ECC. It keeps
    // the framebuffer in r30 and has already loaded the depth surface into r4.
    if (g_depthCopyTraceEnabled && ctx.lr == 0x828B3ED0 && ctx.r30.u32 != 0 && ctx.r4.u32 != 0)
    {
        constexpr uint32_t framebufferDepth = 0x18;
        constexpr uint32_t framebufferMode = 0x34;
        constexpr uint32_t surfaceHandle = 0x70;
        constexpr uint32_t surfaceResolveTexture = 0x74;
        constexpr uint32_t surfaceResolveLayer = 0x78;
        constexpr uint32_t textureHandle = 0x64;

        const uint32_t framebuffer = ctx.r30.u32;
        if (PPC_LOAD_U32(framebuffer + framebufferMode) == 0)
        {
            const uint32_t surface = PPC_LOAD_U32(framebuffer + framebufferDepth);
            if (surface != 0 && PPC_LOAD_U32(surface + surfaceHandle) == ctx.r4.u32)
            {
                const uint32_t texture = PPC_LOAD_U32(surface + surfaceResolveTexture);
                if (texture != 0)
                {
                    destination.textureAddress = PPC_LOAD_U32(texture + textureHandle);
                    destination.layer = PPC_LOAD_U32(surface + surfaceResolveLayer);
                }
            }
        }
    }
#endif

    SetDepthStencilSurface(ArgTranslator::GetValue<GuestDevice*>(ctx, base, 0),
        ArgTranslator::GetValue<GuestSurface*>(ctx, base, 1), destination);
}

GUEST_FUNCTION_HOOK(sub_82555B30, Clear);

GUEST_FUNCTION_HOOK(sub_825436F0, SetViewport);

GUEST_FUNCTION_HOOK(sub_8253AC40, SetTexture);
GUEST_FUNCTION_HOOK(sub_82543628, SetScissorRect);

GUEST_FUNCTION_HOOK(sub_826FEC28, DrawPrimitive);
GUEST_FUNCTION_HOOK(sub_826FF030, DrawIndexedPrimitive);
GUEST_FUNCTION_HOOK(sub_826FE5C0, DrawPrimitiveUP);

GUEST_FUNCTION_HOOK(sub_82547118, CreateVertexDeclaration);
GUEST_FUNCTION_HOOK(sub_825470F8, SetVertexDeclaration);

GUEST_FUNCTION_HOOK(sub_82548700, CreateVertexShader);
GUEST_FUNCTION_HOOK(sub_82546EE0, SetVertexShader);

GUEST_FUNCTION_HOOK(sub_82543918, SetStreamSource);
GUEST_FUNCTION_HOOK(sub_82543AC8, SetIndices);

GUEST_FUNCTION_HOOK(sub_82548608, CreatePixelShader);
GUEST_FUNCTION_HOOK(sub_82546BD8, SetPixelShader);

GUEST_FUNCTION_HOOK(sub_82636BF8, BeginConditionalSurvey);
GUEST_FUNCTION_HOOK(sub_82636C08, EndConditionalSurvey);
GUEST_FUNCTION_HOOK(sub_82636C10, BeginConditionalRendering);
GUEST_FUNCTION_HOOK(sub_82636C18, EndConditionalRendering);

GUEST_FUNCTION_HOOK(sub_8253B760, IsSet);

GUEST_FUNCTION_HOOK(sub_82543CF0, SetClipPlane);

GUEST_FUNCTION_HOOK(sub_82541A78, SetRenderState<D3DRS_ZENABLE>);
GUEST_FUNCTION_HOOK(sub_82541AC0, SetRenderState<D3DRS_ZWRITEENABLE>);
GUEST_FUNCTION_HOOK(sub_82541460, SetRenderState<D3DRS_ALPHATESTENABLE>);
GUEST_FUNCTION_HOOK(sub_825415C0, SetRenderState<D3DRS_SRCBLEND>);
GUEST_FUNCTION_HOOK(sub_82541650, SetRenderState<D3DRS_DESTBLEND>);
GUEST_FUNCTION_HOOK(sub_82541400, SetRenderState<D3DRS_CULLMODE>);
GUEST_FUNCTION_HOOK(sub_82541AF0, SetRenderState<D3DRS_ZFUNC>);
GUEST_FUNCTION_HOOK(sub_825418C8, SetRenderState<D3DRS_ALPHAREF>);
GUEST_FUNCTION_HOOK(sub_825414A0, SetRenderState<D3DRS_ALPHABLENDENABLE>);
GUEST_FUNCTION_HOOK(sub_82541530, SetRenderState<D3DRS_BLENDOP>);
GUEST_FUNCTION_HOOK(sub_82543ED0, SetRenderState<D3DRS_SCISSORTESTENABLE>);
GUEST_FUNCTION_HOOK(sub_82541E90, SetRenderState<D3DRS_SLOPESCALEDEPTHBIAS>);
GUEST_FUNCTION_HOOK(sub_82541F58, SetRenderState<D3DRS_DEPTHBIAS>);
GUEST_FUNCTION_HOOK(sub_82541750, SetRenderState<D3DRS_SRCBLENDALPHA>);
GUEST_FUNCTION_HOOK(sub_825417C0, SetRenderState<D3DRS_DESTBLENDALPHA>);
GUEST_FUNCTION_HOOK(sub_825416E0, SetRenderState<D3DRS_BLENDOPALPHA>);
GUEST_FUNCTION_HOOK(sub_82542050, SetRenderState<D3DRS_COLORWRITEENABLE>);
GUEST_FUNCTION_HOOK(sub_82541B30, SetRenderState<D3DRS_STENCILENABLE>);
GUEST_FUNCTION_HOOK(sub_82541B78, SetRenderState<D3DRS_TWOSIDEDSTENCILMODE>);
GUEST_FUNCTION_HOOK(sub_82541BE8, SetRenderState<D3DRS_STENCILFAIL>);
GUEST_FUNCTION_HOOK(sub_82541C28, SetRenderState<D3DRS_STENCILZFAIL>);
GUEST_FUNCTION_HOOK(sub_82541C68, SetRenderState<D3DRS_STENCILPASS>);
GUEST_FUNCTION_HOOK(sub_82541BB8, SetRenderState<D3DRS_STENCILFUNC>);
GUEST_FUNCTION_HOOK(sub_82541D78, SetRenderState<D3DRS_STENCILREF>);
GUEST_FUNCTION_HOOK(sub_82541D98, SetRenderState<D3DRS_STENCILMASK>);
GUEST_FUNCTION_HOOK(sub_82541DB8, SetRenderState<D3DRS_STENCILWRITEMASK>);
GUEST_FUNCTION_HOOK(sub_82541CC8, SetRenderState<D3DRS_CCW_STENCILFAIL>);
GUEST_FUNCTION_HOOK(sub_82541D08, SetRenderState<D3DRS_CCW_STENCILZFAIL>);
GUEST_FUNCTION_HOOK(sub_82541D48, SetRenderState<D3DRS_CCW_STENCILPASS>);
GUEST_FUNCTION_HOOK(sub_82541C98, SetRenderState<D3DRS_CCW_STENCILFUNC>);
GUEST_FUNCTION_HOOK(sub_82541E38, SetRenderState<D3DRS_CLIPPLANEENABLE>);

int GetType(GuestResource* resource)
{
    if (resource->type == ResourceType::Texture) return 3;
    if (resource->type == ResourceType::VolumeTexture) return 17;
    if (resource->type == ResourceType::ArrayTexture) return 19;

    LOGF_WARNING("unknown resource type {:d}!", (int32_t)resource->type);
    __builtin_trap();
    return 0;
}

GUEST_FUNCTION_HOOK(sub_8253AE08, GetType);

// Game asks about the size of surface to check if it needs to be tiled.
// Because EDRAM has only 10MB, if size is more than 1024, then it enables tiling.
// We return 0 to always disable tiling.
int SurfaceSize(uint32_t width, uint32_t height, uint32_t format, uint32_t multisampleLevel)
{
    return 0;
}

GUEST_FUNCTION_HOOK(sub_82538D60, SurfaceSize);
GUEST_FUNCTION_HOOK(sub_82656B68, MakePictureData);
GUEST_FUNCTION_HOOK(sub_82656DB8, MakePictureData);

// GUEST_FUNCTION_HOOK(sub_82E9EE38, SetResolution);

GUEST_FUNCTION_HOOK(sub_82736178, ScreenShaderInit);

GUEST_FUNCTION_STUB(sub_8253EB38);
GUEST_FUNCTION_STUB(sub_8253EB78);
GUEST_FUNCTION_STUB(sub_82543BE0); // SetGammaRamp
GUEST_FUNCTION_STUB(sub_82543C68); // SetGammaRamp
GUEST_FUNCTION_STUB(sub_82547278); // Set shader allocation
GUEST_FUNCTION_STUB(sub_8272FAD0);
GUEST_FUNCTION_STUB(sub_82558E00);
GUEST_FUNCTION_STUB(sub_82559928);
GUEST_FUNCTION_STUB(sub_82559C18);
GUEST_FUNCTION_STUB(sub_82700C18); // D3DXFilterTexture
GUEST_FUNCTION_STUB(sub_8253EAE0);
GUEST_FUNCTION_STUB(sub_8254D598); // BeginConditional
GUEST_FUNCTION_STUB(sub_8254D7B0); // BeginConditional
GUEST_FUNCTION_STUB(sub_8254D9D0); // BeginConditional
GUEST_FUNCTION_STUB(sub_8254DB90); // BeginConditional
GUEST_FUNCTION_STUB(sub_8254DD40); // SetScreenExtentQueryMode

struct Rect
{
    be<uint32_t> x1;
    be<uint32_t> y1;
    be<uint32_t> x2;
    be<uint32_t> y2;
};

struct RESOLVE_PARAMS
{
    be<uint32_t> format;
    be<uint32_t> unk;
    be<uint32_t> format2;
};

int D3DDevice_BeginTiling(GuestDevice* device, uint32_t flags, uint32_t count, Rect* pTileRects, be<float>* pClearColor, float clearZ, uint32_t clearStencil)
{
    Clear(device, 0x3F, 0, pClearColor, clearZ, clearStencil);

    return 0;
}

GUEST_FUNCTION_HOOK(sub_82558F88, D3DDevice_BeginTiling);

int D3DDevice_EndTiling(GuestDevice* device, uint32_t flags, Rect* pResolveRects, GuestTexture* pDestTexture, be<float>* pClearColor, float clearZ, uint32_t clearStencil, RESOLVE_PARAMS* resolveParams)
{
    if (pDestTexture)
    {
        StretchRect(flags, pDestTexture, 0);
    }

    return 0;
}

GUEST_FUNCTION_HOOK(sub_82559480, D3DDevice_EndTiling);

int D3DDevice_BeginShaderConstantF4(GuestDevice* device, uint32_t isPixelShader, uint32_t startRegister, be<uint32_t>* cachedConstantData, be<uint32_t>* writeCombinedConstantData, uint32_t vectorCount)
{
    uint32_t* constants;
    be<uint64_t>* dirtyFlags;

    if (isPixelShader)
    {
        constants = &device->pixelShaderFloatConstants[startRegister * 4];
        dirtyFlags = &device->dirtyFlags[1];
    }
    else
    {
        constants = &device->vertexShaderFloatConstants[startRegister * 4];
        dirtyFlags = &device->dirtyFlags[0];
    }

    const uint32_t addr = g_memory.MapVirtual(constants);
    *cachedConstantData = addr;
    *writeCombinedConstantData = addr;

    const uint32_t startBit = startRegister >> 2;
    const uint32_t endBit = (startRegister + vectorCount - 1) >> 2;
    const uint64_t dirtyFlag = ~0ull << startBit >> startBit >> (63 - endBit) << (63 - endBit);
    *dirtyFlags = dirtyFlags->get() | dirtyFlag;

    return 0;
}

GUEST_FUNCTION_HOOK(sub_825466E8, D3DDevice_BeginShaderConstantF4);
