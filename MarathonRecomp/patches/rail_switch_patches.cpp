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

// P-06-style rail switching for on-foot characters (CommonGrind + PostureControl).
//
// While grinding on a rail (Sonicteam::Player::State::CommonGrind, state 12), tilting the left
// stick laterally leans and turns the character toward that side. Tapping Jump (A) toward an
// adjacent grind rail (MeshSplinePath with collision attribute bit 0x00400000) drives a
// frame-rate-independent parabolic hop with a P-06-style 360-degree sideways corkscrew flip
// using the shared airborne launch animation (ID 75) before catching the target rail back in
// grind pose (ID 23) with speed preserved. Holding Jump or having no reachable target uses the
// game's normal rail jump.

namespace
{
    constexpr uint32_t kMeshSplinePathVftable = 0x8204BE74u;
    constexpr uint32_t kPostureControlVftable = 0x82009048u;
    constexpr uint32_t kRailPluginVftable = 0x82008484u;
    constexpr uint32_t kGrindRailAttrBit = 0x00400000u;
    constexpr uint32_t kPackageModelVftable = 0x8200CB58u;
    constexpr uint32_t kMotionHierarchyVftable = 0x82046DD0u;

    // Body PackageModel -> body model -> bound frame (for camera-independent visual rotation).
    constexpr uint32_t kBodyPackageOffset = 0x30u;
    constexpr uint32_t kPackageBodyModelOffset = 0x3Cu;
    constexpr uint32_t kBodyModelFrameOffset = 0x24u;
    constexpr uint32_t kMotionNodeCountOffset = 0x28u;
    constexpr uint32_t kMotionLocalPoseOffset = 0x2Cu;
    constexpr uint32_t kSonicNodeCount = 99u;
    constexpr uint32_t kRootNodeIndex = 0u;
    constexpr uint32_t kQuatRotationFlag = 0x002u;
    constexpr uint32_t kNodeRotationMask = 0x1C2u;
    constexpr uint32_t kNodeAnyTransformMask = 0xFFFu;
    constexpr float kFlipPivotHeightCm = 48.0f; // Approximate center-of-mass height for clean in-place flips.

    // Shared CommonContext animation IDs verified from CommonGrind (sub_82209620) and
    // CommonJump::Update (sub_8220AE28). Valid across all playable characters.
    constexpr uint32_t kCommonContextAnimFlagOffset = 0x70u;
    constexpr uint32_t kAnimRestartFlag = 0xFFFFFFFEu; // -2
    constexpr uint32_t kGrindAnimId = 23u;
    constexpr uint32_t kRailSwitchAirAnimId = 6u; // Airborne fall/tuck pose (bent knees, open arms).

    // Logical Action A bits from ListenerNormal, forwarded to ICommonContext::m_Buttons.
    constexpr uint32_t kJumpPressed = 1u;
    constexpr uint32_t kJumpHeld = 2u;
    constexpr uint32_t kJumpReleased = 4u;
    constexpr float kRailJumpTapSeconds = 0.12f;

    constexpr uint32_t kPostureWorldInputQuatOffset = 0x2E0u;
    constexpr uint32_t kPostureWorldInputMagOffset = 0x2F0u;
    constexpr uint32_t kPostureRailPluginOffset = 0x340u;

    // Offsets relative to IPosturePlugIn* stored at PostureControl + 0x340 (Path + 0x20).
    constexpr uint32_t kRailPluginDirectionOffset = 0x1Cu;
    constexpr uint32_t kRailPluginSplineCtrlOffset = 0x24u;

    // SplineControl layout (returned by MeshSplinePath 0x1A001 / 0x1A002).
    constexpr uint32_t kSplineCtrlInstanceOffset = 0x08u;
    constexpr uint32_t kSplineCtrlDistanceOffset = 0x18u;
    constexpr uint32_t kSplineCtrlPositionOffset = 0x20u;
    constexpr uint32_t kSplineCtrlUpOffset = 0x30u;
    constexpr uint32_t kSplineCtrlTangentOffset = 0x40u;

    constexpr float kPi = 3.14159265f;
    constexpr float kTwoPi = 6.28318530f;
    constexpr float kMaxRailLeanYawRadians = 0.34906585f;  // 20 degrees torso turn on stick tilt.
    constexpr float kMaxRailLeanRollRadians = 0.38397244f; // 22 degrees body lean on stick tilt.
    constexpr float kRailLeanSmoothRate = 14.0f;
    constexpr float kFlipPeakYawRadians = 0.59341195f;     // 34 degrees yaw toward target rail mid-flip.
    constexpr float kFlipPeakPitchRadians = 0.45378561f;   // 26 degrees forward pitch tuck at apex.
    constexpr float kFlipCatchAnimProgress = 0.78f;        // Blend back to grind pose before landing.
    constexpr float kMinStickMagnitude = 0.35f;
    constexpr float kMinStickLateral = 0.40f;
    constexpr float kMinSideAlignDot = 0.65f;
    constexpr float kMinTangentAlignDot = 0.55f;
    constexpr float kMinLateralDistCm = 55.0f;
    constexpr float kMaxLateralDistCm = 850.0f;
    constexpr float kMinVertDeltaCm = -300.0f;
    constexpr float kMaxVertDeltaCm = 350.0f;
    constexpr float kMaxForwardDeltaCm = 350.0f;
    constexpr float kMinRemainingSplineCm = 80.0f;
    constexpr float kSplineEndMarginCm = 5.0f;

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
    inline float Length(const Vec3& v) { return std::sqrt(LengthSq(v)); }

    inline Vec3 Normalize(const Vec3& v, const Vec3& fallback = { 0.0f, 0.0f, 1.0f })
    {
        const float lenSq = LengthSq(v);
        if (lenSq <= 1e-8f)
            return fallback;

        const float invLen = 1.0f / std::sqrt(lenSq);
        return v * invLen;
    }

    inline Vec3 LoadVec3(const Sonicteam::SoX::Math::Vector& v)
    {
        return { v.X.get(), v.Y.get(), v.Z.get() };
    }

    inline Sonicteam::SoX::Math::Vector StoreVec4(const Vec3& v, float w = 1.0f)
    {
        return Sonicteam::SoX::Math::Vector(v.x, v.y, v.z, w);
    }

