#include <stdafx.h>
#include <cpu/ppc_context.h>
#include <os/android/storage_android.h>
#include <os/logger.h>

namespace
{
    alignas(16) constexpr uint8_t kGuestVectorByteSwap[16] =
    {
        0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
    };

    bool UseNativeMathHooks()
    {
        static const bool s_enabled = []()
        {
            std::error_code error;
            const bool disabled = std::filesystem::exists(
                os::android::GetDataRoot() / "native_math_hooks.disabled", error);
            LOGF("Guest math SIMD hooks {}.",
                disabled ? "disabled by native_math_hooks.disabled" : "enabled (sub_82168C48, sub_825A1490)");
            return !disabled;
        }();
        return s_enabled;
    }

    inline simde__m128i LoadUnalignedGuestVecI(const uint8_t* base, uint32_t guestAddr, simde__m128i swapMask) noexcept
    {
        return simde_mm_shuffle_epi8(
            simde_mm_loadu_si128(reinterpret_cast<const simde__m128i*>(base + guestAddr)),
            swapMask);
    }

    inline simde__m128i LoadAlignedGuestVecI(const uint8_t* base, uint32_t guestAddr, simde__m128i swapMask) noexcept
    {
        return simde_mm_shuffle_epi8(
            simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(base + (guestAddr & ~0xFU))),
            swapMask);
    }

    inline void StoreAlignedGuestVecI(uint8_t* base, uint32_t guestAddr, simde__m128i value, simde__m128i swapMask) noexcept
    {
        simde_mm_store_si128(
            reinterpret_cast<simde__m128i*>(base + (guestAddr & ~0xFU)),
            simde_mm_shuffle_epi8(value, swapMask));
    }
}

