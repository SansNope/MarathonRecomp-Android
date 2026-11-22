#include "input_patches.h"
#include <api/Marathon.h>
#include <kernel/function.h>
#include <os/logger.h>
#include <user/config.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>

// P-06-style Mach Speed controls & wall slam for Sonic (state_module_fast).
//
// When Config::ImprovedMachSpeed is enabled:
// - Speed control (three tiers, matching P-06 SonicFast.ControlSpeed):
//   * Stick forward: accelerates smoothly up to c_run_speed_max.
//   * Stick neutral / sideways: smoothly converges to cruise speed (c_walk_speed_max).
//   * Stick backward: smoothly brakes (2x acceleration, analog) down to a reduced
//     slow tier (~54.5% of cruise speed) for tighter turning and precise jumps.
// - Landing / re-entering FastRun preserves braked speed down to the slow tier
//   instead of snapping back up to cruise speed.
// - Wall collisions (P-06 OnWallSlam / StateWallSlam / StateAirSlam):
//   * Below 90% of c_run_speed_max: no ring loss and no instant death; Sonic
//     performs a brief knockback recovery (WallSlam on ground, AirSlam in air)
//     and resumes at slow speed with brief wall-slam immunity.
//   * At or above 90% of c_run_speed_max: triggers standard full-speed wall crash
//     damage (ring scatter or death at 0 rings).

namespace
{
    constexpr uint32_t kPostureControlVftable = 0x82009048u;
    constexpr uint32_t kPostureWorldInputQuatOffset = 0x2E0u;
    constexpr uint32_t kPostureWorldInputMagOffset = 0x2F0u;

    // FastContext offsets (verified from ppc_recomp.14.cpp).
    constexpr uint32_t kFastBaseSpeedForwardOffset = 0x30u;     // +48
    constexpr uint32_t kFastGimmickSpeedForwardOffset = 0x34u;  // +52
    constexpr uint32_t kFastLockButtonsOffset = 0x44u;          // +68
    constexpr uint32_t kFastButtonsOffset = 0x54u;              // +84
    constexpr uint32_t kFastCurrentStickBorderOffset = 0x58u;   // +88
    constexpr uint32_t kFastInvincibleTimerOffset = 0xA8u;      // +168
    constexpr uint32_t kFastPostureFlagsOffset = 0xACu;         // +172
    constexpr uint32_t kFastFlagLockControlOffset = 0xC1u;      // +193
    constexpr uint32_t kFastFlagDamageOffset = 0xC2u;           // +194
    constexpr uint32_t kFastFlagDeadOrScriptOffset = 0xC4u;     // +196
    constexpr uint32_t kFastFlagLightDashOffset = 0xC9u;        // +201
    constexpr uint32_t kFastFlagInvincible2Offset = 0xCAu;      // +202
    constexpr uint32_t kFastFlagInvincible1Offset = 0xD0u;      // +208
    constexpr uint32_t kFastScorePtrOffset = 0xD4u;             // +212
    constexpr uint32_t kFastStickBorderOffset = 0xE4u;          // +228
    constexpr uint32_t kFastRunAccOffset = 0xE8u;               // +232
    constexpr uint32_t kFastWalkSpeedMaxOffset = 0xECu;         // +236
    constexpr uint32_t kFastRunSpeedMaxOffset = 0xF0u;          // +240
    constexpr uint32_t kFastBrakeAccOffset = 0xF4u;             // +244
    constexpr uint32_t kFastBrakeDashPanelOffset = 0xF8u;       // +248

    // FastContext::m_PostureFlags bits.
    constexpr uint32_t kFastPostureGrounded = 0x01u;
    constexpr uint32_t kFastPostureWall = 0x10u;

    // Animations in sonic_fast.pkg (verified from sub_822102F8, sub_82210410, sub_822115F8).
    constexpr uint32_t kFastAnimRun = 0u;
    constexpr uint32_t kFastAnimDamage = 7u;