    inline Quat LoadQuat(const Sonicteam::SoX::Math::Quaternion& q)
    {
        return { q.X.get(), q.Y.get(), q.Z.get(), q.W.get() };
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

    inline Quat NormalizeQuat(const Quat& q)
    {
        const float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
        if (lenSq <= 1e-8f)
            return { 0.0f, 0.0f, 0.0f, 1.0f };

        const float invLen = 1.0f / std::sqrt(lenSq);
        return { q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen };
    }

    // Matches Xbox 360 XNAMath XMMatrixRotationQuaternion row-vector convention (sub_825D2BC0 / sub_825D2AA8).
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

    inline Quat QuatConj(const Quat& q)
    {
        return { -q.x, -q.y, -q.z, q.w };
    }

    inline Quat QuatFromTo(const Vec3& fromUnit, const Vec3& toUnit)
    {
        const float d = Dot(fromUnit, toUnit);
        if (d >= 0.999999f)
            return { 0.0f, 0.0f, 0.0f, 1.0f };

        if (d <= -0.999999f)
        {
            Vec3 ortho = Cross({ 1.0f, 0.0f, 0.0f }, fromUnit);
            if (LengthSq(ortho) <= 1e-6f)
                ortho = Cross({ 0.0f, 0.0f, 1.0f }, fromUnit);
            ortho = Normalize(ortho, { 1.0f, 0.0f, 0.0f });
            return { ortho.x, ortho.y, ortho.z, 0.0f };
        }

        const Vec3 c = Cross(fromUnit, toUnit);
        return NormalizeQuat({ c.x, c.y, c.z, 1.0f + d });
    }

    Quat BlendRailRotation(const Quat& from, const Quat& to, float t)
    {
        const float dot = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
        const float toWeight = dot < 0.0f ? -t : t;
        const float fromWeight = 1.0f - t;
        return NormalizeQuat({
            from.x * fromWeight + to.x * toWeight,
            from.y * fromWeight + to.y * toWeight,
            from.z * fromWeight + to.z * toWeight,
            from.w * fromWeight + to.w * toWeight
        });
    }

    // Builds an XNAMath row-vector quaternion from orthonormal right (row 0), up (row 1), forward (row 2).
    inline Quat QuatFromBasis(const Vec3& right, const Vec3& up, const Vec3& forward)
    {
        const float m00 = right.x, m01 = right.y, m02 = right.z;
        const float m10 = up.x, m11 = up.y, m12 = up.z;
        const float m20 = forward.x, m21 = forward.y, m22 = forward.z;
        const float trace = m00 + m11 + m22;

        Quat q{};
        if (trace > 0.0f)
        {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m12 - m21) / s;
            q.y = (m20 - m02) / s;
            q.z = (m01 - m10) / s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            const float s = std::sqrt(std::max(1.0f + m00 - m11 - m22, 1e-8f)) * 2.0f;
            q.w = (m12 - m21) / s;
            q.x = 0.25f * s;
            q.y = (m01 + m10) / s;
            q.z = (m20 + m02) / s;
        }
        else if (m11 > m22)
        {
            const float s = std::sqrt(std::max(1.0f + m11 - m00 - m22, 1e-8f)) * 2.0f;
            q.w = (m20 - m02) / s;
            q.x = (m01 + m10) / s;
            q.y = 0.25f * s;
            q.z = (m12 + m21) / s;
        }
        else
        {
            const float s = std::sqrt(std::max(1.0f + m22 - m00 - m11, 1e-8f)) * 2.0f;
            q.w = (m01 - m10) / s;
            q.x = (m20 + m02) / s;
            q.y = (m12 + m21) / s;
            q.z = 0.25f * s;
        }

        return NormalizeQuat(q);
    }

    inline void MakeOrthonormalRailFrame(const Vec3& rawTangent, float dirSign, const Vec3& rawUp, Vec3& outRight, Vec3& outUp, Vec3& outForward)
    {
        outForward = Normalize(rawTangent * dirSign, { 0.0f, 0.0f, 1.0f });
        Vec3 upCand = Normalize(rawUp, { 0.0f, 1.0f, 0.0f });
        if (std::fabs(Dot(upCand, outForward)) > 0.99f)
            upCand = (std::fabs(outForward.y) < 0.9f) ? Vec3{ 0.0f, 1.0f, 0.0f } : Vec3{ 1.0f, 0.0f, 0.0f };

        outRight = Normalize(Cross(upCand, outForward), { 1.0f, 0.0f, 0.0f });
        outUp = Normalize(Cross(outForward, outRight), { 0.0f, 1.0f, 0.0f });
    }

    inline Quat QuatAxisAngleX(float radians)
    {
        const float half = 0.5f * radians;
        return { std::sin(half), 0.0f, 0.0f, std::cos(half) };
    }

    inline Quat QuatAxisAngleY(float radians)
    {
        const float half = 0.5f * radians;
        return { 0.0f, std::sin(half), 0.0f, std::cos(half) };
    }

    inline Quat QuatAxisAngleZ(float radians)
    {
        const float half = 0.5f * radians;
        return { 0.0f, 0.0f, std::sin(half), std::cos(half) };
    }

    struct RailSwitchArc
    {
        uint32_t contextAddr{};
        uint32_t targetMeshRailAddr{};
        uint32_t targetSplineCtrlAddr{};
        uint32_t bodyFrameAddr{};
        float targetDistance{};
        float targetTotalLen{};
        float targetDirection{ 1.0f };
        float savedSpeed{};
        float duration{ 0.28f };
        float elapsed{};
        float arcHeight{ 100.0f };
        float localOffsetX{};
        float localOffsetY{};
        float localOffsetZ{};
        float startLean{};
        Quat startRotation{};
        bool switchLeft{};
        bool landingAnimRestored{};
    };

    struct alignas(16) MotionLocalPose
    {
        Sonicteam::SoX::Math::Vector translation;
        Sonicteam::SoX::Math::Quaternion rotation;
        Sonicteam::SoX::Math::Vector scale;
        be<uint32_t> componentFlags;
        uint8_t padding[12];
    };
    static_assert(sizeof(MotionLocalPose) == 64);
    static_assert(offsetof(MotionLocalPose, rotation) == 16);
    static_assert(offsetof(MotionLocalPose, componentFlags) == 48);

    struct VisualPoseSample
    {
        uint32_t localPoseAddr{};
        uint32_t nodeCount{};
        Quat rootRotation{};
        float pivotY{};
        float flipProgress{};
        bool isFlip{};
        bool switchLeft{};
    };

    // Expressive acrobatic tuck for Sonic (99-node skeleton) layered over the airborne clip
    // during the 360-degree corkscrew flip. Smoothly returns to 0 at takeoff and landing.
    struct SonicFlipJoint
    {
        uint32_t leftNode;
        uint32_t rightNode;
        Vec3 localAxis;
        float rightSign;
        float peakDegrees;
    };

    constexpr SonicFlipJoint kSonicFlipLimbs[] = {
        {  3,  3, { 0, 0, 1 },  1.0f,  24.0f }, // Spine curls forward into somersault tuck.
        {  4,  4, { 1, 0, 0 }, -1.0f,  28.0f }, // Chest twists toward target rail.
        { 36, 58, { 0, 0, 1 }, -1.0f,  42.0f }, // Leading arm sweeps out for balance.
        { 58, 36, { 0, 0, 1 }, -1.0f, -26.0f }, // Trailing arm tucks in.
        { 37, 59, { 0, 1, 0 },  1.0f, -35.0f }, // Leading elbow bends cleanly.
        { 59, 37, { 0, 1, 0 },  1.0f, -45.0f }, // Trailing elbow tucks tight.
        { 87, 81, { 0, 1, 0 },  1.0f, -38.0f }, // Leading thigh pulls up into tuck.
        { 81, 87, { 0, 1, 0 },  1.0f, -46.0f }, // Trailing thigh tucks tighter.
        { 88, 82, { 0, 1, 0 },  1.0f, -52.0f }, // Leading knee bends into flip.
        { 82, 88, { 0, 1, 0 },  1.0f, -62.0f }, // Trailing knee tucks tight.
    };

    Quat ComputeFlipLocalRotation(float u, bool switchLeft, float startLean)
    {
        const float smoothU = u * u * (3.0f - 2.0f * u);
        const float dirSign = switchLeft ? -1.0f : 1.0f; // +1 = right, -1 = left.
        const float arch = std::sin(kPi * u);
        const float startLeanYaw = startLean * kMaxRailLeanYawRadians;
        const float startLeanRoll = -startLean * kMaxRailLeanRollRadians;

        const float yawAngle = startLeanYaw * (1.0f - smoothU) + dirSign * kFlipPeakYawRadians * arch;
        const float rollAngle = startLeanRoll * (1.0f - smoothU) - dirSign * kTwoPi * smoothU;
        const float pitchAngle = kFlipPeakPitchRadians * arch;

        return NormalizeQuat(QuatMul(QuatAxisAngleY(yawAngle),
            QuatMul(QuatAxisAngleZ(rollAngle), QuatAxisAngleX(pitchAngle))));
    }

