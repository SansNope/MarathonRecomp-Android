#include <api/Marathon.h>
#include <gpu/video.h>
#include <kernel/memory.h>
#include <os/logger.h>
#include <patches/aspect_ratio_patches.h>
#include <user/config.h>
#include <cstring>
#include <string_view>

const char* g_pBlockName{};

extern const char* g_surfaceCreationName;

void SetMSAALevel(PPCRegister& val)
{
    val.u32 = 0;
}

void BeginBlockGetName(PPCRegister& r3)
{
    g_pBlockName = (const char*)g_memory.Translate(r3.u32);

#if _DEBUG
    if (g_pBlockName)
        LOGFN_UTILITY("Block Begin: {}", g_pBlockName);
#endif
}

// Sonicteam::RenderAction::ReadBlock
PPC_FUNC_IMPL(__imp__sub_8260DF88);
PPC_FUNC(sub_8260DF88)
{
    // Resize must reload the script even when it has the same path.
    if (!Video::IsResizePending() && ctx.r4.u32 != 0)
    {
        const auto pFilePath = reinterpret_cast<stdx::string*>(base + ctx.r4.u32);

        // Don't process render script if it's the same one currently in use.
        if (Sonicteam::Globals::ms_pCurrentRenderScript && *pFilePath == *Sonicteam::Globals::ms_pCurrentRenderScript)
            return;
    }

    __imp__sub_8260DF88(ctx, base);
}

// EndBlock
PPC_FUNC_IMPL(__imp__sub_826078D8);
PPC_FUNC(sub_826078D8)
{
#if _DEBUG
    if (g_pBlockName)
        LOGFN_UTILITY("Block End: {}", g_pBlockName);
#endif

    g_pBlockName = nullptr;

    __imp__sub_826078D8(ctx, base);
}

float ReflectionScaleFactor(EReflectionResolution ref) {
    switch (ref) {
        case EReflectionResolution::Off:
        case EReflectionResolution::Eighth:
            return 0.5f;
        case EReflectionResolution::Quarter:
            return 1.0f;
        case EReflectionResolution::Half:
            return 2.0f;
        case EReflectionResolution::Full:
            return 4.0f;
        default:
            return 1.0f;
    }
}

#ifdef __ANDROID__
// Lua 5.0's binary chunk loader returns a Proto before the root chunk executes.
PPC_FUNC_IMPL(__imp__sub_825E3D80);
PPC_FUNC(sub_825E3D80)
{
    __imp__sub_825E3D80(ctx, base);

    const uint32_t prototype = ctx.r3.u32;
    if (!prototype || Config::GetEffectiveReflectionResolution() != EReflectionResolution::Off)
        return;

    // Match the stock render_reflection.lub root. Its first two instructions
    // set EnableReflection = true, followed by the two function definitions.
    constexpr uint32_t loadTrueIntoR0 = 0x8002;
    constexpr uint32_t loadFalseIntoR0 = 0x0002;
    constexpr uint32_t setFirstGlobalFromR0 = 0x0007;
    constexpr uint32_t luaValueSize = 16;
    if (PPC_LOAD_U32(prototype + 40) != 3 || PPC_LOAD_U32(prototype + 44) != 7)
        return;

    const uint32_t constants = PPC_LOAD_U32(prototype + 8);
    const uint32_t code = PPC_LOAD_U32(prototype + 12);
    if (!constants || !code ||
        PPC_LOAD_U32(code) != loadTrueIntoR0 || PPC_LOAD_U32(code + 4) != setFirstGlobalFromR0)
        return;

    const auto hasStringConstant = [base](uint32_t value, std::string_view expected)
    {
        constexpr uint32_t luaStringType = 4;
        if (PPC_LOAD_U32(value) != luaStringType)
            return false;

        const uint32_t string = PPC_LOAD_U32(value + 8);
        return string && PPC_LOAD_U32(string + 12) == expected.size() &&
            std::memcmp(base + string + 16, expected.data(), expected.size()) == 0;
    };

    if (hasStringConstant(constants, "EnableReflection") &&
        hasStringConstant(constants + luaValueSize, "CreateReflectionTexture") &&
        hasStringConstant(constants + 2 * luaValueSize, "RenderReflection"))
    {
        // Use the same branch as cutscenes: it keeps the neutral texture and
        // its bindings while skipping the reflected geometry and sky.
        PPC_STORE_U32(code, loadFalseIntoR0);
        LOG("[graphics] Real-time reflections disabled in render_reflection.");
    }
}
#endif

