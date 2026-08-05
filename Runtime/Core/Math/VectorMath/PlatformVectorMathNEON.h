#pragma once
#if PLATFORM_SUPPORT_NEON_INTRIN
#include "Core/Math/Math.h"

#if PLATFORM_WINDOWS
    #include <arm64_neon.h>
#elif PLATFORM_MACOS
    #include <arm_neon.h>
#else
    #error "No valid platform. This code requires ARM NEON support on Windows or macOS."
#endif

typedef float32x4_t FFloat128;
typedef int32x4_t   FInt128;

struct FPlatformVectorMathNEON
{
private:
    static FORCEINLINE uint32x4_t VECTORCALL VectorAsUInt(FFloat128 Vector) noexcept
    {
        return vreinterpretq_u32_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorFromUInt(uint32x4_t Vector) noexcept
    {
        return vreinterpretq_f32_u32(Vector);
    }

    static FORCEINLINE uint32x4_t VECTORCALL VectorAsUInt(FInt128 Vector) noexcept
    {
        return vreinterpretq_u32_s32(Vector);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFromUIntToInt(uint32x4_t Vector) noexcept
    {
        return vreinterpretq_s32_u32(Vector);
    }

    static FORCEINLINE bool VECTORCALL VectorAllTrueU32(uint32x4_t Mask) noexcept
    {
    #if defined(__aarch64__)
        // If all lanes are 0xFFFFFFFF, the min is 0xFFFFFFFF.
        return vminvq_u32(Mask) == 0xFFFFFFFFu;
    #else
        ALIGN_AS(16) uint32 Tmp[4];
        vst1q_u32(Tmp, Mask);
        return (Tmp[0] == 0xFFFFFFFFu) && (Tmp[1] == 0xFFFFFFFFu) && (Tmp[2] == 0xFFFFFFFFu) && (Tmp[3] == 0xFFFFFFFFu);
    #endif
    }

public:

    // ---------------------------------------------------------------------------------------------
    // Load / Store
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorLoad(const float* Source) noexcept
    {
        return vld1q_f32(Source);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorLoadAligned(const float* Source) noexcept
    {
        // NEON loads are generally unaligned-safe; alignment is a hint only.
        return vld1q_f32(Source);
    }

    static FORCEINLINE void VECTORCALL VectorStore(FFloat128 Vector, float* Dest) noexcept
    {
        vst1q_f32(Dest, Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreAligned(FFloat128 Vector, float* Dest) noexcept
    {
        vst1q_f32(Dest, Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStore3(FFloat128 Vector, float* Dest) noexcept
    {
        ALIGN_AS(16) float Tmp[4];
        vst1q_f32(Tmp, Vector);
        Dest[0] = Tmp[0];
        Dest[1] = Tmp[1];
        Dest[2] = Tmp[2];
    }

    // ---------------------------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorSet(float x, float y, float z, float w) noexcept
    {
        float32x4_t V = vdupq_n_f32(0.0f);
        V = vsetq_lane_f32(x, V, 0);
        V = vsetq_lane_f32(y, V, 1);
        V = vsetq_lane_f32(z, V, 2);
        V = vsetq_lane_f32(w, V, 3);
        return V;
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet1(float Scalar) noexcept
    {
        return vdupq_n_f32(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSetScalar(float Scalar) noexcept
    {
        float32x4_t V = vdupq_n_f32(0.0f);
        return vsetq_lane_f32(Scalar, V, 0);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOne() noexcept
    {
        return vdupq_n_f32(1.0f);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorZero() noexcept
    {
        return vdupq_n_f32(0.0f);
    }

    // ---------------------------------------------------------------------------------------------
    // Shuffle / Broadcast
    // ---------------------------------------------------------------------------------------------

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle(FFloat128 VectorA) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        // Fast-path a handful of very common shuffles.
        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 1 && ComponentIndexZ == 0 && ComponentIndexW == 1)
        {
            // (x, y, x, y)
            const float32x2_t Lo = vget_low_f32(VectorA);
            return vcombine_f32(Lo, Lo);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 3 && ComponentIndexZ == 2 && ComponentIndexW == 3)
        {
            // (z, w, z, w)
            const float32x2_t Hi = vget_high_f32(VectorA);
            return vcombine_f32(Hi, Hi);
        }
        else if constexpr (ComponentIndexX == 1 && ComponentIndexY == 0 && ComponentIndexZ == 3 && ComponentIndexW == 2)
        {
            // (y, x, w, z)
            return vrev64q_f32(VectorA);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 3 && ComponentIndexZ == 0 && ComponentIndexW == 1)
        {
            // (z, w, x, y)
            return vextq_f32(VectorA, VectorA, 2);
        }
        else
        {
            // Correctness-first fallback: extract/insert lanes.
            float32x4_t R = vdupq_n_f32(0.0f);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexX), R, 0);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexY), R, 1);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexZ), R, 2);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexW), R, 3);
            return R;
        }
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 1 && ComponentIndexZ == 0 && ComponentIndexW == 1)
        {
            // (Ax, Ay, Bx, By)
            return vcombine_f32(vget_low_f32(VectorA), vget_low_f32(VectorB));
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 3 && ComponentIndexZ == 2 && ComponentIndexW == 3)
        {
            // (Az, Aw, Bz, Bw)
            return vcombine_f32(vget_high_f32(VectorA), vget_high_f32(VectorB));
        }
        else if constexpr (ComponentIndexX == 0 && ComponentIndexY == 2 && ComponentIndexZ == 0 && ComponentIndexW == 2)
        {
            // (Ax, Az, Bx, Bz)
            return vuzpq_f32(VectorA, VectorB).val[0];
        }
        else if constexpr (ComponentIndexX == 1 && ComponentIndexY == 3 && ComponentIndexZ == 1 && ComponentIndexW == 3)
        {
            // (Ay, Aw, By, Bw)
            return vuzpq_f32(VectorA, VectorB).val[1];
        }
        else
        {
            float32x4_t R = vdupq_n_f32(0.0f);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexX), R, 0);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexY), R, 1);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorB, ComponentIndexZ), R, 2);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorB, ComponentIndexW), R, 3);
            return R;
        }
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 0 && ComponentIndexZ == 1 && ComponentIndexW == 1)
        {
            // (Ax, Bx, Ay, By)
            return vzipq_f32(VectorA, VectorB).val[0];
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 2 && ComponentIndexZ == 3 && ComponentIndexW == 3)
        {
            // (Az, Bz, Aw, Bw)
            return vzipq_f32(VectorA, VectorB).val[1];
        }
        else
        {
            float32x4_t R = vdupq_n_f32(0.0f);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexX), R, 0);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorB, ComponentIndexY), R, 1);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorA, ComponentIndexZ), R, 2);
            R = vsetq_lane_f32(vgetq_lane_f32(VectorB, ComponentIndexW), R, 3);
            return R;
        }
    }

    template<uint8 ComponentIndex>
    static FORCEINLINE FFloat128 VECTORCALL VectorBroadcast(FFloat128 Vector) noexcept
    {
        static_assert(ComponentIndex < 4, "ComponentIndex out of range");
    #if defined(__aarch64__)
        return vdupq_laneq_f32(Vector, ComponentIndex);
    #else
        return vdupq_n_f32(vgetq_lane_f32(Vector, ComponentIndex));
    #endif
    }

    // ---------------------------------------------------------------------------------------------
    // Component extract
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept
    {
        return vgetq_lane_f32(Vector, 0);
    }

    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept
    {
        return vgetq_lane_f32(Vector, 1);
    }

    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept
    {
        return vgetq_lane_f32(Vector, 2);
    }

    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept
    {
        return vgetq_lane_f32(Vector, 3);
    }

    // ---------------------------------------------------------------------------------------------
    // Arithmetic
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return vmulq_f32(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
    #if defined(__aarch64__)
        return vdivq_f32(VectorA, VectorB);
    #else
        return vmulq_f32(VectorA, VectorRecipAccurate(VectorB));
    #endif
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return vaddq_f32(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return vsubq_f32(VectorA, VectorB);
    }

    // ---------------------------------------------------------------------------------------------
    // Horizontal ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Matches _mm_hadd_ps(A, B) layout: (Ax+Ay, Az+Aw, Bx+By, Bz+Bw)
        const float32x2_t SumA = vpadd_f32(vget_low_f32(VectorA),  vget_high_f32(VectorA));
        const float32x2_t SumB = vpadd_f32(vget_low_f32(VectorB),  vget_high_f32(VectorB));
        return vcombine_f32(SumA, SumB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
    #if defined(__aarch64__)
        const float Sum = vaddvq_f32(Vector);
        return vdupq_n_f32(Sum);
    #else
        const float32x2_t Sum2 = vpadd_f32(vget_low_f32(Vector), vget_high_f32(Vector)); // (x+y, z+w)
        const float32x2_t Sum1 = vpadd_f32(Sum2, Sum2);                                 // (sum, sum)
        return vdupq_n_f32(vget_lane_f32(Sum1, 0));
    #endif
    }

    // ---------------------------------------------------------------------------------------------
    // Reciprocal / sqrt
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 Vector) noexcept
    {
    #if defined(__aarch64__)
        return vsqrtq_f32(Vector);
    #else
        // Correctness-first fallback.
        ALIGN_AS(16) float Tmp[4];
        vst1q_f32(Tmp, Vector);
        Tmp[0] = Math::Sqrt(Tmp[0]);
        Tmp[1] = Math::Sqrt(Tmp[1]);
        Tmp[2] = Math::Sqrt(Tmp[2]);
        Tmp[3] = Math::Sqrt(Tmp[3]);
        return vld1q_f32(Tmp);
    #endif
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrt(FFloat128 Vector) noexcept
    {
        return vrsqrteq_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecip(FFloat128 Vector) noexcept
    {
        return vrecpeq_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipAccurate(FFloat128 Vector) noexcept
    {
        // Two Newton-Raphson refinement steps on rcp approximation.
        float32x4_t R = vrecpeq_f32(Vector);
        R = vmulq_f32(vrecpsq_f32(Vector, R), R);
        R = vmulq_f32(vrecpsq_f32(Vector, R), R);
        return R;
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrtAccurate(FFloat128 Vector) noexcept
    {
        // Two Newton-Raphson refinement steps on rsqrt approximation.
        float32x4_t R = vrsqrteq_f32(Vector);
        R = vmulq_f32(R, vrsqrtsq_f32(vmulq_f32(Vector, R), R));
        R = vmulq_f32(R, vrsqrtsq_f32(vmulq_f32(Vector, R), R));
        return R;
    }

    // ---------------------------------------------------------------------------------------------
    // Bitwise float ops (masks/signs/branchless)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorAnd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vandq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOr(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vorrq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorXor(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(veorq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAndNot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // (~A) & B
        const uint32x4_t NotA = vmvnq_u32(VectorAsUInt(VectorA));
        return VectorFromUInt(vandq_u32(NotA, VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSelect(FFloat128 Mask, FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // (Mask & A) | (~Mask & B)
        return VectorFromUInt(vbslq_u32(VectorAsUInt(Mask), VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSignMask() noexcept
    {
        // Sign bit set in all lanes (-0.0f)
        return VectorFromUInt(vdupq_n_u32(0x80000000u));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMaskXYZ() noexcept
    {
        // All bits set in x, y and z; w cleared.
        return VectorFromUInt(vsetq_lane_u32(0u, vdupq_n_u32(0xFFFFFFFFu), 3));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNegate(FFloat128 Vector) noexcept
    {
        return VectorXor(Vector, VectorSignMask());
    }

    // ---------------------------------------------------------------------------------------------
    // Compare masks (expected return format: all-bits set for true lanes, 0 for false lanes)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vceqq_f32(VectorA, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareNotEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32x4_t Eq = vceqq_f32(VectorA, VectorB);
        return VectorFromUInt(vmvnq_u32(Eq));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vcgtq_f32(VectorA, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vcgeq_f32(VectorA, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vcltq_f32(VectorA, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorFromUInt(vcleq_f32(VectorA, VectorB));
    }

    // ---------------------------------------------------------------------------------------------
    // Special compare masks (NaN / Inf / NearEqual)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorIsNaNMask(FFloat128 Vector) noexcept
    {
        // NaN lanes are the ones where (v == v) is false.
        const uint32x4_t Eq = vceqq_f32(Vector, Vector);
        return VectorFromUInt(vmvnq_u32(Eq));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIsInfMask(FFloat128 Vector) noexcept
    {
        // abs(x) == +inf
        const uint32x4_t Abs = vandq_u32(VectorAsUInt(Vector), vdupq_n_u32(0x7FFFFFFFu));
        const uint32x4_t Inf = vdupq_n_u32(0x7F800000u);
        return VectorFromUInt(vceqq_u32(Abs, Inf));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNearEqualMask(FFloat128 VectorA, FFloat128 VectorB, FFloat128 Epsilon) noexcept
    {
        // abs(a-b) <= epsilon
        const float32x4_t Diff = vsubq_f32(VectorA, VectorB);
        const uint32x4_t Abs = vandq_u32(VectorAsUInt(Diff), vdupq_n_u32(0x7FFFFFFFu));
        return VectorFromUInt(vcleq_f32(vreinterpretq_f32_u32(Abs), Epsilon));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNearEqualMask(FFloat128 VectorA, FFloat128 VectorB, float Epsilon) noexcept
    {
        return VectorNearEqualMask(VectorA, VectorB, vdupq_n_f32(Epsilon));
    }

    // ---------------------------------------------------------------------------------------------
    // Dot product
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorHorizontalSum(vmulq_f32(VectorA, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDot3(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // dot3(a,b) = dot4(a,b) - (aw*bw)
        const float32x4_t Dot4 = VectorDot(VectorA, VectorB);
        const float W = vgetq_lane_f32(VectorA, 3) * vgetq_lane_f32(VectorB, 3);
        return vsubq_f32(Dot4, vdupq_n_f32(W));
    }

    // ---------------------------------------------------------------------------------------------
    // Bool reductions (all lanes)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE bool VECTORCALL VectorAllEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorAllTrueU32(vceqq_f32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcgtq_f32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcgeq_f32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcltq_f32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcleq_f32(VectorA, VectorB));
    }

    // ---------------------------------------------------------------------------------------------
    // Min / Max
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMin(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return vminq_f32(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMax(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return vmaxq_f32(VectorA, VectorB);
    }

    // ---------------------------------------------------------------------------------------------
    // Integer ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FInt128 VECTORCALL VectorLoadInt(const int32* Source) noexcept
    {
        return vld1q_s32(Source);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorLoadIntAligned(const int32* Source) noexcept
    {
        return vld1q_s32(Source);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        int32x4_t V = vdupq_n_s32(0);
        V = vsetq_lane_s32(x, V, 0);
        V = vsetq_lane_s32(y, V, 1);
        V = vsetq_lane_s32(z, V, 2);
        V = vsetq_lane_s32(w, V, 3);
        return V;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt1(int32 Scalar) noexcept
    {
        return vdupq_n_s32(Scalar);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetScalarInt(int32 Scalar) noexcept
    {
        int32x4_t V = vdupq_n_s32(0);
        return vsetq_lane_s32(Scalar, V, 0);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorZeroInt() noexcept
    {
        return vdupq_n_s32(0);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOneInt() noexcept
    {
        return vdupq_n_s32(1);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIntToFloat(FInt128 Vector) noexcept
    {
        return vreinterpretq_f32_s32(Vector);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return vreinterpretq_s32_f32(Vector);
    }

    // ---------------------------------------------------------------------------------------------
    // Bitwise int ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FInt128 VECTORCALL VectorAndInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorFromUIntToInt(vandq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOrInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorFromUIntToInt(vorrq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorXorInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorFromUIntToInt(veorq_u32(VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAndNotInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // (~A) & B
        const uint32x4_t NotA = vmvnq_u32(VectorAsUInt(VectorA));
        return VectorFromUIntToInt(vandq_u32(NotA, VectorAsUInt(VectorB)));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSelectInt(FInt128 Mask, FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // (Mask & A) | (~Mask & B)
        return VectorFromUIntToInt(vbslq_u32(VectorAsUInt(Mask), VectorAsUInt(VectorA), VectorAsUInt(VectorB)));
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept
    {
        vst1q_s32(Dest, Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreIntAligned(FInt128 Vector, int32* Dest) noexcept
    {
        vst1q_s32(Dest, Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt3(FInt128 Vector, int32* Dest) noexcept
    {
        ALIGN_AS(16) int32 Tmp[4];
        vst1q_s32(Tmp, Vector);
        Dest[0] = Tmp[0];
        Dest[1] = Tmp[1];
        Dest[2] = Tmp[2];
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAddInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return vaddq_s32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSubInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return vsubq_s32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return vmulq_s32(VectorA, VectorB);
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorAllTrueU32(vceqq_s32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcgtq_s32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcgeq_s32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcltq_s32(VectorA, VectorB));
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return VectorAllTrueU32(vcleq_s32(VectorA, VectorB));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return vminq_s32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return vmaxq_s32(VectorA, VectorB);
    }

    // ---------------------------------------------------------------------------------------------
    // Rounding
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorRound(FFloat128 Vector) noexcept
    {
        // Round-to-nearest integral value.
        return vrndnq_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorFloor(FFloat128 Vector) noexcept
    {
        return vrndmq_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCeil(FFloat128 Vector) noexcept
    {
        return vrndpq_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorTrunc(FFloat128 Vector) noexcept
    {
        // Truncate toward zero.
        const int32x4_t I = vcvtq_s32_f32(Vector);
        return vcvtq_f32_s32(I);
    }
};

#endif