    Quat ComputeLeanLocalRotation(float lean)
    {
        return NormalizeQuat(QuatMul(QuatAxisAngleY(lean * kMaxRailLeanYawRadians),
            QuatAxisAngleZ(-lean * kMaxRailLeanRollRadians)));
    }

    bool ApplyVisualNodePose(MotionLocalPose& pose, uint32_t node, const VisualPoseSample& sample)
    {
        if (node == kRootNodeIndex)
        {
            const uint32_t flags = pose.componentFlags.get();
            const Vec3 origT = (flags & kNodeAnyTransformMask) != 0 ? LoadVec3(pose.translation) : Vec3{ 0.0f, 0.0f, 0.0f };
            const Quat origR = (flags & kNodeRotationMask) != 0 ? LoadQuat(pose.rotation) : Quat{ 0.0f, 0.0f, 0.0f, 1.0f };

            pose.rotation = StoreQuat(NormalizeQuat(QuatMul(sample.rootRotation, origR)));
            if (sample.pivotY > 0.0f)
            {
                const Vec3 pivot{ 0.0f, sample.pivotY, 0.0f };
                const Vec3 shifted = origT + (pivot - QuatRotate(sample.rootRotation, pivot));
                pose.translation = StoreVec4(shifted, 1.0f);
            }
            else if ((flags & kNodeAnyTransformMask) == 0)
            {
                pose.translation = StoreVec4(origT, 1.0f);
            }
            pose.componentFlags = flags | kQuatRotationFlag;
            return true;
        }

        if (!sample.isFlip || sample.nodeCount != kSonicNodeCount)
            return false;

        for (const auto& joint : kSonicFlipLimbs)
        {
            if (node != (sample.switchLeft ? joint.leftNode : joint.rightNode))
                continue;
            if ((pose.componentFlags.get() & kNodeRotationMask) == 0)
                return false;

            const float arch = std::sin(kPi * sample.flipProgress);
            constexpr float degToHalfRad = 0.00872664626f;
            const float halfAngle = joint.peakDegrees * arch * degToHalfRad *
                (sample.switchLeft ? 1.0f : joint.rightSign);
            const Vec3 axisSin = joint.localAxis * std::sin(halfAngle);
            const Quat delta{ axisSin.x, axisSin.y, axisSin.z, std::cos(halfAngle) };
            pose.rotation = StoreQuat(NormalizeQuat(QuatMul(LoadQuat(pose.rotation), delta)));
            return true;
        }

        return false;
    }

    std::mutex g_railSwitchMutex;
    std::unordered_set<uint32_t> g_grindRails;
    std::unordered_map<uint32_t, RailSwitchArc> g_activeArcs; // key = PostureControl guest addr
    std::unordered_map<uint32_t, float> g_pendingRailJumps; // CommonContext guest addr -> elapsed seconds
    std::unordered_map<uint32_t, float> g_railLeans; // PostureControl guest addr -> smoothed lateral lean [-1, 1]
    std::unordered_map<uint32_t, uint32_t> g_postureBodyFrames; // PostureControl guest addr -> body frame guest addr
    std::unordered_map<uint32_t, float> g_grindBodyLeans; // body frame guest addr -> current visual lean [-1, 1]
    thread_local bool g_inRailSwitchHook = false;
    thread_local VisualPoseSample g_visualPoseSample;

    constexpr bool IsGuestHeapAddress(uint32_t addr)
    {
        // Both game objects and guest stacks live in Heap::Init's virtual heap.
        return addr >= 0x20000u && addr < 0x7FEA0000u && (addr & 3u) == 0;
    }

    inline uint32_t ReadGuestU32(uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<uint32_t>*>(base + addr)->get();
    }

    inline float ReadGuestF32(uint8_t* base, uint32_t addr)
    {
        return reinterpret_cast<const be<float>*>(base + addr)->get();
    }

    inline Vec3 ReadGuestVec3(uint8_t* base, uint32_t addr)
    {
        return LoadVec3(*reinterpret_cast<const Sonicteam::SoX::Math::Vector*>(base + addr));
    }

    inline Quat ReadGuestQuat(uint8_t* base, uint32_t addr)
    {
        return LoadQuat(*reinterpret_cast<const Sonicteam::SoX::Math::Quaternion*>(base + addr));
    }

    inline void ReleaseRefCountObject(uint32_t guestAddr)
    {
        if (guestAddr != 0)
            GuestToHostFunction<void>(sub_82581E38, g_memory.Translate(guestAddr));
    }

    inline float GetSplineTotalLength(uint32_t splineCtrlAddr)
    {
        if (splineCtrlAddr == 0)
            return 0.0f;

        return GuestToHostFunction<float>(sub_82618718, g_memory.Translate(splineCtrlAddr));
    }

    bool EvaluateSplineAt(uint32_t splineCtrlAddr, float distance)
    {
        auto* pCtrl = g_memory.Translate(splineCtrlAddr);
        if (!GuestToHostFunction<bool>(sub_826188A8, pCtrl, distance))
            return false;

        GuestToHostFunction<void>(sub_82618998, pCtrl);
        return true;
    }

    uint32_t GetRailPluginAddr(uint8_t* base, uint32_t postureAddr)
    {
        const uint32_t addr = ReadGuestU32(base, postureAddr + kPostureRailPluginOffset);
        return IsGuestHeapAddress(addr) && ReadGuestU32(base, addr) == kRailPluginVftable ? addr : 0;
    }

    inline uint32_t QueryClosestSplineCtrl(uint32_t meshRailAddr, const Vec3& worldPos)
    {
        guest_stack_var<be<uint32_t>> outSplineCtrl{};
        *outSplineCtrl.get() = 0;

        guest_stack_var<Sonicteam::SoX::Math::Vector> queryPos(StoreVec4(worldPos, 1.0f));
        GuestToHostFunction<void>(sub_825FF190, outSplineCtrl.get(), g_memory.Translate(meshRailAddr), queryPos.get());
        return outSplineCtrl->get();
    }

    uint32_t ResolvePostureAddrFromContext(uint8_t* base, uint32_t contextAddr)
    {
        if (!IsGuestHeapAddress(contextAddr))
            return 0;

        auto* pContext = reinterpret_cast<Sonicteam::Player::State::CommonContext*>(base + contextAddr);
        auto* pScore = pContext->m_spScore.get();
        if (!pScore)
            return 0;

        auto* pPlayer = pScore->m_pPlayer.get();
        if (!pPlayer)
            return 0;

        auto* pPosture = pPlayer->m_spPostureControl.get();
        if (!pPosture)
            return 0;

        const uint32_t postureAddr = g_memory.MapVirtual(pPosture);
        if (!IsGuestHeapAddress(postureAddr))
            return 0;

        // Only handle on-foot PostureControl (skip PostureControlSnowBoard 0x82008EF0).
        if (ReadGuestU32(base, postureAddr) != kPostureControlVftable)
            return 0;

        return postureAddr;
    }