    // P-06 tuning ratios.
    constexpr float kSlowTierRatio = 0.545f;           // P-06: 30 / 55
    constexpr float kMinSlowSpeed = 1500.0f;           // Floor in cm/s
    constexpr float kWallDamageSpeedRatio = 0.90f;     // P-06: c_run_speed_max * 0.9f
    constexpr float kBrakeDotThreshold = -0.35f;       // Stick pulled opposite to facing
    constexpr float kAccelDotThreshold = 0.25f;        // Stick pushed toward facing
    constexpr float kGroundSlamDuration = 0.45f;       // WallSlam knockback recovery (s)
    constexpr float kAirSlamDuration = 0.65f;          // AirSlam horizontal decay (s)
    constexpr float kAirSlamMaxDuration = 0.85f;       // AirSlam safety cap (s)
    constexpr float kSlamCooldownDuration = 0.65f;     // Post-slam wall immunity (s)
    constexpr float kBounceSpeedRatio = 0.55f;         // Knockback speed relative to slow tier
    constexpr float kMinBounceSpeed = 800.0f;
    constexpr float kMaxBounceSpeed = 1600.0f;

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

    struct MachSpeedRuntime
    {
        bool slamActive{};
        bool slamAirborne{};
        bool lockControlOwned{};
        float slamElapsed{};
        float bounceSpeed{};
        float slamCooldown{};
        float lastSlowSpeedMin{ 2000.0f };
    };

    std::mutex g_machSpeedMutex;
    std::unordered_map<uint32_t, MachSpeedRuntime> g_machSpeedStates;

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

    inline Vec3 Normalize(const Vec3& v, const Vec3& fallback = { 0.0f, 0.0f, 1.0f })
    {
        const float lenSq = Dot(v, v);
        if (lenSq <= 1e-8f)
            return fallback;

        const float invLen = 1.0f / std::sqrt(lenSq);
        return v * invLen;
    }

    // Matches Xbox 360 XNAMath row-vector quaternion convention used by PostureControl.
    inline Vec3 QuatRotate(const Quat& q, const Vec3& v)
    {
        const Vec3 qv{ q.x, q.y, q.z };
        const Vec3 t = Cross(qv, v) + v * q.w;
        return v + Cross(qv, t) * 2.0f;
    }

    inline bool IsValidGuestHeapPtr(uint32_t addr)
    {
        return addr >= 0x20000u && addr < 0x7FEA0000u;
    }

    inline uint8_t ReadGuestU8(const uint8_t* base, uint32_t addr)
    {
        return *(base + addr);
    }

    inline void WriteGuestU8(uint8_t* base, uint32_t addr, uint8_t value)
    {
        *(base + addr) = value;
    }

    inline uint32_t ReadGuestU32(const uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<uint32_t>*>(base + addr)->get();
    }

    inline float ReadGuestF32(const uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<float>*>(base + addr)->get();
    }

    inline void WriteGuestF32(uint8_t* base, uint32_t addr, float value)
    {
        *reinterpret_cast<be<float>*>(base + addr) = value;
    }

    inline Quat ReadGuestQuat(const uint8_t* base, uint32_t addr)
    {
        const auto* q = reinterpret_cast<const Sonicteam::SoX::Math::Quaternion*>(base + addr);
        return { q->X.get(), q->Y.get(), q->Z.get(), q->W.get() };
    }

    inline float ComputeSlowSpeedMin(float walkSpeedMax)
    {
        if (walkSpeedMax <= 0.0f)
            return kMinSlowSpeed;

        const float scaled = walkSpeedMax * kSlowTierRatio;
        return std::clamp(scaled, std::min(kMinSlowSpeed, walkSpeedMax), walkSpeedMax * 0.70f);
    }