// 4x4 guest float matrix multiply (r3 = out, r4 = matA, r5 = matB; 2.9% of guest CPU on SM7475).
// Eliminates 32 volatile scalar 64-bit stack copies and 20 PPCContext vector register spills per call
// while preserving exact SIMDe rounding, denormal flush, unaligned input loads, and in-place aliasing.
PPC_FUNC_IMPL(__imp__sub_82168C48);
PPC_FUNC(sub_82168C48)
{
    if (!UseNativeMathHooks() || ((ctx.r1.u32 & 0xFU) != 0)) [[unlikely]]
    {
        __imp__sub_82168C48(ctx, base);
        return;
    }

    const uint32_t out = ctx.r3.u32;
    const uint32_t matA = ctx.r4.u32;
    const uint32_t matB = ctx.r5.u32;
    const simde__m128i swapMask = simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(kGuestVectorByteSwap));

    const simde__m128i a0 = LoadUnalignedGuestVecI(base, matA + 0U, swapMask);
    const simde__m128i a1 = LoadUnalignedGuestVecI(base, matA + 16U, swapMask);
    const simde__m128i a2 = LoadUnalignedGuestVecI(base, matA + 32U, swapMask);
    const simde__m128i a3 = LoadUnalignedGuestVecI(base, matA + 48U, swapMask);

    const simde__m128 b0 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matB + 0U, swapMask));
    const simde__m128 b1 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matB + 16U, swapMask));
    const simde__m128 b2 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matB + 32U, swapMask));
    const simde__m128 b3 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matB + 48U, swapMask));

    const simde__m128i t0 = simde_mm_unpackhi_epi32(a2, a0);
    const simde__m128i t1 = simde_mm_unpacklo_epi32(a2, a0);
    const simde__m128i t2 = simde_mm_unpackhi_epi32(a3, a1);
    const simde__m128i t3 = simde_mm_unpacklo_epi32(a3, a1);

    const simde__m128 c0 = simde_mm_castsi128_ps(simde_mm_unpackhi_epi32(t2, t0));
    const simde__m128 c1 = simde_mm_castsi128_ps(simde_mm_unpacklo_epi32(t2, t0));
    const simde__m128 c2 = simde_mm_castsi128_ps(simde_mm_unpackhi_epi32(t3, t1));
    const simde__m128 c3 = simde_mm_castsi128_ps(simde_mm_unpacklo_epi32(t3, t1));

    ctx.fpscr.enableFlushMode();
    ctx.fpscr.overrideRoundingMode(PPC_ROUND_TOWARD_ZERO);

    const simde__m128i d00 = simde_mm_castps_si128(simde_mm_dp_ps(b0, c0, 0xFF));
    const simde__m128i d02 = simde_mm_castps_si128(simde_mm_dp_ps(b0, c2, 0xFF));
    const simde__m128i d03 = simde_mm_castps_si128(simde_mm_dp_ps(b0, c3, 0xFF));
    const simde__m128i d01 = simde_mm_castps_si128(simde_mm_dp_ps(b0, c1, 0xFF));

    const simde__m128i d10 = simde_mm_castps_si128(simde_mm_dp_ps(b1, c0, 0xFF));
    const simde__m128i d11 = simde_mm_castps_si128(simde_mm_dp_ps(b1, c1, 0xFF));
    const simde__m128i d12 = simde_mm_castps_si128(simde_mm_dp_ps(b1, c2, 0xFF));
    const simde__m128i d13 = simde_mm_castps_si128(simde_mm_dp_ps(b1, c3, 0xFF));

    const simde__m128i d22 = simde_mm_castps_si128(simde_mm_dp_ps(b2, c2, 0xFF));
    const simde__m128i d30 = simde_mm_castps_si128(simde_mm_dp_ps(b3, c0, 0xFF));
    const simde__m128i d32 = simde_mm_castps_si128(simde_mm_dp_ps(b3, c2, 0xFF));
    const simde__m128i d20 = simde_mm_castps_si128(simde_mm_dp_ps(b2, c0, 0xFF));
    const simde__m128i d21 = simde_mm_castps_si128(simde_mm_dp_ps(b2, c1, 0xFF));
    const simde__m128i d31 = simde_mm_castps_si128(simde_mm_dp_ps(b3, c1, 0xFF));
    const simde__m128i d33 = simde_mm_castps_si128(simde_mm_dp_ps(b3, c3, 0xFF));
    const simde__m128i d23 = simde_mm_castps_si128(simde_mm_dp_ps(b2, c3, 0xFF));

    const simde__m128i r0 = simde_mm_unpackhi_epi32(
        simde_mm_unpackhi_epi32(d03, d01),
        simde_mm_unpackhi_epi32(d02, d00));
    const simde__m128i r1 = simde_mm_unpackhi_epi32(
        simde_mm_unpackhi_epi32(d13, d11),
        simde_mm_unpackhi_epi32(d12, d10));
    const simde__m128i r2 = simde_mm_unpackhi_epi32(
        simde_mm_unpackhi_epi32(d23, d21),
        simde_mm_unpackhi_epi32(d22, d20));
    const simde__m128i r3 = simde_mm_unpackhi_epi32(
        simde_mm_unpackhi_epi32(d33, d31),
        simde_mm_unpackhi_epi32(d32, d30));

    StoreAlignedGuestVecI(base, out + 0U, r0, swapMask);
    StoreAlignedGuestVecI(base, out + 16U, r1, swapMask);
    StoreAlignedGuestVecI(base, out + 48U, r3, swapMask);
    StoreAlignedGuestVecI(base, out + 32U, r2, swapMask);

    ctx.fpscr.restoreRoundingMode();
}