    uint32_t ResolvePlayerBodyFrame(uint8_t* base, Sonicteam::Player::State::CommonContext* pContext)
    {
        auto* pScore = pContext->m_spScore.get();
        auto* pPlayer = pScore ? pScore->m_pPlayer.get() : nullptr;
        if (!pPlayer)
            return 0;

        auto* pModel = pPlayer->m_spModel.get();
        if (!pModel)
            return 0;
        const uint32_t modelAddr = g_memory.MapVirtual(pModel);
        if (!IsGuestHeapAddress(modelAddr))
            return 0;

        const uint32_t packageAddr = ReadGuestU32(base, modelAddr + kBodyPackageOffset);
        if (!IsGuestHeapAddress(packageAddr) || ReadGuestU32(base, packageAddr) != kPackageModelVftable)
            return 0;
        const uint32_t bodyAddr = ReadGuestU32(base, packageAddr + kPackageBodyModelOffset);
        if (!IsGuestHeapAddress(bodyAddr))
            return 0;
        const uint32_t frameAddr = ReadGuestU32(base, bodyAddr + kBodyModelFrameOffset);
        return IsGuestHeapAddress(frameAddr) ? frameAddr : 0;
    }

    inline void SetContextAnimation(uint8_t* base, uint32_t contextAddr, uint32_t animId)
    {
        if (!IsGuestHeapAddress(contextAddr))
            return;

        auto* pContext = reinterpret_cast<Sonicteam::Player::State::CommonContext*>(base + contextAddr);
        pContext->m_AnimationID = animId;
        *reinterpret_cast<be<uint32_t>*>(base + contextAddr + kCommonContextAnimFlagOffset) = kAnimRestartFlag;
    }

    bool CancelArcForContext(uint32_t contextAddr)
    {
        std::lock_guard lock(g_railSwitchMutex);
        g_pendingRailJumps.erase(contextAddr);
        const auto it = std::find_if(g_activeArcs.begin(), g_activeArcs.end(),
            [contextAddr](const auto& entry) { return entry.second.contextAddr == contextAddr; });
        if (it == g_activeArcs.end())
            return false;

        g_railLeans.erase(it->first);
        if (it->second.bodyFrameAddr != 0)
            g_grindBodyLeans.erase(it->second.bodyFrameAddr);
        ReleaseRefCountObject(it->second.targetSplineCtrlAddr);
        g_activeArcs.erase(it);
        return true;
    }

    inline bool HasActiveArc(uint32_t postureAddr)
    {
        std::lock_guard lock(g_railSwitchMutex);
        return g_activeArcs.find(postureAddr) != g_activeArcs.end();
    }

    enum class PendingRailJump { None, Waiting, Tap, Normal };

    PendingRailJump UpdatePendingRailJump(uint32_t contextAddr, bool eligible, uint32_t buttons, float dt)
    {
        std::lock_guard lock(g_railSwitchMutex);
        const auto it = g_pendingRailJumps.find(contextAddr);
        if (it == g_pendingRailJumps.end())
            return PendingRailJump::None;

        if (!eligible)
        {
            g_pendingRailJumps.erase(it);
            return PendingRailJump::None;
        }

        it->second += dt;
        const bool holdElapsed = !std::isfinite(dt) || dt < 0.0f || it->second >= kRailJumpTapSeconds;
        const bool released = (buttons & kJumpReleased) != 0 || (buttons & kJumpHeld) == 0;
        if (!holdElapsed && !released)
            return PendingRailJump::Waiting;

        g_pendingRailJumps.erase(it);
        return holdElapsed ? PendingRailJump::Normal : PendingRailJump::Tap;
    }

    enum class RailSwitchRequest { Check, Start };