    Sonicteam::Player::IPostureControl* GetFastPosture(const uint8_t* base, uint32_t fastCtxAddr, uint32_t& outPostureAddr)
    {
        outPostureAddr = 0u;
        if (!IsValidGuestHeapPtr(fastCtxAddr))
            return nullptr;

        const uint32_t scoreAddr = ReadGuestU32(base, fastCtxAddr + kFastScorePtrOffset);
        if (!IsValidGuestHeapPtr(scoreAddr))
            return nullptr;

        auto* pScore = reinterpret_cast<Sonicteam::Player::Score*>(g_memory.Translate(scoreAddr));
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

    bool SampleStickForwardDot(const uint8_t* base, uint32_t fastCtxAddr, float& outStickMag, float& outForwardDot)
    {
        outStickMag = 0.0f;
        outForwardDot = 0.0f;

        uint32_t postureAddr = 0u;
        auto* pPosture = GetFastPosture(base, fastCtxAddr, postureAddr);
        if (!pPosture)
            return false;

        outStickMag = ReadGuestF32(base, postureAddr + kPostureWorldInputMagOffset);
        if (outStickMag <= 1e-4f)
            return true;

        Vec3 up{
            pPosture->m_SurfaceNormal.X.get(),
            pPosture->m_SurfaceNormal.Y.get(),
            pPosture->m_SurfaceNormal.Z.get()
        };
        if (Dot(up, up) <= 1e-6f)
            up = { 0.0f, 1.0f, 0.0f };
        else
            up = Normalize(up, { 0.0f, 1.0f, 0.0f });

        const Quat bodyRot{
            pPosture->m_Rotation.X.get(),
            pPosture->m_Rotation.Y.get(),
            pPosture->m_Rotation.Z.get(),
            pPosture->m_Rotation.W.get()
        };
        Vec3 forward = QuatRotate(bodyRot, { 0.0f, 0.0f, 1.0f });
        forward = forward - up * Dot(forward, up);
        if (Dot(forward, forward) <= 1e-6f)
            return false;
        forward = Normalize(forward);

        const Quat stickQuat = ReadGuestQuat(base, postureAddr + kPostureWorldInputQuatOffset);
        Vec3 stickDir = QuatRotate(stickQuat, { 0.0f, 0.0f, 1.0f });
        stickDir = stickDir - up * Dot(stickDir, up);
        if (Dot(stickDir, stickDir) <= 1e-6f)
            return false;
        stickDir = Normalize(stickDir);

        outForwardDot = std::clamp(Dot(forward, stickDir), -1.0f, 1.0f);
        return true;
    }

    void SetFastAnimation(uint32_t fastCtxAddr, uint32_t animId)
    {
        GuestToHostFunction<void>(sub_82208F48, g_memory.Translate(fastCtxAddr), animId);
    }

    void EndSlamLocked(uint8_t* base, uint32_t fastCtxAddr, MachSpeedRuntime& rt, bool restoreSpeedAndAnim)
    {
        rt.slamActive = false;
        rt.slamAirborne = false;
        rt.slamElapsed = 0.0f;
        rt.slamCooldown = kSlamCooldownDuration;

        if (rt.lockControlOwned && IsValidGuestHeapPtr(fastCtxAddr))
        {
            WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 0u);
            rt.lockControlOwned = false;
        }

        if (restoreSpeedAndAnim && IsValidGuestHeapPtr(fastCtxAddr))
        {
            const float walkSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastWalkSpeedMaxOffset);
            const float resumeSpeed = ComputeSlowSpeedMin(walkSpeedMax);
            WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, resumeSpeed);
            WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
            SetFastAnimation(fastCtxAddr, kFastAnimRun);
        }
    }
}

// FastContext::Update (0x82210898)
// Ticks post-slam cooldown and keeps lock-control flag synced with active WallSlam/AirSlam.
PPC_FUNC_IMPL(__imp__sub_82210898);
PPC_FUNC(sub_82210898)
{
    const uint32_t fastCtxAddr = ctx.r3.u32;
    const float dt = std::clamp(static_cast<float>(ctx.f1.f64), 0.0f, 0.1f);

    if (IsValidGuestHeapPtr(fastCtxAddr))
    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        auto it = g_machSpeedStates.find(fastCtxAddr);
        if (it != g_machSpeedStates.end())
        {
            auto& rt = it->second;
            if (!Config::ImprovedMachSpeed)
            {
                if (rt.lockControlOwned)
                    WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 0u);
                g_machSpeedStates.erase(it);
            }
            else
            {
                if (rt.slamCooldown > 0.0f)
                    rt.slamCooldown = std::max(0.0f, rt.slamCooldown - dt);

                if (rt.slamActive)
                {
                    const bool interrupted =
                        ReadGuestU8(base, fastCtxAddr + kFastFlagDamageOffset) != 0u ||
                        ReadGuestU8(base, fastCtxAddr + kFastFlagDeadOrScriptOffset) != 0u ||
                        ReadGuestU8(base, fastCtxAddr + kFastFlagLightDashOffset) != 0u;
                    if (interrupted)
                    {
                        EndSlamLocked(base, fastCtxAddr, rt, false);
                    }
                    else
                    {
                        WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 1u);
                        rt.lockControlOwned = true;
                    }
                }
            }
        }
    }

    __imp__sub_82210898(ctx, base);
}

