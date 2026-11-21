#include "input_patches.h"
#include <api/Marathon.h>
#include <cpu/guest_stack_var.h>
#include <kernel/function.h>
#include <os/logger.h>
#include <user/config.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// P-06 / Sonic Adventure-style Spindash for Sonic.
//
// When Config::ImprovedSpindash is enabled:
// - On the ground (Wait, Walk, Run), pressing X immediately starts charging Spindash (State 70)
//   even while moving, instead of requiring Sonic to stand still and hold X for c_spindash_time.
//   (Light Dash retains priority when near rings and LightDash == X.)
// - A B tap starts Spin Kick on release. A B hold starts Sliding on release.
// - X while rolling uncurls without dropping speed. A new ground press starts charging again,
//   and the next launch can add a capped boost to the speed carried into the charge.
// - Rolling retains slope physics, stick steering, wall and low-speed exits, and momentum
//   through Spindash Jump (A) and ledge fall-off.

namespace
{
    constexpr uint32_t kPostureControlVftable = 0x82009048u;
    constexpr uint32_t kPostureWorldInputQuatOffset = 0x2E0u;
    constexpr uint32_t kPostureWorldInputMagOffset = 0x2F0u;

    // Logical button bits from ListenerNormal (forwarded to ICommonContext::m_Buttons).
    constexpr uint32_t kBtnAPressed = 0x00000001u;
    constexpr uint32_t kBtnXPressed = 0x00000100u; // Stock bit includes a timed-action release.
    constexpr uint32_t kBtnXHeld = 0x00000200u;
    constexpr uint32_t kBtnXReleased = 0x00000400u;
    constexpr uint32_t kBtnYPressed = 0x00008000u;

    // External flag bit checked by SonicWait/SonicWalk/SonicRun for Light Dash ring trail proximity.
    constexpr uint64_t kExternalFlagLightDashReady = 0x4000ull;
    constexpr uint32_t kPostureFlagWall = 0x8u;

    // Sonic state IDs in Sonicteam::Player::State::Machine2.
    constexpr uint32_t kStateFall = 3u;
    constexpr uint32_t kStateJump = 4u;
    constexpr uint32_t kStateSonicSliding = 68u;
    constexpr uint32_t kStateSonicSpinkick = 69u;
    constexpr uint32_t kStateSonicSpindash = 70u;

    // Rolling animation (verified from sub_82217AF0).
    constexpr uint32_t kAnimSpindashRoll = 82u;

    // CommonMiscFlagsSlot indices (offset relative to CommonContext = 0xD8 + slot).
    constexpr size_t kMiscSlotAttackHitbox = 0xE5u - 0xD8u; // +229 (0xE5)
    constexpr size_t kMiscSlotSpindashFlag = 0xF0u - 0xD8u; // +240 (0xF0)

    // Tuning constants (Sonic 2006 units: cm/s and cm/s^2; Sonic max run speed ~1700 cm/s).
    constexpr float kMaxChargeTime = 0.75f;          // Seconds to reach full Spindash charge.
    constexpr float kMinLaunchSpeed = 2400.0f;       // Quick tap / uncharged launch speed.
    constexpr float kMaxLaunchSpeed = 4000.0f;       // Fully charged launch speed.
    constexpr float kTapMomentumBoost = 220.0f;      // Bonus on a quick recharge with rolling momentum.
    constexpr float kFullMomentumBoost = 440.0f;     // Bonus with a full recharge.
    constexpr float kMaxMomentumLaunchSpeed = 4400.0f;
    constexpr float kMaxRollSpeed = 5200.0f;         // Maximum speed achievable downhill.
    constexpr float kMinRollExitSpeed = 550.0f;      // Uncurl back to normal ground state below this.
    constexpr float kChargeEntryDecel = 3600.0f;     // Smoothly brakes entry momentum while charging.
    constexpr float kChargeAimTurnRate = 14.0f;      // rad/s aiming turn speed while charging.
    constexpr float kRollTurnRateSlow = 3.6f;        // rad/s steering turn speed at low roll speed.
    constexpr float kRollTurnRateFast = 2.2f;        // rad/s steering turn speed at high roll speed.
    constexpr float kSlopeGravityAccel = 1800.0f;    // Downhill boost / uphill drag (cm/s^2).
    constexpr float kRollFrictionForward = 380.0f;   // Gentle drag while holding stick forward.
    constexpr float kRollFrictionNeutral = 700.0f;   // Drag when stick is neutral.
    constexpr float kRollFrictionBrake = 2200.0f;    // Braking drag when pulling stick opposite.
    constexpr float kMinStickMagnitude = 0.20f;
    constexpr float kUncurlDeceleration = 900.0f; // Ground speed above Sonic's normal running cap.