    bool TryRailSwitch(uint8_t* base, uint32_t contextAddr, uint32_t postureAddr, RailSwitchRequest request)
    {
        auto* pContext = reinterpret_cast<Sonicteam::Player::State::CommonContext*>(base + contextAddr);
        auto* pPosture = reinterpret_cast<Sonicteam::Player::IPostureControl*>(base + postureAddr);

        // Must have stick pushed noticeably sideways relative to the current rail.
        const float stickMag = ReadGuestF32(base, postureAddr + kPostureWorldInputMagOffset);
        if (!std::isfinite(stickMag) || stickMag < kMinStickMagnitude)
            return false;

        const uint32_t railPluginAddr = GetRailPluginAddr(base, postureAddr);
        if (railPluginAddr == 0)
        {
            LOG("[rail-switch] No grind Path plugin for the current posture");
            return false;
        }

        const uint32_t currSplineCtrlAddr = ReadGuestU32(base, railPluginAddr + kRailPluginSplineCtrlOffset);
        if (!IsGuestHeapAddress(currSplineCtrlAddr))
        {
            LOG("[rail-switch] Current Path plugin has no spline");
            return false;
        }

        const uint32_t currInstanceAddr = ReadGuestU32(base, currSplineCtrlAddr + kSplineCtrlInstanceOffset);
        if (currInstanceAddr == 0)
            return false;

        const float currDir = ReadGuestF32(base, railPluginAddr + kRailPluginDirectionOffset) < 0.0f ? -1.0f : 1.0f;
        const Vec3 currRawUp = ReadGuestVec3(base, currSplineCtrlAddr + kSplineCtrlUpOffset);
        const Vec3 currRawTangent = ReadGuestVec3(base, currSplineCtrlAddr + kSplineCtrlTangentOffset);
        Vec3 currRight{}, currUp{}, currForward{};
        MakeOrthonormalRailFrame(currRawTangent, currDir, currRawUp, currRight, currUp, currForward);

        const Quat inputQuat = ReadGuestQuat(base, postureAddr + kPostureWorldInputQuatOffset);
        const Vec3 worldStickDir = Normalize(QuatRotate(inputQuat, { 0.0f, 0.0f, 1.0f }), { 0.0f, 0.0f, 0.0f });
        const Vec3 stickLat = worldStickDir - currForward * Dot(worldStickDir, currForward);
        const Vec3 stickSide = stickLat - currUp * Dot(stickLat, currUp);
        const float stickSideLen = Length(stickSide);
        if (!std::isfinite(stickSideLen) || stickSideLen < kMinStickLateral)
            return false;

        const Vec3 desiredSideDir = stickSide * (1.0f / stickSideLen);
        const Vec3 playerPos = LoadVec3(pPosture->m_Position);

        const float savedSpeed = pContext->m_BaseSpeedForward.get();
        if (!std::isfinite(savedSpeed) || savedSpeed < 0.0f)
            return false;

        // The rail destructor takes this lock before releasing its spline instance.
        std::unique_lock lock(g_railSwitchMutex);
        const size_t registeredRailCount = g_grindRails.size();
        size_t queriedSplines = 0;

        uint32_t bestMeshRailAddr = 0;
        uint32_t bestSplineCtrlAddr = 0;
        float bestScore = 1e30f;
        float bestDist = 0.0f;
        float bestTotalLen = 0.0f;
        float bestDir = 1.0f;
        float bestLatDist = 0.0f;
        float bestVertDelta = 0.0f;
        float bestDuration = 0.0f;

        for (uint32_t railAddr : g_grindRails)
        {
            if (!IsGuestHeapAddress(railAddr))
                continue;

            if (ReadGuestU32(base, railAddr) != kMeshSplinePathVftable)
                continue;

            const uint32_t railInstanceAddr = ReadGuestU32(base, railAddr + 0x0Cu);
            if (railInstanceAddr == 0 || railInstanceAddr == currInstanceAddr)
                continue;

            const uint32_t ctrlAddr = QueryClosestSplineCtrl(railAddr, playerPos);
            if (!IsGuestHeapAddress(ctrlAddr))
                continue;

            ++queriedSplines;
            const uint32_t ctrlInstanceAddr = ReadGuestU32(base, ctrlAddr + kSplineCtrlInstanceOffset);
            if (ctrlInstanceAddr == 0 || ctrlInstanceAddr == currInstanceAddr)
            {
                ReleaseRefCountObject(ctrlAddr);
                continue;
            }

            const Vec3 candPos = ReadGuestVec3(base, ctrlAddr + kSplineCtrlPositionOffset);
            const Vec3 candTangent = Normalize(ReadGuestVec3(base, ctrlAddr + kSplineCtrlTangentOffset), currForward);
            const float candDist = ReadGuestF32(base, ctrlAddr + kSplineCtrlDistanceOffset);
            const float candTotalLen = GetSplineTotalLength(ctrlAddr);

            const Vec3 delta = candPos - playerPos;
            const float fwdDelta = Dot(delta, currForward);
            const float vertDelta = Dot(delta, currUp);
            const Vec3 horizDelta = delta - currForward * fwdDelta - currUp * vertDelta;
            const float latDist = Length(horizDelta);

            const float tanDot = Dot(currForward, candTangent);
            const float candDir = (tanDot >= 0.0f) ? 1.0f : -1.0f;
            const float remaining = (candDir > 0.0f) ? (candTotalLen - candDist) : candDist;
            const float duration = std::clamp(0.25f + (latDist / kMaxLateralDistCm) * 0.11f, 0.25f, 0.36f);

            bool valid = std::isfinite(candTotalLen) && candDist >= 0.0f && candDist <= candTotalLen
                && (latDist >= kMinLateralDistCm && latDist <= kMaxLateralDistCm)
                && (vertDelta >= kMinVertDeltaCm && vertDelta <= kMaxVertDeltaCm)
                && (std::fabs(fwdDelta) <= kMaxForwardDeltaCm)
                && (std::fabs(tanDot) >= kMinTangentAlignDot)
                && (remaining >= std::max(kMinRemainingSplineCm, savedSpeed * duration + kSplineEndMarginCm));

            float sideAlign = 0.0f;
            if (valid)
            {
                sideAlign = Dot(horizDelta * (1.0f / latDist), desiredSideDir);
                valid = (sideAlign >= kMinSideAlignDot);
            }

            if (!valid)
            {
                ReleaseRefCountObject(ctrlAddr);
                continue;
            }

            if (request == RailSwitchRequest::Check)
            {
                ReleaseRefCountObject(ctrlAddr);
                return true;
            }

            const float score = latDist + std::fabs(vertDelta) * 0.35f + (1.0f - sideAlign) * 150.0f;
            if (score < bestScore)
            {
                if (bestSplineCtrlAddr != 0)
                    ReleaseRefCountObject(bestSplineCtrlAddr);

                bestScore = score;
                bestMeshRailAddr = railAddr;
                bestSplineCtrlAddr = ctrlAddr;
                bestDist = candDist;
                bestTotalLen = candTotalLen;
                bestDir = candDir;
                bestLatDist = latDist;
                bestVertDelta = vertDelta;
                bestDuration = duration;
            }
            else
            {
                ReleaseRefCountObject(ctrlAddr);
            }
        }

        if (bestSplineCtrlAddr == 0)
        {
            lock.unlock();
            LOGF("[rail-switch] No reachable rail (registered={}, queried={})", registeredRailCount, queriedSplines);
            return false;
        }

        // Compute initial player offset in the target rail's local orthonormal frame so curves are tracked cleanly.
        const Vec3 targetPos0 = ReadGuestVec3(base, bestSplineCtrlAddr + kSplineCtrlPositionOffset);
        const Vec3 targetUp0 = ReadGuestVec3(base, bestSplineCtrlAddr + kSplineCtrlUpOffset);
        const Vec3 targetTan0 = ReadGuestVec3(base, bestSplineCtrlAddr + kSplineCtrlTangentOffset);
        Vec3 r0{}, u0{}, f0{};
        MakeOrthonormalRailFrame(targetTan0, bestDir, targetUp0, r0, u0, f0);

        const Vec3 offset0 = playerPos - targetPos0;
        const float localX = Dot(offset0, r0);
        const float localY = Dot(offset0, u0);
        const float localZ = Dot(offset0, f0);

        const float arcHeight = std::clamp(75.0f + bestLatDist * 0.10f + std::max(0.0f, bestVertDelta) * 0.25f, 75.0f, 150.0f);
        const uint32_t bodyFrameAddr = ResolvePlayerBodyFrame(base, pContext);

        float startLean = 0.0f;
        if (auto leanIt = g_railLeans.find(postureAddr); leanIt != g_railLeans.end())
        {
            startLean = leanIt->second;
            g_railLeans.erase(leanIt);
        }
        if (bodyFrameAddr != 0)
            g_grindBodyLeans.erase(bodyFrameAddr);

        RailSwitchArc arc{};
        arc.contextAddr = contextAddr;
        arc.targetMeshRailAddr = bestMeshRailAddr;
        arc.targetSplineCtrlAddr = bestSplineCtrlAddr;
        arc.bodyFrameAddr = bodyFrameAddr;
        arc.targetDistance = bestDist;
        arc.targetTotalLen = bestTotalLen;
        arc.targetDirection = bestDir;
        arc.savedSpeed = savedSpeed;
        arc.duration = bestDuration;
        arc.elapsed = 0.0f;
        arc.arcHeight = arcHeight;
        arc.localOffsetX = localX;
        arc.localOffsetY = localY;
        arc.localOffsetZ = localZ;
        arc.startLean = std::clamp(startLean, -1.0f, 1.0f);
        // Use the unleaned current rail basis so camera heading transitions smoothly.
        arc.startRotation = QuatFromBasis(currRight, currUp, currForward);
        arc.switchLeft = Dot(desiredSideDir, currRight) < 0.0f;
        arc.landingAnimRestored = false;

        if (!g_activeArcs.try_emplace(postureAddr, arc).second)
        {
            ReleaseRefCountObject(bestSplineCtrlAddr);
            return false;
        }

        lock.unlock();
        SetContextAnimation(base, contextAddr, kRailSwitchAirAnimId);
        LOGF("[rail-switch] Started (side={}, distance={:.0f}, speed={:.0f}, duration={:.3f}, lean={:.2f}) anim={} bodyFrame={}",
            arc.switchLeft ? "left" : "right", bestLatDist, savedSpeed, bestDuration,
            arc.startLean, pContext->m_AnimationID.get(), bodyFrameAddr != 0);
        return true;
    }

    void ApplyPostureTransform(uint8_t* base, uint32_t postureAddr, const Vec3& pos, const Vec3& railUp, const Quat& worldRot)
    {
        auto* pPosture = reinterpret_cast<Sonicteam::Player::IPostureControl*>(base + postureAddr);
        const Quat surfaceQuat = QuatFromTo({ 0.0f, 1.0f, 0.0f }, Normalize(railUp, { 0.0f, 1.0f, 0.0f }));
        // Match the quaternion composition used by the original posture update.
        guest_stack_var<Sonicteam::SoX::Math::Quaternion> inverseSurfaceVar(StoreQuat(QuatConj(surfaceQuat)));
        guest_stack_var<Sonicteam::SoX::Math::Quaternion> rotVar(StoreQuat(worldRot));
        GuestToHostFunction<void>(sub_825D1D68, &pPosture->m_Rotation, inverseSurfaceVar.get(), rotVar.get());

        pPosture->m_Position = StoreVec4(pos, 1.0f);
        pPosture->m_SurfaceNormal = Sonicteam::SoX::Math::Vector(surfaceQuat.x, surfaceQuat.y, surfaceQuat.z, surfaceQuat.w);
        GuestToHostFunction<void>(sub_821F1968, pPosture, &pPosture->m_Position, rotVar.get());
    }