// FastRun::OnStart (0x822102F8)
// Preserves reduced (braked) speed down to the slow tier on landing / state entry
// instead of snapping back up to cruise speed (c_walk_speed_max).
PPC_FUNC_IMPL(__imp__sub_822102F8);
PPC_FUNC(sub_822102F8)
{
    if (!Config::ImprovedMachSpeed)
    {
        __imp__sub_822102F8(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t fastCtxAddr = IsValidGuestHeapPtr(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0u;
    if (!IsValidGuestHeapPtr(fastCtxAddr))
    {
        __imp__sub_822102F8(ctx, base);
        return;
    }

    const float prevBaseSpeed = ReadGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset);
    const float prevGimmickSpeed = ReadGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset);
    const float walkSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastWalkSpeedMaxOffset);
    const float slowSpeedMin = ComputeSlowSpeedMin(walkSpeedMax);

    bool endedAirSlam = false;
    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        auto it = g_machSpeedStates.find(fastCtxAddr);
        if (it != g_machSpeedStates.end() && it->second.slamActive)
        {
            EndSlamLocked(base, fastCtxAddr, it->second, false);
            endedAirSlam = true;
        }
    }

    __imp__sub_822102F8(ctx, base);

    if (endedAirSlam)
    {
        WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, slowSpeedMin);
        WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
        LOGF_UTILITY("[mach_speed] AirSlam landed -> resume={:.1f}", slowSpeedMin);
    }
    else if (prevBaseSpeed >= slowSpeedMin && prevBaseSpeed < walkSpeedMax && prevGimmickSpeed <= 0.0f)
    {
        // Keep the player's braked speed across jumps/landings instead of snapping up to cruise.
        WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, prevBaseSpeed);
        WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
    }
}