// CreateTexture
PPC_FUNC_IMPL(__imp__sub_82619D00);
PPC_FUNC(sub_82619D00)
{
    auto pName = (stdx::string*)g_memory.Translate(ctx.r4.u32);

    if (*pName == "radermap")
    {
        ctx.r5.u32 = g_radarMapScale;
        ctx.r6.u32 = g_radarMapScale;
    }

    if (*pName == "reflection0")
    {
        const float scale = ReflectionScaleFactor(Config::GetEffectiveReflectionResolution());
        ctx.r5.u32 = static_cast<int>(static_cast<float>(ctx.r5.u32) * scale);
        ctx.r6.u32 = static_cast<int>(static_cast<float>(ctx.r6.u32) * scale);
    }

    // RenderMefiress
    if (*pName == "user0")
    {
        const auto resolution = static_cast<uint32_t>(Config::GetEffectiveShadowResolution());
        ctx.r5.u32 = resolution;
        ctx.r6.u32 = resolution;
    }

#if _DEBUG
    auto width = ctx.r5.u32;
    auto height = ctx.r6.u32;
#endif

    g_surfaceCreationName = pName->c_str();
    __imp__sub_82619D00(ctx, base);
    g_surfaceCreationName = nullptr;

#if _DEBUG
    LOGFN_UTILITY("Created texture: {} ({}x{})", pName->c_str(), width, height);
#endif
}

// CreateDepthStencilSurface
PPC_FUNC_IMPL(__imp__sub_82619B88);
PPC_FUNC(sub_82619B88)
{
    auto pName = (stdx::string*)g_memory.Translate(ctx.r4.u32);

    if (g_pBlockName)
    {
        if (strcmp(g_pBlockName, "radermap0") == 0)
        {
            ctx.r5.u32 = g_radarMapScale;
            ctx.r6.u32 = g_radarMapScale;
        }

        // RenderMefiress
        if (strcmp(g_pBlockName, "user0") == 0 && *pName == "depthstencil_256")
        {
            const auto resolution = static_cast<uint32_t>(Config::GetEffectiveShadowResolution());
            ctx.r5.u32 = resolution;
            ctx.r6.u32 = resolution;
        }
    }

    if (*pName == "depthstencil_1_4")
    {
        const float scale = ReflectionScaleFactor(Config::GetEffectiveReflectionResolution());
        ctx.r5.u32 = static_cast<int>(static_cast<float>(ctx.r5.u32) * scale);
        ctx.r6.u32 = static_cast<int>(static_cast<float>(ctx.r6.u32) * scale);
    }
    
#if _DEBUG
    auto width = ctx.r5.u32;
    auto height = ctx.r6.u32;
#endif

    g_surfaceCreationName = pName->c_str();
    __imp__sub_82619B88(ctx, base);
    g_surfaceCreationName = nullptr;

#if _DEBUG
    if (g_pBlockName)
    {
        LOGFN_UTILITY("Created texture for {}: {} ({}x{})", g_pBlockName, pName->c_str(), width, height);
    }
    else
    {
        LOGFN_UTILITY("Created texture: {} ({}x{})", pName->c_str(), width, height);
    }
#endif
}

float ShadowScaleFactor(EShadowResolution ref) {
    switch (ref) {
        case EShadowResolution::x512:
            return 0.5f;
        case EShadowResolution::x1024:
            return 1.0f;
        case EShadowResolution::x2048:
            return 2.0f;
        case EShadowResolution::x4096:
            return 4.0f;
        case EShadowResolution::x8192:
            return 8.0f;
        default:
            return 1.0f;
    }
}

// CreateArrayTexture
PPC_FUNC_IMPL(__imp__sub_82619FF0);
PPC_FUNC(sub_82619FF0)
{
    auto pName = (stdx::string*)g_memory.Translate(ctx.r4.u32);

    if (*pName == "csm")
    {
        const float scale = ShadowScaleFactor(Config::GetEffectiveShadowResolution());
        ctx.r5.u32 = static_cast<int>(static_cast<float>(ctx.r5.u32) * scale);
        ctx.r6.u32 = static_cast<int>(static_cast<float>(ctx.r6.u32) * scale);
    }

    g_surfaceCreationName = pName->c_str();
    __imp__sub_82619FF0(ctx, base);
    g_surfaceCreationName = nullptr;
}

std::string g_renderWorldFBO;

void GetRenderWorldFBO(PPCRegister& name)
{
    auto pName = xpointer(reinterpret_cast<char*>(name.u32));
    g_renderWorldFBO = std::string(pName.get());
}

void FurtherObjectShadows(PPCRegister& scope)
{
    if (g_renderWorldFBO != "shadowmap" || Config::IsLowGraphicsEnabled())
        return;

    scope.u32 = 1;
}

bool DisableRadialBlur()
{
    return Config::GetEffectiveRadialBlur() == ERadialBlur::Off;
}

bool DisableKingdomValleyMist()
{
    return Config::DisableKingdomValleyMist;
}

static thread_local bool g_isLoadingEnvMap = false;

void SceneEnvMapLoadTexture()
{
    if (Config::FixEnvironmentMaps)
        g_isLoadingEnvMap = true;
}

void ResourceMgrKeyOverride(PPCRegister& r1, PPCRegister& r28)
{
    if (!g_isLoadingEnvMap)
        return;

    g_isLoadingEnvMap = false;

    // std::string::assign(const std::string&, uint, uint)
    GuestToHostFunction<void>(0x82161D10, r1.u32 + 0x60, r28.u32, 0, 0xFFFFFFFF);
}