    void MaintainGrindMotion(Sonicteam::Player::IPostureControl* pPosture,
        Sonicteam::Player::State::CommonContext* pContext, float speed)
    {
        // Path normally reports Grounded | RailGrind. Even during the hop,
        // CommonGrind's transition check falls out of grind if Grounded is clear.
        using namespace Sonicteam::Player;
        const uint32_t flags = (pPosture->m_PostureFlag.get() & ~static_cast<uint32_t>(
            PostureControlFlags_FallIntermediate | PostureControlFlags_Fall))
            | PostureControlFlags_Grounded | PostureControlFlags_RailGrind;
        pPosture->m_PostureFlag = flags;
        pContext->m_PostureFlags = static_cast<PostureControlFlags>(flags);
        pPosture->m_ImpulseForward = speed;
        pContext->m_BaseSpeedForward = speed;
    }

    bool StepActiveRailSwitch(uint8_t* base, uint32_t postureAddr, float dt)
    {
        // Do not let MeshSplinePath destruction invalidate the spline during this frame.
        std::unique_lock lock(g_railSwitchMutex);
        auto it = g_activeArcs.find(postureAddr);
        if (it == g_activeArcs.end())
            return false;

        auto& arc = it->second;
        const auto finishArc = [&] {
            ReleaseRefCountObject(arc.targetSplineCtrlAddr);
            g_activeArcs.erase(it);
            lock.unlock();
        };

        if (!Config::RailSwitching || !std::isfinite(dt))
        {
            const uint32_t ctxAddr = arc.contextAddr;
            finishArc();
            SetContextAnimation(base, ctxAddr, kGrindAnimId);
            return false;
        }

        const float step = std::clamp(dt, 0.0f, arc.duration - arc.elapsed);
        arc.elapsed = std::min(arc.elapsed + step, arc.duration);
        arc.targetDistance += arc.savedSpeed * step * arc.targetDirection;

        const bool reachedSplineEnd = arc.targetDirection > 0.0f
            ? arc.targetDistance >= arc.targetTotalLen
            : arc.targetDistance <= 0.0f;
        if (reachedSplineEnd || !EvaluateSplineAt(arc.targetSplineCtrlAddr, arc.targetDistance))
        {
            const uint32_t ctxAddr = arc.contextAddr;
            finishArc();
            SetContextAnimation(base, ctxAddr, kGrindAnimId);
            LOG("[rail-switch] Cancelled: target spline ended or became invalid");
            return false;
        }

        auto* pPosture = reinterpret_cast<Sonicteam::Player::IPostureControl*>(base + postureAddr);
        auto* pContext = reinterpret_cast<Sonicteam::Player::State::CommonContext*>(base + arc.contextAddr);

        const Vec3 targetPos = ReadGuestVec3(base, arc.targetSplineCtrlAddr + kSplineCtrlPositionOffset);
        const Vec3 targetRawUp = ReadGuestVec3(base, arc.targetSplineCtrlAddr + kSplineCtrlUpOffset);
        const Vec3 targetRawTan = ReadGuestVec3(base, arc.targetSplineCtrlAddr + kSplineCtrlTangentOffset);

        Vec3 rt{}, ut{}, ft{};
        MakeOrthonormalRailFrame(targetRawTan, arc.targetDirection, targetRawUp, rt, ut, ft);
        const Quat baseQuat = QuatFromBasis(rt, ut, ft);

        const float u = std::clamp(arc.elapsed / arc.duration, 0.0f, 1.0f);
        if (u < 1.0f)
        {
            const float oneMinusU = 1.0f - u;
            const float smoothU = u * u * (3.0f - 2.0f * u);
            // Blend linear and smoothstep for a punchy takeoff and clean rail catch.
            const float lateralRemain = 0.5f * oneMinusU + 0.5f * (1.0f - smoothU);
            const Vec3 localOffsetWorld = (rt * arc.localOffsetX + ut * arc.localOffsetY + ft * arc.localOffsetZ) * lateralRemain;
            const Vec3 arcLift = ut * (4.0f * arc.arcHeight * u * oneMinusU);
            const Vec3 worldPos = targetPos + localOffsetWorld + arcLift;

            // Keep PostureControl rotation smoothly aligned to the rail heading so the camera
            // glides without rolling or twitching. The 360-degree corkscrew flip is applied
            // strictly to the character's skeleton root in MotionHierarchy::Apply (sub_825991F0).
            const Quat heading = BlendRailRotation(arc.startRotation, baseQuat, smoothU);
            ApplyPostureTransform(base, postureAddr, worldPos, ut, heading);

            if (u >= kFlipCatchAnimProgress && !arc.landingAnimRestored)
            {
                SetContextAnimation(base, arc.contextAddr, kGrindAnimId);
                arc.landingAnimRestored = true;
            }

            MaintainGrindMotion(pPosture, pContext, arc.savedSpeed);

            return true;
        }

        // Path::Attach retains its own reference to the spline control on landing.
        const uint32_t railPluginAddr = GetRailPluginAddr(base, postureAddr);
        if (railPluginAddr == 0)
        {
            const uint32_t ctxAddr = arc.contextAddr;
            finishArc();
            SetContextAnimation(base, ctxAddr, kGrindAnimId);
            LOG("[rail-switch] Cancelled: grind Path plugin is no longer present");
            return false;
        }

        guest_stack_var<be<uint32_t>> splineCtrlPtrVar{};
        *splineCtrlPtrVar.get() = arc.targetSplineCtrlAddr;
        guest_stack_var<Sonicteam::SoX::Math::Quaternion> landQuatVar(StoreQuat(baseQuat));
        if (!GuestToHostFunction<bool>(sub_821EF320, g_memory.Translate(railPluginAddr), splineCtrlPtrVar.get(), landQuatVar.get()))
        {
            const uint32_t ctxAddr = arc.contextAddr;
            finishArc();
            SetContextAnimation(base, ctxAddr, kGrindAnimId);
            LOG("[rail-switch] Cancelled: Path rejected the target spline");
            return false;
        }

        *reinterpret_cast<be<float>*>(base + railPluginAddr + kRailPluginDirectionOffset) = arc.targetDirection;
        ApplyPostureTransform(base, postureAddr, targetPos, ut, baseQuat);
        if (!arc.landingAnimRestored)
            SetContextAnimation(base, arc.contextAddr, kGrindAnimId);
        MaintainGrindMotion(pPosture, pContext, arc.savedSpeed);

        const float landingSpeed = pContext->m_BaseSpeedForward.get();
        const uint32_t landingAnim = pContext->m_AnimationID.get();
        finishArc();
        LOGF("[rail-switch] Landed on target rail (speed={:.0f}) anim={}",
            landingSpeed, landingAnim);
        return true;
    }