// FastContext::UpdateSpeed (0x82210B50)
// P-06-style three-tier speed control:
// - Stick forward: accelerate to c_run_speed_max.
// - Stick backward: smooth analog braking (up to 2x accel) down to slowSpeedMin (~54.5% of cruise).
// - Neutral / sideways: converge smoothly toward cruise speed (c_walk_speed_max).
PPC_FUNC_IMPL(__imp__sub_82210B50);
PPC_FUNC(sub_82210B50)
{
    if (!Config::ImprovedMachSpeed)
    {
        __imp__sub_82210B50(ctx, base);
        return;
    }

    const uint32_t fastCtxAddr = ctx.r3.u32;
    const float dt = std::clamp(static_cast<float>(ctx.f1.f64), 0.0f, 0.1f);
    if (!IsValidGuestHeapPtr(fastCtxAddr))
    {
        __imp__sub_82210B50(ctx, base);
        return;
    }

    const float stickBorder = ReadGuestF32(base, fastCtxAddr + kFastStickBorderOffset);
    const float runAcc = ReadGuestF32(base, fastCtxAddr + kFastRunAccOffset);
    const float walkSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastWalkSpeedMaxOffset);
    const float runSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastRunSpeedMaxOffset);
    const float brakeAcc = ReadGuestF32(base, fastCtxAddr + kFastBrakeAccOffset);
    const float brakeDashPanel = ReadGuestF32(base, fastCtxAddr + kFastBrakeDashPanelOffset);

    if (walkSpeedMax <= 0.0f || runSpeedMax <= 0.0f)
    {
        __imp__sub_82210B50(ctx, base);
        return;
    }

    float baseSpeed = ReadGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset);
    float gimmickSpeed = ReadGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset);

    const float slowSpeedMin = ComputeSlowSpeedMin(walkSpeedMax);
    const float cruiseSpeed = walkSpeedMax;
    const float maxSpeed = runSpeedMax;

    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        g_machSpeedStates[fastCtxAddr].lastSlowSpeedMin = slowSpeedMin;
    }

    float stickMag = ReadGuestF32(base, fastCtxAddr + kFastCurrentStickBorderOffset);
    float forwardDot = 1.0f;
    float sampledMag = 0.0f;
    float sampledDot = 0.0f;
    if (SampleStickForwardDot(base, fastCtxAddr, sampledMag, sampledDot))
    {
        stickMag = sampledMag;
        forwardDot = sampledDot;
    }

    const float lockButtons = ReadGuestF32(base, fastCtxAddr + kFastLockButtonsOffset);
    const uint32_t buttons = (lockButtons <= 0.0f) ? ReadGuestU32(base, fastCtxAddr + kFastButtonsOffset) : 0u;
    const bool bBraking = (buttons & InputListenerButtons::BDown) != 0u;

    const bool stickActive = stickMag >= stickBorder;
    const bool stickBraking = stickActive && forwardDot < kBrakeDotThreshold;
    const bool braking = bBraking || stickBraking;
    const bool accelerating = !bBraking && stickActive && forwardDot > kAccelDotThreshold;

    if (braking)
    {
        const float brakeInput = bBraking
            ? 1.0f
            : std::clamp((-forwardDot - (-kBrakeDotThreshold)) / (1.0f - (-kBrakeDotThreshold)), 0.25f, 1.0f);
        if (gimmickSpeed > 0.0f)
        {
            const float gimmickBrake = std::max(brakeDashPanel * 2.5f, runAcc * 7.5f) * brakeInput;
            gimmickSpeed = std::max(0.0f, gimmickSpeed - gimmickBrake * dt);
        }

        const float brakeRate = std::max(brakeAcc, runAcc * 2.0f) * brakeInput;
        if (baseSpeed > slowSpeedMin)
            baseSpeed = std::max(slowSpeedMin, baseSpeed - brakeRate * dt);
        else if (baseSpeed < slowSpeedMin)
            baseSpeed = std::min(slowSpeedMin, baseSpeed + runAcc * 2.0f * dt);
    }
    else
    {
        if (gimmickSpeed > 0.0f)
            gimmickSpeed = std::max(0.0f, gimmickSpeed - brakeDashPanel * dt);

        if (accelerating)
        {
            if (baseSpeed < maxSpeed)
                baseSpeed = std::min(maxSpeed, baseSpeed + runAcc * dt);
        }
        else
        {
            // Neutral or pure sideways steering: converge smoothly to cruise speed.
            if (baseSpeed < cruiseSpeed)
                baseSpeed = std::min(cruiseSpeed, baseSpeed + runAcc * dt);
            else if (baseSpeed > cruiseSpeed && gimmickSpeed <= 0.0f)
                baseSpeed = std::max(cruiseSpeed, baseSpeed - std::max(brakeAcc, runAcc) * dt);
        }
    }

    WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, baseSpeed);
    WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, gimmickSpeed);

    ctx.r3.s64 = (baseSpeed + gimmickSpeed >= maxSpeed) ? 1 : 0;
}