    struct Vec3
    {
        float x{};
        float y{};
        float z{};
    };

    struct Quat
    {
        float x{};
        float y{};
        float z{};
        float w{ 1.0f };
    };

    inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline Vec3 operator*(const Vec3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

    inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    inline Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline float LengthSq(const Vec3& v) { return Dot(v, v); }

    inline Vec3 Normalize(const Vec3& v, const Vec3& fallback = { 0.0f, 0.0f, 1.0f })
    {
        const float lenSq = LengthSq(v);
        if (lenSq <= 1e-8f)
            return fallback;

        const float invLen = 1.0f / std::sqrt(lenSq);
        return v * invLen;
    }

    inline Quat NormalizeQuat(const Quat& q)
    {
        const float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
        if (lenSq <= 1e-8f)
            return { 0.0f, 0.0f, 0.0f, 1.0f };

        const float invLen = 1.0f / std::sqrt(lenSq);
        return { q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen };
    }

    // Matches Xbox 360 XNAMath row-vector quaternion convention used by PostureControl.
    inline Vec3 QuatRotate(const Quat& q, const Vec3& v)
    {
        const Vec3 qv{ q.x, q.y, q.z };
        const Vec3 t = Cross(qv, v) + v * q.w;
        return v + Cross(qv, t) * 2.0f;
    }

    inline Quat QuatMul(const Quat& a, const Quat& b)
    {
        return {
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        };
    }

    inline Quat QuatAxisAngleY(float radians)
    {
        const float half = 0.5f * radians;
        return { 0.0f, std::sin(half), 0.0f, std::cos(half) };
    }

    inline Sonicteam::SoX::Math::Quaternion StoreQuat(const Quat& q)
    {
        Sonicteam::SoX::Math::Quaternion out{};
        out.X = q.x;
        out.Y = q.y;
        out.Z = q.z;
        out.W = q.w;
        return out;
    }

    inline bool IsValidGuestHeapPtr(uint32_t addr)
    {
        return addr >= 0x20000u && addr < 0x7FEA0000u;
    }

    inline uint32_t ReadGuestU32(const uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<uint32_t>*>(base + addr)->get();
    }

    inline void WriteGuestU32(uint8_t* base, uint32_t addr, uint32_t value)
    {
        *reinterpret_cast<be<uint32_t>*>(base + addr) = value;
    }

    inline float ReadGuestF32(const uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<float>*>(base + addr)->get();
    }

    inline Quat ReadGuestQuat(const uint8_t* base, uint32_t addr)
    {
        const auto* q = reinterpret_cast<const Sonicteam::SoX::Math::Quaternion*>(base + addr);
        return { q->X.get(), q->Y.get(), q->Z.get(), q->W.get() };
    }

    inline uint32_t GetUnlockedButtons(const Sonicteam::Player::State::SonicContext* pCtx)
    {
        if (!pCtx || pCtx->m_LockButtons.get() > 0.0f)
            return 0u;

        return pCtx->m_Buttons.get();
    }

    inline bool CanLightDashNow(const Sonicteam::Player::State::SonicContext* pCtx, uint32_t buttons)
    {
        const uint32_t lightDashButton = Config::LightDash == ELightDash::X
            ? InputListenerButtons::XPhysicalPressed : kBtnYPressed;
        return (buttons & lightDashButton) != 0u &&
            (pCtx->m_ExternalFlags.get() & kExternalFlagLightDashReady) != 0ull &&
            (GuestToHostFunction<uint32_t>(sub_82209058, pCtx, 0u) & 0xFFu) != 0u;
    }

    bool CallChangeState(uint8_t* base, uint32_t machineAddr, uint32_t stateId)
    {
        if (!IsValidGuestHeapPtr(machineAddr))
            return false;

        const uint32_t vftableAddr = ReadGuestU32(base, machineAddr);
        if (vftableAddr < 0x82000000u || vftableAddr >= 0x82100000u)
            return false;

        const uint32_t fnAddr = ReadGuestU32(base, vftableAddr + 12u);
        if (!g_memory.FindFunction(fnAddr))
            return false;

        return (GuestToHostFunction<uint32_t>(fnAddr, g_memory.Translate(machineAddr), stateId) & 0xFFu) != 0u;
    }

    bool CallSpindashExit(uint32_t stateAddr)
    {
        return (GuestToHostFunction<uint32_t>(sub_82209068, g_memory.Translate(stateAddr)) & 0xFFu) != 0u;
    }

    Sonicteam::Player::IPostureControl* GetSonicPosture(uint8_t* base, Sonicteam::Player::State::SonicContext* pCtx, uint32_t& outPostureAddr)
    {
        outPostureAddr = 0u;
        if (!pCtx)
            return nullptr;

        auto* pScore = pCtx->m_spScore.get();
        if (!pScore)
            return nullptr;

        auto* pPlayer = pScore->m_pPlayer.get();
        if (!pPlayer)
            return nullptr;

        auto* pPosture = pPlayer->m_spPostureControl.get();
        if (!pPosture)
            return nullptr;

        const uint32_t postureAddr = g_memory.MapVirtual(pPosture);
        if (!IsValidGuestHeapPtr(postureAddr))
            return nullptr;

        if (ReadGuestU32(base, postureAddr) != kPostureControlVftable)
            return nullptr;

        outPostureAddr = postureAddr;
        return pPosture;
    }

    float ComputeChargedLaunchSpeed(float chargeTime, float entrySpeed)
    {
        const float t = std::clamp(chargeTime / kMaxChargeTime, 0.0f, 1.0f);
        const float eased = 1.0f - (1.0f - t) * (1.0f - t);
        const float charged = std::lerp(kMinLaunchSpeed, kMaxLaunchSpeed, eased);
        const float entry = std::clamp(entrySpeed, 0.0f, kMaxRollSpeed);
        const float bonus = std::lerp(kTapMomentumBoost, kFullMomentumBoost, t);
        const float boosted = std::min(entry + bonus, kMaxMomentumLaunchSpeed);
        return std::max({ charged, entry, boosted });
    }

    // Rotates Sonic's local surface yaw toward the ground-projected world stick direction.
    // Returns the dot product between current forward and ground-projected stick direction (or 0 if neutral).
    float SteerOnSurface(uint8_t* base, Sonicteam::Player::IPostureControl* pPosture, uint32_t postureAddr,
        float maxTurnRate, float dt, bool allowTurn, Vec3& outWorldForward)
    {
        const Quat surfaceQuat = NormalizeQuat({
            pPosture->m_SurfaceNormal.X.get(),
            pPosture->m_SurfaceNormal.Y.get(),
            pPosture->m_SurfaceNormal.Z.get(),
            pPosture->m_SurfaceNormal.W.get()
        });
        const Quat localRot = NormalizeQuat({
            pPosture->m_Rotation.X.get(),
            pPosture->m_Rotation.Y.get(),
            pPosture->m_Rotation.Z.get(),
            pPosture->m_Rotation.W.get()
        });

        const Quat worldRot = NormalizeQuat(QuatMul(surfaceQuat, localRot));
        const Vec3 worldUp = Normalize(QuatRotate(surfaceQuat, { 0.0f, 1.0f, 0.0f }), { 0.0f, 1.0f, 0.0f });
        outWorldForward = Normalize(QuatRotate(worldRot, { 0.0f, 0.0f, 1.0f }), { 0.0f, 0.0f, 1.0f });

        const float stickMag = ReadGuestF32(base, postureAddr + kPostureWorldInputMagOffset);
        if (!std::isfinite(stickMag) || stickMag < kMinStickMagnitude)
            return 0.0f;

        const Quat inputQuat = ReadGuestQuat(base, postureAddr + kPostureWorldInputQuatOffset);
        const Vec3 worldStickDir = Normalize(QuatRotate(inputQuat, { 0.0f, 0.0f, 1.0f }), { 0.0f, 0.0f, 0.0f });
        const Vec3 projected = worldStickDir - worldUp * Dot(worldStickDir, worldUp);
        if (LengthSq(projected) <= 1e-6f)
            return 0.0f;

        const Vec3 desiredFwd = Normalize(projected, outWorldForward);
        const float alignDot = std::clamp(Dot(outWorldForward, desiredFwd), -1.0f, 1.0f);

        if (allowTurn && maxTurnRate > 0.0f && dt > 0.0f)
        {
            const float sinAngle = Dot(worldUp, Cross(outWorldForward, desiredFwd));
            const float deltaAngle = std::atan2(sinAngle, alignDot);
            const float step = std::clamp(deltaAngle, -maxTurnRate * dt, maxTurnRate * dt);

            if (std::fabs(step) > 1e-5f)
            {
                const Quat newLocalRot = NormalizeQuat(QuatMul(localRot, QuatAxisAngleY(step)));
                const Quat newWorldRot = NormalizeQuat(QuatMul(surfaceQuat, newLocalRot));
                pPosture->m_Rotation = StoreQuat(newLocalRot);
                outWorldForward = Normalize(QuatRotate(newWorldRot, { 0.0f, 0.0f, 1.0f }), outWorldForward);

                guest_stack_var<Sonicteam::SoX::Math::Quaternion> worldRotVar(StoreQuat(newWorldRot));
                GuestToHostFunction<void>(sub_821F1968, pPosture, &pPosture->m_Position, worldRotVar.get());
            }
        }

        return alignDot;
    }

    struct SpindashRuntime
    {
        float chargeTime{};
        float entrySpeed{};
        float rollSpeed{};
    };

    std::mutex g_spindashMutex;
    std::unordered_map<uint32_t, SpindashRuntime> g_spindashStates;
    std::unordered_set<uint32_t> g_uncurlContexts;

    bool TryStartBAction(uint8_t* base, uint32_t machineAddr,
        Sonicteam::Player::State::SonicContext* pSonicCtx, uint32_t buttons)
    {
        if ((buttons & (InputListenerButtons::BTapReleased | InputListenerButtons::BHoldReleased)) == 0u ||
            (pSonicCtx->m_PostureFlags.get() & Sonicteam::Player::PostureControlFlags_Grounded) == 0u)
        {
            return false;
        }

        if ((buttons & InputListenerButtons::BHoldReleased) != 0u)
        {
            // Preserve the stock slide availability check used by SonicWalk and SonicRun.
            if ((GuestToHostFunction<uint32_t>(sub_82209058, pSonicCtx, 1u) & 0xFFu) == 0u)
                return false;

            return CallChangeState(base, machineAddr, kStateSonicSliding);
        }

        return CallChangeState(base, machineAddr, kStateSonicSpinkick);
    }

    void UpdateSonicGround(PPCContext& ctx, uint8_t* base, uint32_t sonicContextOffset,
        uint32_t suppressedButtons, void (*update)(PPCContext&, uint8_t*))
    {
        if (!Config::ImprovedSpindash)
        {
            update(ctx, base);
            return;
        }

        const uint32_t stateAddr = ctx.r3.u32;
        const uint32_t sonicCtxAddr = ReadGuestU32(base, stateAddr + sonicContextOffset);
        if (!IsValidGuestHeapPtr(sonicCtxAddr))
        {
            update(ctx, base);
            return;
        }

        auto* pSonicCtx = reinterpret_cast<Sonicteam::Player::State::SonicContext*>(base + sonicCtxAddr);
        const uint32_t machineAddr = ReadGuestU32(base, stateAddr + 4u);
        const uint32_t buttons = GetUnlockedButtons(pSonicCtx);
        const bool xPressed = (buttons & InputListenerButtons::XPhysicalPressed) != 0u;
        const bool lightDash = CanLightDashNow(pSonicCtx, buttons);

        if (xPressed && !lightDash &&
            CallChangeState(base, machineAddr, kStateSonicSpindash))
        {
            ctx.r3.u64 = 1u;
            return;
        }

        if ((buttons & kBtnAPressed) == 0u && !lightDash && TryStartBAction(base, machineAddr, pSonicCtx, buttons))
        {
            std::lock_guard lock(g_spindashMutex);
            g_uncurlContexts.erase(sonicCtxAddr);
            ctx.r3.u64 = 1u;
            return;
        }

        // Leave a real X press visible to the original Light Dash check.
        if (!xPressed)
            suppressedButtons |= kBtnXPressed;
        const uint32_t savedButtons = pSonicCtx->m_Buttons.get();
        const uint32_t maskedButtons = savedButtons & ~suppressedButtons;
        if (maskedButtons != savedButtons)
            pSonicCtx->m_Buttons = maskedButtons;

        update(ctx, base);

        if (maskedButtons != savedButtons)
            pSonicCtx->m_Buttons = savedButtons;
    }
}

// Shared ground movement step. f3 is the maximum forward speed used for this update.
PPC_FUNC_IMPL(__imp__sub_8220A630);
PPC_FUNC(sub_8220A630)
{
    const uint32_t contextAddr = ctx.r3.u32;
    const double normalLimit = ctx.f3.f64;
    Sonicteam::Player::State::SonicContext* pSonicCtx = nullptr;
    {
        std::lock_guard lock(g_spindashMutex);
        const auto it = g_uncurlContexts.find(contextAddr);
        if (it != g_uncurlContexts.end())
        {
            auto* pContext = reinterpret_cast<Sonicteam::Player::State::SonicContext*>(base + contextAddr);
            const auto flags = pContext->m_PostureFlags.get();
            const float speed = pContext->m_BaseSpeedForward.get();

            if (Config::ImprovedSpindash && std::isfinite(speed) && speed > normalLimit &&
                (flags & Sonicteam::Player::PostureControlFlags_Grounded) != 0u &&
                (flags & kPostureFlagWall) == 0u && pContext->m_LockButtons.get() <= 0.0f)
            {
                const double dt = std::clamp(ctx.f1.f64, 0.0, 0.1);
                ctx.f3.f64 = std::max(normalLimit, speed - kUncurlDeceleration * dt);
                pSonicCtx = pContext;
            }
            else
            {
                g_uncurlContexts.erase(it);
            }
        }
    }

    __imp__sub_8220A630(ctx, base);
    if (pSonicCtx)
    {
        ctx.f3.f64 = normalLimit;
        // Keep the original return value's meaning for animation and state transitions.
        const float speed = pSonicCtx->m_BaseSpeedForward.get() + pSonicCtx->m_GimmickSpeedForward.get();
        ctx.r3.u64 = speed >= normalLimit ? 1u : 0u;
    }
}

// Shared context destructor used by SonicContext's interface thunks.
PPC_FUNC_IMPL(__imp__sub_82214100);
PPC_FUNC(sub_82214100)
{
    {
        std::lock_guard lock(g_spindashMutex);
        g_uncurlContexts.erase(ctx.r3.u32);
    }

    __imp__sub_82214100(ctx, base);
}

// Sonicteam::Player::State::SonicWait::Update
PPC_FUNC_IMPL(__imp__sub_82218A30);
PPC_FUNC(sub_82218A30)
{
    // Suppress the original charge on held X as well as the Spin Kick on release.
    UpdateSonicGround(ctx, base, 24u, kBtnXHeld | kBtnXReleased, __imp__sub_82218A30);
}

// Sonicteam::Player::State::SonicWalk::Update
PPC_FUNC_IMPL(__imp__sub_82218D60);
PPC_FUNC(sub_82218D60)
{
    UpdateSonicGround(ctx, base, 12u, kBtnXReleased, __imp__sub_82218D60);
}

// Sonicteam::Player::State::SonicRun::Update
PPC_FUNC_IMPL(__imp__sub_82219040);
PPC_FUNC(sub_82219040)
{
    UpdateSonicGround(ctx, base, 12u, kBtnXReleased, __imp__sub_82219040);
}

// Sonicteam::Player::State::SonicSpindash::OnStart
PPC_FUNC_IMPL(__imp__sub_82217658);
PPC_FUNC(sub_82217658)
{
    const uint32_t stateAddr = ctx.r3.u32;
    __imp__sub_82217658(ctx, base);

    if (!Config::ImprovedSpindash || !IsValidGuestHeapPtr(stateAddr))
        return;

    const uint32_t sonicCtxAddr = ReadGuestU32(base, stateAddr + 12u);
    float entrySpeed = 0.0f;
    if (IsValidGuestHeapPtr(sonicCtxAddr))
    {
        auto* pSonicCtx = reinterpret_cast<Sonicteam::Player::State::SonicContext*>(base + sonicCtxAddr);
        entrySpeed = std::max(0.0f, pSonicCtx->m_BaseSpeedForward.get());
        pSonicCtx->m_MiscFlags[kMiscSlotSpindashFlag] = 1u;
    }

    WriteGuestU32(base, stateAddr + 16u, 0u); // Begin charging.
    std::lock_guard lock(g_spindashMutex);
    g_uncurlContexts.erase(sonicCtxAddr);
    g_spindashStates[stateAddr] = SpindashRuntime{ .entrySpeed = entrySpeed };
}

// Sonicteam::Player::State::SonicSpindash::OnEnd
PPC_FUNC_IMPL(__imp__sub_82217670);
PPC_FUNC(sub_82217670)
{
    const uint32_t stateAddr = ctx.r3.u32;
    {
        std::lock_guard lock(g_spindashMutex);
        g_spindashStates.erase(stateAddr);
    }

    __imp__sub_82217670(ctx, base);
}

// Sonicteam::Player::State::SonicSpindash::Update
PPC_FUNC_IMPL(__imp__sub_82217AF0);
PPC_FUNC(sub_82217AF0)
{
    if (!Config::ImprovedSpindash)
    {
        __imp__sub_82217AF0(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const float dt = std::clamp(static_cast<float>(ctx.f1.f64), 0.0f, 0.1f);

    if (!IsValidGuestHeapPtr(stateAddr))
    {
        __imp__sub_82217AF0(ctx, base);
        return;
    }

    const uint32_t machineAddr = ReadGuestU32(base, stateAddr + 4u);
    const uint32_t sonicCtxAddr = ReadGuestU32(base, stateAddr + 12u);
    if (!IsValidGuestHeapPtr(sonicCtxAddr))
    {
        __imp__sub_82217AF0(ctx, base);
        return;
    }

    auto* pSonicCtx = reinterpret_cast<Sonicteam::Player::State::SonicContext*>(base + sonicCtxAddr);

    // Preserve standard engine interrupts (damage, springs, dash panels, rails, external transitions).
    if ((GuestToHostFunction<uint32_t>(sub_8220AAF8, g_memory.Translate(stateAddr)) & 0xFFu) != 0u)
    {
        ctx.r3.u64 = 1u;
        return;
    }

    SpindashRuntime rt{};
    {
        std::lock_guard lock(g_spindashMutex);
        auto it = g_spindashStates.find(stateAddr);
        if (it != g_spindashStates.end())
        {
            rt = it->second;
        }
        else
        {
            rt.entrySpeed = std::max(0.0f, pSonicCtx->m_BaseSpeedForward.get());
            rt.rollSpeed = rt.entrySpeed;
        }
    }

    const uint32_t buttons = GetUnlockedButtons(pSonicCtx);
    const uint32_t postureFlags = static_cast<uint32_t>(pSonicCtx->m_PostureFlags.get());
    const uint32_t phase = ReadGuestU32(base, stateAddr + 16u);

    uint32_t postureAddr = 0u;
    auto* pPosture = GetSonicPosture(base, pSonicCtx, postureAddr);

    // Rolled or slid off a ledge: preserve horizontal speed and transition to Fall.
    if ((postureFlags & Sonicteam::Player::PostureControlFlags_Grounded) == 0u)
    {
        const float carrySpeed = (phase == 1u)
            ? std::max(pSonicCtx->m_BaseSpeedForward.get(), rt.rollSpeed)
            : pSonicCtx->m_BaseSpeedForward.get();
        pSonicCtx->m_BaseSpeedForward = carrySpeed;

        if (CallChangeState(base, machineAddr, kStateFall))
        {
            pSonicCtx->m_BaseSpeedForward = std::max(pSonicCtx->m_BaseSpeedForward.get(), carrySpeed);
            ctx.r3.u64 = 1u;
            return;
        }

        ctx.r3.u64 = CallSpindashExit(stateAddr) ? 1u : 0u;
        return;
    }

    if (phase == 0u)
    {
        // Phase 0: Charging.
        // Allow Spindash Jump directly out of charge on A press.
        if ((buttons & kBtnAPressed) != 0u)
        {
            const float jumpCarrySpeed = (rt.chargeTime >= 0.08f)
                ? ComputeChargedLaunchSpeed(rt.chargeTime, rt.entrySpeed)
                : std::max(pSonicCtx->m_BaseSpeedForward.get(), rt.entrySpeed);

            pSonicCtx->m_BaseSpeedForward = jumpCarrySpeed;
            if (CallChangeState(base, machineAddr, kStateJump))
            {
                pSonicCtx->m_BaseSpeedForward = std::max(pSonicCtx->m_BaseSpeedForward.get(), jumpCarrySpeed);
                ctx.r3.u64 = 1u;
                return;
            }
        }

        // Smoothly decelerate entry speed into crouch-charge while accumulating charge power.
        const float curSpeed = std::max(0.0f, pSonicCtx->m_BaseSpeedForward.get() - kChargeEntryDecel * dt);
        pSonicCtx->m_BaseSpeedForward = curSpeed;
        pSonicCtx->m_BaseSpeedVertical = 0.0f;
        rt.chargeTime = std::min(rt.chargeTime + dt, kMaxChargeTime);

        if (pPosture)
        {
            Vec3 worldFwd{};
            SteerOnSurface(base, pPosture, postureAddr, kChargeAimTurnRate, dt, true, worldFwd);
        }

        // Release X (or quick tap where X is already up) launches into Phase 1 rolling.
        if ((buttons & kBtnXHeld) == 0u)
        {
            const float launchSpeed = ComputeChargedLaunchSpeed(rt.chargeTime, rt.entrySpeed);
            rt.rollSpeed = launchSpeed;

            pSonicCtx->m_BaseSpeedForward = launchSpeed;
            pSonicCtx->m_BaseSpeedVertical = 0.0f;
            pSonicCtx->m_MiscFlags[kMiscSlotAttackHitbox] = 1u;
            WriteGuestU32(base, stateAddr + 16u, 1u);
            GuestToHostFunction<void>(sub_82208F48, pSonicCtx, kAnimSpindashRoll);

            LOGF("[spindash] Launch (charge={:.2f}s, entry={:.0f}, launch={:.0f})",
                rt.chargeTime, rt.entrySpeed, launchSpeed);
        }

        {
            std::lock_guard lock(g_spindashMutex);
            g_spindashStates[stateAddr] = rt;
        }

        ctx.r3.u64 = 0u;
        return;
    }

    // Phase 1: Speed-based rolling spinball.

    // Spindash Jump: pressing A while rolling jumps while preserving current rolling momentum.
    if ((buttons & kBtnAPressed) != 0u)
    {
        const float jumpCarrySpeed = std::max(pSonicCtx->m_BaseSpeedForward.get(), rt.rollSpeed);
        pSonicCtx->m_BaseSpeedForward = jumpCarrySpeed;

        if (CallChangeState(base, machineAddr, kStateJump))
        {
            pSonicCtx->m_BaseSpeedForward = std::max(pSonicCtx->m_BaseSpeedForward.get(), jumpCarrySpeed);
            ctx.r3.u64 = 1u;
            return;
        }
    }

    // Wall impact exits rolling.
    if ((postureFlags & kPostureFlagWall) != 0u)
    {
        if (CallSpindashExit(stateAddr))
        {
            ctx.r3.u64 = 1u;
            return;
        }
    }

    if ((buttons & InputListenerButtons::XPhysicalPressed) != 0u)
    {
        const float carrySpeed = std::max(pSonicCtx->m_BaseSpeedForward.get(), rt.rollSpeed);
        pSonicCtx->m_BaseSpeedForward = carrySpeed;

        // Consume this press so the ground state cannot use it to start another charge.
        const uint32_t savedButtons = pSonicCtx->m_Buttons.get();
        pSonicCtx->m_Buttons = savedButtons & ~(InputListenerButtons::XPhysicalPressed | kBtnXPressed);
        {
            std::lock_guard lock(g_spindashMutex);
            g_uncurlContexts.insert(sonicCtxAddr);
        }

        if (CallSpindashExit(stateAddr))
        {
            const float exitSpeed = std::max(pSonicCtx->m_BaseSpeedForward.get(), carrySpeed);
            pSonicCtx->m_BaseSpeedForward = exitSpeed;
            LOGF("[spindash] Exit on X (roll={:.0f}, exit={:.0f})", carrySpeed, exitSpeed);
            ctx.r3.u64 = 1u;
            return;
        }
        pSonicCtx->m_Buttons = savedButtons;
        std::lock_guard lock(g_spindashMutex);
        g_uncurlContexts.erase(sonicCtxAddr);
    }

    if ((buttons & kBtnAPressed) == 0u && TryStartBAction(base, machineAddr, pSonicCtx, buttons))
    {
        ctx.r3.u64 = 1u;
        return;
    }

    // Sync with any external speed boosts (e.g. dash pads) without losing tracked roll speed.
    const float engineSpeed = pSonicCtx->m_BaseSpeedForward.get();
    if (std::isfinite(engineSpeed) && engineSpeed > rt.rollSpeed)
        rt.rollSpeed = std::min(engineSpeed, kMaxRollSpeed);

    Vec3 worldFwd{ 0.0f, 0.0f, 1.0f };
    float stickAlign = 0.0f;
    if (pPosture)
    {
        const float speedAlpha = std::clamp((rt.rollSpeed - kMinRollExitSpeed) / (kMaxLaunchSpeed - kMinRollExitSpeed), 0.0f, 1.0f);
        const float turnRate = kRollTurnRateSlow + (kRollTurnRateFast - kRollTurnRateSlow) * speedAlpha;

        // First probe alignment without turning if player is pulling directly backward to brake.
        const float probeAlign = SteerOnSurface(base, pPosture, postureAddr, 0.0f, 0.0f, false, worldFwd);
        const bool allowTurn = probeAlign > -0.55f;
        stickAlign = SteerOnSurface(base, pPosture, postureAddr, turnRate, dt, allowTurn, worldFwd);
    }

    // Slope physics: downhill accelerates (worldFwd.y < 0), uphill decelerates (worldFwd.y > 0).
    const float slopeAccel = -worldFwd.y * kSlopeGravityAccel;

    float friction = kRollFrictionNeutral;
    if (stickAlign >= 0.25f)
        friction = kRollFrictionForward;
    else if (stickAlign <= -0.35f)
        friction = kRollFrictionBrake;

    rt.rollSpeed = std::clamp(rt.rollSpeed + (slopeAccel - friction) * dt, 0.0f, kMaxRollSpeed);
    pSonicCtx->m_BaseSpeedForward = rt.rollSpeed;
    pSonicCtx->m_BaseSpeedVertical = 0.0f;
    pSonicCtx->m_MiscFlags[kMiscSlotAttackHitbox] = 1u;

    {
        std::lock_guard lock(g_spindashMutex);
        g_spindashStates[stateAddr] = rt;
    }

    // Uncurl naturally when rolling speed drops below threshold.
    if (rt.rollSpeed < kMinRollExitSpeed)
    {
        if (CallSpindashExit(stateAddr))
        {
            ctx.r3.u64 = 1u;
            return;
        }
    }

    ctx.r3.u64 = 0u;
}