    // Smooth P-06-style torso turn and body lean toward the stick direction while grinding.
    // Applied strictly via the character's skeleton root in MotionHierarchy::Apply so the
    // camera never twitches when tilting the stick.
    void UpdateGrindRailLean(uint8_t* base, uint32_t postureAddr, float dt)
    {
        const auto clearLean = [postureAddr] {
            std::lock_guard lock(g_railSwitchMutex);
            g_railLeans.erase(postureAddr);
            if (auto bfIt = g_postureBodyFrames.find(postureAddr); bfIt != g_postureBodyFrames.end())
                g_grindBodyLeans.erase(bfIt->second);
        };

        if (!Config::RailSwitching || !std::isfinite(dt) || dt < 0.0f || !IsGuestHeapAddress(postureAddr) ||
            ReadGuestU32(base, postureAddr) != kPostureControlVftable)
        {
            clearLean();
            return;
        }

        auto* pPosture = reinterpret_cast<Sonicteam::Player::IPostureControl*>(base + postureAddr);
        const bool onRail = (pPosture->m_PostureFlag.get() & static_cast<uint32_t>(Sonicteam::Player::PostureControlFlags_RailGrind)) != 0;
        const uint32_t railPluginAddr = onRail ? GetRailPluginAddr(base, postureAddr) : 0;
        if (railPluginAddr == 0)
        {
            clearLean();
            return;
        }

        const uint32_t currSplineCtrlAddr = ReadGuestU32(base, railPluginAddr + kRailPluginSplineCtrlOffset);
        if (!IsGuestHeapAddress(currSplineCtrlAddr))
            return;

        const float currDir = ReadGuestF32(base, railPluginAddr + kRailPluginDirectionOffset) < 0.0f ? -1.0f : 1.0f;
        const Vec3 currRawUp = ReadGuestVec3(base, currSplineCtrlAddr + kSplineCtrlUpOffset);
        const Vec3 currRawTangent = ReadGuestVec3(base, currSplineCtrlAddr + kSplineCtrlTangentOffset);
        Vec3 currRight{}, currUp{}, currForward{};
        MakeOrthonormalRailFrame(currRawTangent, currDir, currRawUp, currRight, currUp, currForward);

        float targetLean = 0.0f;
        const float stickMag = ReadGuestF32(base, postureAddr + kPostureWorldInputMagOffset);
        if (std::isfinite(stickMag) && stickMag >= 0.15f)
        {
            const Quat inputQuat = ReadGuestQuat(base, postureAddr + kPostureWorldInputQuatOffset);
            const Vec3 worldStickDir = Normalize(QuatRotate(inputQuat, { 0.0f, 0.0f, 1.0f }), { 0.0f, 0.0f, 0.0f });
            const float lateral = Dot(worldStickDir, currRight);
            if (std::isfinite(lateral))
                targetLean = std::clamp(lateral * std::min(stickMag, 1.0f), -1.0f, 1.0f);
        }

        std::lock_guard lock(g_railSwitchMutex);
        const auto bfIt = g_postureBodyFrames.find(postureAddr);
        const uint32_t bodyFrameAddr = (bfIt != g_postureBodyFrames.end()) ? bfIt->second : 0;

        auto& stored = g_railLeans[postureAddr];
        const float alpha = std::clamp(dt * kRailLeanSmoothRate, 0.0f, 1.0f);
        stored += (targetLean - stored) * alpha;
        if (std::fabs(stored) < 0.004f && std::fabs(targetLean) < 0.004f)
        {
            g_railLeans.erase(postureAddr);
            if (bodyFrameAddr != 0)
                g_grindBodyLeans.erase(bodyFrameAddr);
            return;
        }

        if (bodyFrameAddr != 0)
            g_grindBodyLeans[bodyFrameAddr] = stored;
    }
}

// MotionHierarchy::Apply: apply camera-independent rail lean and 360-degree corkscrew flip
// (plus Sonic acrobatic limb tuck) strictly to the character's visual skeleton.
PPC_FUNC_IMPL(__imp__sub_825991F0);
PPC_FUNC(sub_825991F0)
{
    const uint32_t motionAddr = ctx.r3.u32;
    const uint32_t frameAddr = ctx.r4.u32;
    VisualPoseSample sample{};

    if (!g_inRailSwitchHook && Config::RailSwitching &&
        IsGuestHeapAddress(frameAddr) && IsGuestHeapAddress(motionAddr) &&
        ReadGuestU32(base, motionAddr) == kMotionHierarchyVftable)
    {
        const uint32_t nodeCount = ReadGuestU32(base, motionAddr + kMotionNodeCountOffset);
        if (nodeCount > 0 && nodeCount <= 256u)
        {
            std::unique_lock lock(g_railSwitchMutex, std::try_to_lock);
            if (lock.owns_lock())
            {
                const uint32_t poseAddr = ReadGuestU32(base, motionAddr + kMotionLocalPoseOffset);
                const bool poseValid = IsGuestHeapAddress(poseAddr) && (poseAddr & 15u) == 0 &&
                    IsGuestHeapAddress(poseAddr + nodeCount * sizeof(MotionLocalPose) - 4);
                if (poseValid)
                {
                    const auto arcIt = std::find_if(g_activeArcs.begin(), g_activeArcs.end(),
                        [frameAddr](const auto& entry) { return entry.second.bodyFrameAddr == frameAddr; });
                    if (arcIt != g_activeArcs.end() && arcIt->second.elapsed > 0.0f && arcIt->second.elapsed < arcIt->second.duration)
                    {
                        const float u = std::clamp(arcIt->second.elapsed / arcIt->second.duration, 0.0f, 1.0f);
                        sample.localPoseAddr = poseAddr;
                        sample.nodeCount = nodeCount;
                        sample.rootRotation = ComputeFlipLocalRotation(u, arcIt->second.switchLeft, arcIt->second.startLean);
                        sample.pivotY = kFlipPivotHeightCm;
                        sample.flipProgress = u;
                        sample.isFlip = true;
                        sample.switchLeft = arcIt->second.switchLeft;
                    }
                    else if (auto leanIt = g_grindBodyLeans.find(frameAddr); leanIt != g_grindBodyLeans.end())
                    {
                        sample.localPoseAddr = poseAddr;
                        sample.nodeCount = nodeCount;
                        sample.rootRotation = ComputeLeanLocalRotation(leanIt->second);
                        sample.pivotY = 0.0f;
                        sample.isFlip = false;
                    }
                }
            }
        }
    }

    const VisualPoseSample previous = g_visualPoseSample;
    g_visualPoseSample = sample;
    __imp__sub_825991F0(ctx, base);
    g_visualPoseSample = previous;
}

// Convert a single local pose to a matrix. Pass a private copy to the game so
// blending and later evaluations keep the original sampled poses intact.
PPC_FUNC_IMPL(__imp__sub_825A0048);
PPC_FUNC(sub_825A0048)
{
    const uint32_t firstPose = g_visualPoseSample.localPoseAddr;
    if (firstPose != 0 && ctx.r4.u32 >= firstPose)
    {
        const uint32_t offset = ctx.r4.u32 - firstPose;
        if (offset < g_visualPoseSample.nodeCount * sizeof(MotionLocalPose) &&
            (offset % sizeof(MotionLocalPose)) == 0)
        {
            const uint32_t node = offset / sizeof(MotionLocalPose);
            auto pose = *reinterpret_cast<const MotionLocalPose*>(base + ctx.r4.u32);
            if (ApplyVisualNodePose(pose, node, g_visualPoseSample))
            {
                guest_stack_var<MotionLocalPose> adjusted(pose);
                const uint64_t originalPose = ctx.r4.u64;
                ctx.r4.u64 = g_memory.MapVirtual(adjusted.get());
                __imp__sub_825A0048(ctx, base);
                ctx.r4.u64 = originalPose;
                return;
            }
        }
    }
    __imp__sub_825A0048(ctx, base);
}