// FastCommonTransitions (0x822113C8)
// Intercepts Mach Speed wall collisions (m_PostureFlags & 0x10):
// - Below 90% of c_run_speed_max: triggers P-06 WallSlam / AirSlam (no ring loss / no death).
// - At or above 90% of c_run_speed_max: runs stock full-speed wall damage.
PPC_FUNC_IMPL(__imp__sub_822113C8);
PPC_FUNC(sub_822113C8)
{
    if (!Config::ImprovedMachSpeed)
    {
        __imp__sub_822113C8(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t fastCtxAddr = IsValidGuestHeapPtr(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0u;
    if (!IsValidGuestHeapPtr(fastCtxAddr))
    {
        __imp__sub_822113C8(ctx, base);
        return;
    }

    const uint32_t postureFlags = ReadGuestU32(base, fastCtxAddr + kFastPostureFlagsOffset);
    const bool hitWall = (postureFlags & kFastPostureWall) != 0u;
    if (!hitWall)
    {
        __imp__sub_822113C8(ctx, base);
        return;
    }

    const bool stockInvincible =
        ReadGuestU8(base, fastCtxAddr + kFastFlagInvincible1Offset) != 0u ||
        ReadGuestU8(base, fastCtxAddr + kFastFlagInvincible2Offset) != 0u ||
        ReadGuestF32(base, fastCtxAddr + kFastInvincibleTimerOffset) > 0.0f ||
        ReadGuestU8(base, fastCtxAddr + kFastFlagDamageOffset) != 0u ||
        ReadGuestU8(base, fastCtxAddr + kFastFlagDeadOrScriptOffset) != 0u;

    if (stockInvincible)
    {
        __imp__sub_822113C8(ctx, base);
        return;
    }

    const float baseSpeed = ReadGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset);
    const float gimmickSpeed = ReadGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset);
    const float totalSpeed = baseSpeed + gimmickSpeed;
    const float walkSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastWalkSpeedMaxOffset);
    const float runSpeedMax = ReadGuestF32(base, fastCtxAddr + kFastRunSpeedMaxOffset);
    const float dangerSpeed = runSpeedMax * kWallDamageSpeedRatio;
    const bool grounded = (postureFlags & kFastPostureGrounded) != 0u;

    bool suppressWallDamage = false;
    bool startedSlam = false;
    float startedBounceSpeed = 0.0f;
    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        auto& rt = g_machSpeedStates[fastCtxAddr];
        if (rt.slamActive || rt.slamCooldown > 0.0f)
        {
            suppressWallDamage = true;
        }
        else if (runSpeedMax > 0.0f && totalSpeed < dangerSpeed)
        {
            const float slowSpeedMin = ComputeSlowSpeedMin(walkSpeedMax);
            const float bounce = std::clamp(slowSpeedMin * kBounceSpeedRatio, kMinBounceSpeed, kMaxBounceSpeed);
            rt.slamActive = true;
            rt.slamAirborne = !grounded;
            rt.slamElapsed = 0.0f;
            rt.bounceSpeed = bounce;
            rt.lockControlOwned = true;
            rt.lastSlowSpeedMin = slowSpeedMin;

            suppressWallDamage = true;
            startedSlam = true;
            startedBounceSpeed = bounce;
        }
    }

    if (startedSlam)
    {
        WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 1u);
        WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, -startedBounceSpeed);
        WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
        SetFastAnimation(fastCtxAddr, kFastAnimDamage);
        LOGF_UTILITY("[mach_speed] {} start (speed={:.1f} < danger={:.1f}, bounce={:.1f})",
            grounded ? "WallSlam" : "AirSlam", totalSpeed, dangerSpeed, startedBounceSpeed);
    }

    if (suppressWallDamage)
    {
        // Clear wall bit only for the duration of __imp__sub_822113C8 so other transitions
        // (springs, rails, goal, hazards) still run normally without invoking wall damage.
        *reinterpret_cast<be<uint32_t>*>(base + fastCtxAddr + kFastPostureFlagsOffset) = postureFlags & ~kFastPostureWall;
        __imp__sub_822113C8(ctx, base);
        *reinterpret_cast<be<uint32_t>*>(base + fastCtxAddr + kFastPostureFlagsOffset) = postureFlags;
        return;
    }

    __imp__sub_822113C8(ctx, base);
}

