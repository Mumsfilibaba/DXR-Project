#pragma once
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"

struct FInt128
{
    int32 x = 0;
    int32 y = 0;
    int32 z = 0;
    int32 w = 0;
};

struct FFloat128
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct FGenericVectorMath
{
private:
    static FORCEINLINE uint32 VECTORCALL FloatToBits(float Value) noexcept
    {
        uint32 Bits = 0;
        FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
        return Bits;
    }

    static FORCEINLINE float VECTORCALL BitsToFloat(uint32 Bits) noexcept
    {
        float Value = 0.0f;
        FMemory::Memcpy(&Value, &Bits, sizeof(Value));
        return Value;
    }

    static FORCEINLINE uint32 VECTORCALL IntToBits(int32 Value) noexcept
    {
        uint32 Bits = 0;
        FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
        return Bits;
    }

    static FORCEINLINE int32 VECTORCALL BitsToInt(uint32 Bits) noexcept
    {
        int32 Value = 0;
        FMemory::Memcpy(&Value, &Bits, sizeof(Value));
        return Value;
    }

    static FORCEINLINE int32 VECTORCALL AddWrapInt32(int32 A, int32 B) noexcept
    {
        const uint32 UA = IntToBits(A);
        const uint32 UB = IntToBits(B);
        return BitsToInt(UA + UB);
    }

    static FORCEINLINE int32 VECTORCALL SubWrapInt32(int32 A, int32 B) noexcept
    {
        const uint32 UA = IntToBits(A);
        const uint32 UB = IntToBits(B);
        return BitsToInt(UA - UB);
    }

    static FORCEINLINE int32 VECTORCALL MulWrapInt32(int32 A, int32 B) noexcept
    {
        const uint64 UA = static_cast<uint64>(IntToBits(A));
        const uint64 UB = static_cast<uint64>(IntToBits(B));
        const uint32 Lo = static_cast<uint32>(UA * UB);
        return BitsToInt(Lo);
    }

    static FORCEINLINE uint32 VECTORCALL BoolMask(bool bValue) noexcept
    {
        return bValue ? 0xFFFFFFFFu : 0u;
    }

    static FORCEINLINE FFloat128 VECTORCALL MaskToFloat128(uint32 Mask) noexcept
    {
        const float M = BitsToFloat(Mask);
        return FFloat128{ M, M, M, M };
    }

public:

    // ---------------------------------------------------------------------------------------------
    // Load / Store
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorLoad(const float* Source) noexcept
    {
        return FFloat128{ Source[0], Source[1], Source[2], Source[3] };
    }

    static FORCEINLINE void VECTORCALL VectorStore(FFloat128 Vector, float* Dest) noexcept
    {
        Dest[0] = Vector.x;
        Dest[1] = Vector.y;
        Dest[2] = Vector.z;
        Dest[3] = Vector.w;
    }

    static FORCEINLINE void VECTORCALL VectorStore3(FFloat128 Vector, float* Dest) noexcept
    {
        Dest[0] = Vector.x;
        Dest[1] = Vector.y;
        Dest[2] = Vector.z;
    }

    // ---------------------------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorSet(float x, float y, float z, float w) noexcept
    {
        return FFloat128{ x, y, z, w };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet1(float Scalar) noexcept
    {
        return FFloat128{ Scalar, Scalar, Scalar, Scalar };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSetScalar(float Scalar) noexcept
    {
        return FFloat128{ Scalar, 0.0f, 0.0f, 0.0f };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOne() noexcept
    {
        return VectorSet1(1.0f);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorZero() noexcept
    {
        return VectorSet1(0.0f);
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

        const float V[4] = { VectorA.x, VectorA.y, VectorA.z, VectorA.w };
        return FFloat128{ V[ComponentIndexX], V[ComponentIndexY], V[ComponentIndexZ], V[ComponentIndexW] };
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        const float A[4] = { VectorA.x, VectorA.y, VectorA.z, VectorA.w };
        const float B[4] = { VectorB.x, VectorB.y, VectorB.z, VectorB.w };
        return FFloat128{ A[ComponentIndexX], A[ComponentIndexY], B[ComponentIndexZ], B[ComponentIndexW] };
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        const float A[4] = { VectorA.x, VectorA.y, VectorA.z, VectorA.w };
        const float B[4] = { VectorB.x, VectorB.y, VectorB.z, VectorB.w };
        return FFloat128{ A[ComponentIndexX], B[ComponentIndexY], A[ComponentIndexZ], B[ComponentIndexW] };
    }

    template<uint8 ComponentIndex>
    static FORCEINLINE FFloat128 VECTORCALL VectorBroadcast(FFloat128 Vector) noexcept
    {
        static_assert(ComponentIndex < 4, "ComponentIndex out of range");
        return VectorShuffle<ComponentIndex, ComponentIndex, ComponentIndex, ComponentIndex>(Vector);
    }

    // ---------------------------------------------------------------------------------------------
    // Component extract
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept { return Vector.x; }
    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept { return Vector.y; }
    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept { return Vector.z; }
    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept { return Vector.w; }

    // ---------------------------------------------------------------------------------------------
    // Arithmetic
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.x * VectorB.x, VectorA.y * VectorB.y, VectorA.z * VectorB.z, VectorA.w * VectorB.w };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.x / VectorB.x, VectorA.y / VectorB.y, VectorA.z / VectorB.z, VectorA.w / VectorB.w };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.x + VectorB.x, VectorA.y + VectorB.y, VectorA.z + VectorB.z, VectorA.w + VectorB.w };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.x - VectorB.x, VectorA.y - VectorB.y, VectorA.z - VectorB.z, VectorA.w - VectorB.w };
    }

    // ---------------------------------------------------------------------------------------------
    // Horizontal ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Matches _mm_hadd_ps(A, B) layout: (Ax+Ay, Az+Aw, Bx+By, Bz+Bw)
        return FFloat128{ VectorA.x + VectorA.y, VectorA.z + VectorA.w, VectorB.x + VectorB.y, VectorB.z + VectorB.w };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        const float Sum = Vector.x + Vector.y + Vector.z + Vector.w;
        return VectorSet1(Sum);
    }

    // ---------------------------------------------------------------------------------------------
    // Reciprocal / sqrt
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128{ Math::Sqrt(Vector.x), Math::Sqrt(Vector.y), Math::Sqrt(Vector.z), Math::Sqrt(Vector.w) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128{ 1.0f / Math::Sqrt(Vector.x), 1.0f / Math::Sqrt(Vector.y), 1.0f / Math::Sqrt(Vector.z), 1.0f / Math::Sqrt(Vector.w) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecip(FFloat128 Vector) noexcept
    {
        return FFloat128{ 1.0f / Vector.x, 1.0f / Vector.y, 1.0f / Vector.z, 1.0f / Vector.w };
    }

    // ---------------------------------------------------------------------------------------------
    // Bitwise float ops (masks/signs/branchless)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorAnd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = FloatToBits(VectorA.x), Ay = FloatToBits(VectorA.y), Az = FloatToBits(VectorA.z), Aw = FloatToBits(VectorA.w);
        const uint32 Bx = FloatToBits(VectorB.x), By = FloatToBits(VectorB.y), Bz = FloatToBits(VectorB.z), Bw = FloatToBits(VectorB.w);

        return FFloat128{ BitsToFloat(Ax & Bx), BitsToFloat(Ay & By), BitsToFloat(Az & Bz), BitsToFloat(Aw & Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOr(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = FloatToBits(VectorA.x), Ay = FloatToBits(VectorA.y), Az = FloatToBits(VectorA.z), Aw = FloatToBits(VectorA.w);
        const uint32 Bx = FloatToBits(VectorB.x), By = FloatToBits(VectorB.y), Bz = FloatToBits(VectorB.z), Bw = FloatToBits(VectorB.w);

        return FFloat128{ BitsToFloat(Ax | Bx), BitsToFloat(Ay | By), BitsToFloat(Az | Bz), BitsToFloat(Aw | Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorXor(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = FloatToBits(VectorA.x), Ay = FloatToBits(VectorA.y), Az = FloatToBits(VectorA.z), Aw = FloatToBits(VectorA.w);
        const uint32 Bx = FloatToBits(VectorB.x), By = FloatToBits(VectorB.y), Bz = FloatToBits(VectorB.z), Bw = FloatToBits(VectorB.w);

        return FFloat128{ BitsToFloat(Ax ^ Bx), BitsToFloat(Ay ^ By), BitsToFloat(Az ^ Bz), BitsToFloat(Aw ^ Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAndNot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // (~A) & B
        const uint32 Ax = FloatToBits(VectorA.x), Ay = FloatToBits(VectorA.y), Az = FloatToBits(VectorA.z), Aw = FloatToBits(VectorA.w);
        const uint32 Bx = FloatToBits(VectorB.x), By = FloatToBits(VectorB.y), Bz = FloatToBits(VectorB.z), Bw = FloatToBits(VectorB.w);

        return FFloat128{ BitsToFloat((~Ax) & Bx), BitsToFloat((~Ay) & By), BitsToFloat((~Az) & Bz), BitsToFloat((~Aw) & Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSelect(FFloat128 Mask, FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // (Mask & A) | (~Mask & B)
        return VectorOr(VectorAnd(Mask, VectorA), VectorAndNot(Mask, VectorB));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSignMask() noexcept
    {
        // Sign bit set in all lanes (-0.0f)
        const uint32 Sign = 0x80000000u;
        return MaskToFloat128(Sign);
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
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x == VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y == VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z == VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w == VectorB.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareNotEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x != VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y != VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z != VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w != VectorB.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x > VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y > VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z > VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w > VectorB.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x >= VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y >= VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z >= VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w >= VectorB.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x < VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y < VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z < VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w < VectorB.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(VectorA.x <= VectorB.x)),
            BitsToFloat(BoolMask(VectorA.y <= VectorB.y)),
            BitsToFloat(BoolMask(VectorA.z <= VectorB.z)),
            BitsToFloat(BoolMask(VectorA.w <= VectorB.w))
        };
    }

    // ---------------------------------------------------------------------------------------------
    // Special compare masks (NaN / Inf / NearEqual)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorIsNaNMask(FFloat128 Vector) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(Math::IsNaN(Vector.x))),
            BitsToFloat(BoolMask(Math::IsNaN(Vector.y))),
            BitsToFloat(BoolMask(Math::IsNaN(Vector.z))),
            BitsToFloat(BoolMask(Math::IsNaN(Vector.w)))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIsInfMask(FFloat128 Vector) noexcept
    {
        return FFloat128
        {
            BitsToFloat(BoolMask(Math::IsInfinity(Vector.x))),
            BitsToFloat(BoolMask(Math::IsInfinity(Vector.y))),
            BitsToFloat(BoolMask(Math::IsInfinity(Vector.z))),
            BitsToFloat(BoolMask(Math::IsInfinity(Vector.w)))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNearEqualMask(FFloat128 VectorA, FFloat128 VectorB, FFloat128 Epsilon) noexcept
    {
        const float Dx = Math::Abs(VectorA.x - VectorB.x);
        const float Dy = Math::Abs(VectorA.y - VectorB.y);
        const float Dz = Math::Abs(VectorA.z - VectorB.z);
        const float Dw = Math::Abs(VectorA.w - VectorB.w);

        return FFloat128
        {
            BitsToFloat(BoolMask(Dx <= Epsilon.x)),
            BitsToFloat(BoolMask(Dy <= Epsilon.y)),
            BitsToFloat(BoolMask(Dz <= Epsilon.z)),
            BitsToFloat(BoolMask(Dw <= Epsilon.w))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNearEqualMask(FFloat128 VectorA, FFloat128 VectorB, float Epsilon) noexcept
    {
        return VectorNearEqualMask(VectorA, VectorB, VectorSet1(Epsilon));
    }

    // ---------------------------------------------------------------------------------------------
    // Dot + reductions (bool checks)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const float Dot = (VectorA.x * VectorB.x) + (VectorA.y * VectorB.y) + (VectorA.z * VectorB.z) + (VectorA.w * VectorB.w);
        return VectorSet1(Dot);
    }

    static FORCEINLINE bool VECTORCALL VectorAllEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.x == VectorB.x) && (VectorA.y == VectorB.y) && (VectorA.z == VectorB.z) && (VectorA.w == VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.x > VectorB.x) && (VectorA.y > VectorB.y) && (VectorA.z > VectorB.z) && (VectorA.w > VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.x >= VectorB.x) && (VectorA.y >= VectorB.y) && (VectorA.z >= VectorB.z) && (VectorA.w >= VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.x < VectorB.x) && (VectorA.y < VectorB.y) && (VectorA.z < VectorB.z) && (VectorA.w < VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.x <= VectorB.x) && (VectorA.y <= VectorB.y) && (VectorA.z <= VectorB.z) && (VectorA.w <= VectorB.w);
    }

    // ---------------------------------------------------------------------------------------------
    // Min / Max
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMin(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            (VectorA.x < VectorB.x) ? VectorA.x : VectorB.x,
            (VectorA.y < VectorB.y) ? VectorA.y : VectorB.y,
            (VectorA.z < VectorB.z) ? VectorA.z : VectorB.z,
            (VectorA.w < VectorB.w) ? VectorA.w : VectorB.w
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMax(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            (VectorA.x > VectorB.x) ? VectorA.x : VectorB.x,
            (VectorA.y > VectorB.y) ? VectorA.y : VectorB.y,
            (VectorA.z > VectorB.z) ? VectorA.z : VectorB.z,
            (VectorA.w > VectorB.w) ? VectorA.w : VectorB.w
        };
    }

    // ---------------------------------------------------------------------------------------------
    // Integer vector ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FInt128 VECTORCALL VectorLoadInt(const int32* Source) noexcept
    {
        return FInt128{ Source[0], Source[1], Source[2], Source[3] };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return FInt128{ x, y, z, w };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt1(int32 Scalar) noexcept
    {
        return FInt128{ Scalar, Scalar, Scalar, Scalar };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetScalarInt(int32 Scalar) noexcept
    {
        return FInt128{ Scalar, 0, 0, 0 };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorZeroInt() noexcept
    {
        return FInt128{ 0, 0, 0, 0 };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOneInt() noexcept
    {
        return FInt128{ 1, 1, 1, 1 };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIntToFloat(FInt128 Vector) noexcept
    {
        return FFloat128
        {
            BitsToFloat(IntToBits(Vector.x)),
            BitsToFloat(IntToBits(Vector.y)),
            BitsToFloat(IntToBits(Vector.z)),
            BitsToFloat(IntToBits(Vector.w))
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return FInt128
        {
            BitsToInt(FloatToBits(Vector.x)),
            BitsToInt(FloatToBits(Vector.y)),
            BitsToInt(FloatToBits(Vector.z)),
            BitsToInt(FloatToBits(Vector.w))
        };
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept
    {
        Dest[0] = Vector.x;
        Dest[1] = Vector.y;
        Dest[2] = Vector.z;
        Dest[3] = Vector.w;
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt3(FInt128 Vector, int32* Dest) noexcept
    {
        Dest[0] = Vector.x;
        Dest[1] = Vector.y;
        Dest[2] = Vector.z;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAddInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (wrap-around / modulo 2^32).
        return FInt128
        {
            AddWrapInt32(VectorA.x, VectorB.x),
            AddWrapInt32(VectorA.y, VectorB.y),
            AddWrapInt32(VectorA.z, VectorB.z),
            AddWrapInt32(VectorA.w, VectorB.w)
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSubInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (wrap-around / modulo 2^32).
        return FInt128
        {
            SubWrapInt32(VectorA.x, VectorB.x),
            SubWrapInt32(VectorA.y, VectorB.y),
            SubWrapInt32(VectorA.z, VectorB.z),
            SubWrapInt32(VectorA.w, VectorB.w)
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (mullo / modulo 2^32).
        return FInt128
        {
            MulWrapInt32(VectorA.x, VectorB.x),
            MulWrapInt32(VectorA.y, VectorB.y),
            MulWrapInt32(VectorA.z, VectorB.z),
            MulWrapInt32(VectorA.w, VectorB.w)
        };
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.x == VectorB.x) && (VectorA.y == VectorB.y) && (VectorA.z == VectorB.z) && (VectorA.w == VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.x > VectorB.x) && (VectorA.y > VectorB.y) && (VectorA.z > VectorB.z) && (VectorA.w > VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.x >= VectorB.x) && (VectorA.y >= VectorB.y) && (VectorA.z >= VectorB.z) && (VectorA.w >= VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.x < VectorB.x) && (VectorA.y < VectorB.y) && (VectorA.z < VectorB.z) && (VectorA.w < VectorB.w);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.x <= VectorB.x) && (VectorA.y <= VectorB.y) && (VectorA.z <= VectorB.z) && (VectorA.w <= VectorB.w);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128
        {
            (VectorA.x < VectorB.x) ? VectorA.x : VectorB.x,
            (VectorA.y < VectorB.y) ? VectorA.y : VectorB.y,
            (VectorA.z < VectorB.z) ? VectorA.z : VectorB.z,
            (VectorA.w < VectorB.w) ? VectorA.w : VectorB.w
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128
        {
            (VectorA.x > VectorB.x) ? VectorA.x : VectorB.x,
            (VectorA.y > VectorB.y) ? VectorA.y : VectorB.y,
            (VectorA.z > VectorB.z) ? VectorA.z : VectorB.z,
            (VectorA.w > VectorB.w) ? VectorA.w : VectorB.w
        };
    }
};