// MeshSplinePath factory: track all grind rails (collision attribute bit 0x00400000).
PPC_FUNC_IMPL(__imp__sub_826007A0);
PPC_FUNC(sub_826007A0)
{
    const uint32_t outSlotAddr = ctx.r3.u32;
    const uint32_t attrMask = ctx.r6.u32;

    __imp__sub_826007A0(ctx, base);

    if ((attrMask & kGrindRailAttrBit) != 0 && IsGuestHeapAddress(outSlotAddr))
    {
        const uint32_t railAddr = ReadGuestU32(base, outSlotAddr);
        if (IsGuestHeapAddress(railAddr))
        {
            std::lock_guard lock(g_railSwitchMutex);
            g_grindRails.insert(railAddr);
        }
    }
}

// MeshSplinePath destructor: unregister destroyed rails and cancel any arc targeting them.
PPC_FUNC_IMPL(__imp__sub_825FF850);
PPC_FUNC(sub_825FF850)
{
    const uint32_t railAddr = ctx.r3.u32;
    {
        std::lock_guard lock(g_railSwitchMutex);
        if (g_grindRails.erase(railAddr) != 0)
        {
            for (auto it = g_activeArcs.begin(); it != g_activeArcs.end(); )
            {
                if (it->second.targetMeshRailAddr == railAddr)
                {
                    ReleaseRefCountObject(it->second.targetSplineCtrlAddr);
                    it = g_activeArcs.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    __imp__sub_825FF850(ctx, base);
}

// Sonicteam::Player::State::CommonGrind::Update
PPC_FUNC_IMPL(__imp__sub_8220B598);
PPC_FUNC(sub_8220B598)
{
    if (g_inRailSwitchHook)
    {
        __imp__sub_8220B598(ctx, base);
        return;
    }

    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t contextAddr = IsGuestHeapAddress(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0;
    const uint32_t postureAddr = Config::RailSwitching ? ResolvePostureAddrFromContext(base, contextAddr) : 0;

    if (postureAddr == 0)
    {
        if (contextAddr != 0)
            UpdatePendingRailJump(contextAddr, false, 0, 0.0f);
        __imp__sub_8220B598(ctx, base);
        return;
    }

    g_inRailSwitchHook = true;

    if (HasActiveArc(postureAddr))
    {
        // Still allow damage, springs, dash rings, and other external transitions to interrupt the hop.
        const uint32_t transitioned = GuestToHostFunction<uint32_t>(sub_8220AAF8, g_memory.Translate(stateAddr));
        g_inRailSwitchHook = false;
        ctx.r3.u64 = (transitioned & 0xFFu) != 0 ? 1u : 0u;
        return;
    }

    auto* pContext = reinterpret_cast<Sonicteam::Player::State::CommonContext*>(base + contextAddr);
    if (const uint32_t bodyFrameAddr = ResolvePlayerBodyFrame(base, pContext); bodyFrameAddr != 0)
    {
        std::lock_guard lock(g_railSwitchMutex);
        g_postureBodyFrames[postureAddr] = bodyFrameAddr;
    }
    const uint32_t postureFlags = pContext->m_PostureFlags.get();
    const bool onRail = (postureFlags & static_cast<uint32_t>(Sonicteam::Player::PostureControlFlags_RailGrind)) != 0;
    const bool eligible = onRail && pContext->m_LockButtons.get() <= 0.0f;
    const uint32_t buttons = pContext->m_Buttons.get();
    const bool jumpPressed = (buttons & kJumpPressed) != 0;
    const bool jumpHeld = (buttons & kJumpHeld) != 0 && (buttons & kJumpReleased) == 0;
    const float dt = static_cast<float>(ctx.f1.f64);
    const auto pending = UpdatePendingRailJump(contextAddr, eligible, buttons, dt);
    bool sendJumpPressed = jumpPressed;

    if (pending == PendingRailJump::Waiting)
    {
        sendJumpPressed = false;
    }
    else if (pending == PendingRailJump::Tap ||
        (pending == PendingRailJump::None && eligible && jumpPressed && !jumpHeld))
    {
        // Recheck the rail from the player's position and stick direction at release.
        if (TryRailSwitch(base, contextAddr, postureAddr, RailSwitchRequest::Start))
        {
            g_inRailSwitchHook = false;
            ctx.r3.u64 = 0; // Stay in CommonGrind while the switch arc executes.
            return;
        }
        sendJumpPressed = true;
    }
    else if (pending == PendingRailJump::Normal)
    {
        sendJumpPressed = true;
        LOG("[rail-switch] Hold elapsed: normal rail jump");
    }
    else if (eligible && jumpPressed && jumpHeld && std::isfinite(dt) && dt >= 0.0f &&
        TryRailSwitch(base, contextAddr, postureAddr, RailSwitchRequest::Check))
    {
        // Keep running native grind while a press can still become either a tap or a hold.
        std::lock_guard lock(g_railSwitchMutex);
        g_pendingRailJumps.try_emplace(contextAddr, 0.0f);
        sendJumpPressed = false;
    }

    if (sendJumpPressed != jumpPressed)
        pContext->m_Buttons = (buttons & ~kJumpPressed) | (sendJumpPressed ? kJumpPressed : 0u);

    g_inRailSwitchHook = false;
    __imp__sub_8220B598(ctx, base);

    if (sendJumpPressed != jumpPressed)
        pContext->m_Buttons = (pContext->m_Buttons.get() & ~kJumpPressed) | (buttons & kJumpPressed);
}

// Sonicteam::Player::State::CommonGrind::OnEnd
PPC_FUNC_IMPL(__imp__sub_82209660);
PPC_FUNC(sub_82209660)
{
    const uint32_t stateAddr = ctx.r3.u32;
    const uint32_t contextAddr = IsGuestHeapAddress(stateAddr) ? ReadGuestU32(base, stateAddr + 8u) : 0;
    if (contextAddr != 0)
    {
        const uint32_t postureAddr = ResolvePostureAddrFromContext(base, contextAddr);
        if (postureAddr != 0)
        {
            std::lock_guard lock(g_railSwitchMutex);
            g_railLeans.erase(postureAddr);
            if (auto bfIt = g_postureBodyFrames.find(postureAddr); bfIt != g_postureBodyFrames.end())
            {
                g_grindBodyLeans.erase(bfIt->second);
                g_postureBodyFrames.erase(bfIt);
            }
        }
        if (CancelArcForContext(contextAddr))
            LOG("[rail-switch] Cancelled: player left CommonGrind");
    }

    __imp__sub_82209660(ctx, base);
}

// Sonicteam::Player::PostureControl::Update
PPC_FUNC_IMPL(__imp__sub_82200538);
PPC_FUNC(sub_82200538)
{
    const uint32_t postureAddr = ctx.r3.u32;
    if (g_inRailSwitchHook)
    {
        __imp__sub_82200538(ctx, base);
        return;
    }

    const float dt = static_cast<float>(ctx.f1.f64);
    g_inRailSwitchHook = true;
    const bool handled = StepActiveRailSwitch(base, postureAddr, dt);
    g_inRailSwitchHook = false;

    if (!handled)
    {
        __imp__sub_82200538(ctx, base);
        g_inRailSwitchHook = true;
        UpdateGrindRailLean(base, postureAddr, dt);
        g_inRailSwitchHook = false;
    }
}