// FastRun::Update (0x822115F8)
// Drives ground WallSlam knockback recovery when active; otherwise delegates to stock FastRun::Update.
PPC_FUNC_IMPL(__imp__sub_822115F8);
PPC_FUNC(sub_822115F8)
{
    if (!Config::ImprovedMachSpeed)
    {
        __imp__sub_822115F8(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t fastCtxAddr = IsValidGuestHeapPtr(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0u;
    const float dt = std::clamp(static_cast<float>(ctx.f1.f64), 0.0f, 0.1f);

    bool inSlam = false;
    bool finishSlam = false;
    float knockbackSpeed = 0.0f;
    if (IsValidGuestHeapPtr(fastCtxAddr))
    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        auto it = g_machSpeedStates.find(fastCtxAddr);
        if (it != g_machSpeedStates.end() && it->second.slamActive)
        {
            auto& rt = it->second;
            inSlam = true;
            rt.slamAirborne = false;
            rt.slamElapsed += dt;
            const float t = std::clamp(rt.slamElapsed / kGroundSlamDuration, 0.0f, 1.0f);
            knockbackSpeed = std::lerp(-rt.bounceSpeed, 0.0f, t);

            if (rt.slamElapsed >= kGroundSlamDuration)
            {
                EndSlamLocked(base, fastCtxAddr, rt, true);
                finishSlam = true;
            }
        }
    }

    if (!inSlam)
    {
        __imp__sub_822115F8(ctx, base);
        return;
    }

    if (finishSlam)
    {
        LOGN_UTILITY("[mach_speed] WallSlam end");
        ctx.r3.s64 = 0;
        return;
    }

    WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 1u);
    WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, knockbackSpeed);
    WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
    SetFastAnimation(fastCtxAddr, kFastAnimDamage);

    // Still run ground/fall/gimmick transitions (wall damage is suppressed while slamActive).
    const uint32_t transitioned = GuestToHostFunction<uint32_t>(sub_82211528, g_memory.Translate(stateAddr)) & 0xFFu;
    ctx.r3.s64 = transitioned ? 1 : 0;
}

// FastFall::Update (0x82211730)
// Drives AirSlam knockback while airborne and prevents stock air-speed snap during AirSlam.
PPC_FUNC_IMPL(__imp__sub_82211730);
PPC_FUNC(sub_82211730)
{
    if (!Config::ImprovedMachSpeed)
    {
        __imp__sub_82211730(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t fastCtxAddr = IsValidGuestHeapPtr(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0u;
    const float dt = std::clamp(static_cast<float>(ctx.f1.f64), 0.0f, 0.1f);

    bool inSlam = false;
    float knockbackSpeed = 0.0f;
    if (IsValidGuestHeapPtr(fastCtxAddr))
    {
        std::lock_guard<std::mutex> lock(g_machSpeedMutex);
        auto it = g_machSpeedStates.find(fastCtxAddr);
        if (it != g_machSpeedStates.end() && it->second.slamActive)
        {
            auto& rt = it->second;
            inSlam = true;
            rt.slamAirborne = true;
            rt.slamElapsed += dt;
            const float t = std::clamp(rt.slamElapsed / kAirSlamDuration, 0.0f, 1.0f);
            knockbackSpeed = std::lerp(-rt.bounceSpeed, 0.0f, t);

            if (rt.slamElapsed >= kAirSlamMaxDuration)
            {
                EndSlamLocked(base, fastCtxAddr, rt, false);
                inSlam = false;
                LOGN_UTILITY("[mach_speed] AirSlam timeout end");
            }
        }
    }

    if (inSlam)
    {
        // Temporarily zero stick border so stock FastFall::Update doesn't snap baseSpeed to m_JumpWalk.
        const float savedStickBorder = ReadGuestF32(base, fastCtxAddr + kFastCurrentStickBorderOffset);
        WriteGuestU8(base, fastCtxAddr + kFastFlagLockControlOffset, 1u);
        WriteGuestF32(base, fastCtxAddr + kFastBaseSpeedForwardOffset, knockbackSpeed);
        WriteGuestF32(base, fastCtxAddr + kFastGimmickSpeedForwardOffset, 0.0f);
        WriteGuestF32(base, fastCtxAddr + kFastCurrentStickBorderOffset, 0.0f);
        SetFastAnimation(fastCtxAddr, kFastAnimDamage);

        __imp__sub_82211730(ctx, base);

        WriteGuestF32(base, fastCtxAddr + kFastCurrentStickBorderOffset, savedStickBorder);
        return;
    }

    __imp__sub_82211730(ctx, base);
}