// Perspective-transforms 8 AABB corners and computes projected min/max bounds
// (r3 = outMinMax, r4 = mat4x4, r5 = corners8; 5.9% of guest CPU on SM7475).
// Eliminates re-copying the unchanged 64-byte matrix onto the guest stack 8 times per call (128 volatile
// 64-bit loads/stores) and 35 stack vector round-trips while keeping identical Newton-Raphson 1/w math.
PPC_FUNC_IMPL(__imp__sub_825A1490);
PPC_FUNC(sub_825A1490)
{
    if (!UseNativeMathHooks() || ((ctx.r1.u32 & 0xFU) != 0)) [[unlikely]]
    {
        __imp__sub_825A1490(ctx, base);
        return;
    }

    const uint32_t out = ctx.r3.u32;
    const uint32_t matAddr = ctx.r4.u32;
    const uint32_t cornersAddr = ctx.r5.u32;
    const simde__m128i swapMask = simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(kGuestVectorByteSwap));

    const simde__m128 m0 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matAddr + 0U, swapMask));
    const simde__m128 m1 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matAddr + 16U, swapMask));
    const simde__m128 m2 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matAddr + 32U, swapMask));
    const simde__m128 m3 = simde_mm_castsi128_ps(LoadUnalignedGuestVecI(base, matAddr + 48U, swapMask));

    ctx.fpscr.enableFlushMode();

    const simde__m128 one = simde_mm_set1_ps(1.0f);
    const simde__m128 signMask = simde_mm_castsi128_ps(simde_mm_set1_epi32(int(0x80000000)));

    const auto transformCorner = [&](uint32_t index) noexcept -> simde__m128
    {
        const simde__m128i p = LoadAlignedGuestVecI(base, cornersAddr + index * 16U, swapMask);
        const simde__m128 pz = simde_mm_castsi128_ps(simde_mm_shuffle_epi32(p, 0x55));
        const simde__m128 py = simde_mm_castsi128_ps(simde_mm_shuffle_epi32(p, 0xAA));
        const simde__m128 px = simde_mm_castsi128_ps(simde_mm_shuffle_epi32(p, 0xFF));

        const simde__m128 accZ = simde_mm_add_ps(simde_mm_mul_ps(m2, pz), m3);
        const simde__m128 accY = simde_mm_add_ps(simde_mm_mul_ps(py, m1), accZ);
        const simde__m128 clip = simde_mm_add_ps(simde_mm_mul_ps(px, m0), accY);

        const simde__m128 w = simde_mm_castsi128_ps(simde_mm_shuffle_epi32(simde_mm_castps_si128(clip), 0x00));
        const simde__m128 rcp0 = simde_mm_div_ps(one, w);
        const simde__m128 err0 = simde_mm_xor_ps(simde_mm_sub_ps(simde_mm_mul_ps(w, rcp0), one), signMask);
        const simde__m128 rcp1 = simde_mm_add_ps(simde_mm_mul_ps(rcp0, err0), rcp0);
        const simde__m128 err1 = simde_mm_xor_ps(simde_mm_sub_ps(simde_mm_mul_ps(w, rcp1), one), signMask);
        const simde__m128 notNan = simde_mm_cmpeq_ps(rcp1, rcp1);
        const simde__m128 rcp2 = simde_mm_add_ps(simde_mm_mul_ps(rcp1, err1), rcp1);
        const simde__m128 invW = simde_mm_castsi128_ps(simde_mm_or_si128(
            simde_mm_andnot_si128(simde_mm_castps_si128(notNan), simde_mm_castps_si128(rcp0)),
            simde_mm_and_si128(simde_mm_castps_si128(notNan), simde_mm_castps_si128(rcp2))));
        return simde_mm_mul_ps(clip, invW);
    };

    simde__m128 minVec = transformCorner(0);
    simde__m128 maxVec = minVec;

    for (uint32_t i = 1; i < 8; ++i)
    {
        const simde__m128 proj = transformCorner(i);
        minVec = simde_mm_min_ps(minVec, proj);
        maxVec = simde_mm_max_ps(maxVec, proj);
    }

    StoreAlignedGuestVecI(base, out + 0U, simde_mm_castps_si128(minVec), swapMask);
    StoreAlignedGuestVecI(base, out + 16U, simde_mm_castps_si128(maxVec), swapMask);
    ctx.r3.u64 = out;
}
