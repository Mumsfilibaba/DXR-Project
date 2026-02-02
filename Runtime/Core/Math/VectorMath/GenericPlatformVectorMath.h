#pragma once
#include "Core/Math/Math.h"

struct FInt128
{
    FORCEINLINE constexpr FInt128() noexcept
        : X(0), Y(0), Z(0), W(0)
    {
    }

    FORCEINLINE constexpr FInt128(int32 InX, int32 InY, int32 InZ, int32 InW) noexcept
        : X(InX), Y(InY), Z(InZ), W(InW)
    {
    }

    union
    {
        struct
        {
            int32 X;
            int32 Y;
            int32 Z;
            int32 W;
        };

        int32 XYZW[4];
    };
};

struct FFloat128
{
    FORCEINLINE constexpr FFloat128() noexcept
        : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f)
    {
    }

    FORCEINLINE constexpr FFloat128(float InX, float InY, float InZ, float InW) noexcept
        : X(InX), Y(InY), Z(InZ), W(InW)
    {
    }

    union
    {
        struct
        {
            float X;
            float Y;
            float Z;
            float W;
        };

        float XYZW[4];
    };
};

struct FGenericPlatformVectorMath
{
private:
    static FORCEINLINE FFloat128 VECTORCALL MaskToFloat128(uint32 Mask) noexcept
    {
        const float M = Math::BitsToFloat(Mask);
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
        Dest[0] = Vector.X;
        Dest[1] = Vector.Y;
        Dest[2] = Vector.Z;
        Dest[3] = Vector.W;
    }

    static FORCEINLINE void VECTORCALL VectorStore3(FFloat128 Vector, float* Dest) noexcept
    {
        Dest[0] = Vector.X;
        Dest[1] = Vector.Y;
        Dest[2] = Vector.Z;
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

        const float V[4] = { VectorA.X, VectorA.Y, VectorA.Z, VectorA.W };
        return FFloat128{ V[ComponentIndexX], V[ComponentIndexY], V[ComponentIndexZ], V[ComponentIndexW] };
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        const float A[4] = { VectorA.X, VectorA.Y, VectorA.Z, VectorA.W };
        const float B[4] = { VectorB.X, VectorB.Y, VectorB.Z, VectorB.W };
        return FFloat128{ A[ComponentIndexX], A[ComponentIndexY], B[ComponentIndexZ], B[ComponentIndexW] };
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        const float A[4] = { VectorA.X, VectorA.Y, VectorA.Z, VectorA.W };
        const float B[4] = { VectorB.X, VectorB.Y, VectorB.Z, VectorB.W };
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

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept { return Vector.X; }
    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept { return Vector.Y; }
    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept { return Vector.Z; }
    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept { return Vector.W; }

    // ---------------------------------------------------------------------------------------------
    // Arithmetic
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.X * VectorB.X, VectorA.Y * VectorB.Y, VectorA.Z * VectorB.Z, VectorA.W * VectorB.W };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.X / VectorB.X, VectorA.Y / VectorB.Y, VectorA.Z / VectorB.Z, VectorA.W / VectorB.W };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.X + VectorB.X, VectorA.Y + VectorB.Y, VectorA.Z + VectorB.Z, VectorA.W + VectorB.W };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128{ VectorA.X - VectorB.X, VectorA.Y - VectorB.Y, VectorA.Z - VectorB.Z, VectorA.W - VectorB.W };
    }

    // ---------------------------------------------------------------------------------------------
    // Horizontal ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Matches _mm_hadd_ps(A, B) layout: (Ax+Ay, Az+Aw, Bx+By, Bz+Bw)
        return FFloat128{ VectorA.X + VectorA.Y, VectorA.Z + VectorA.W, VectorB.X + VectorB.Y, VectorB.Z + VectorB.W };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        const float Sum = Vector.X + Vector.Y + Vector.Z + Vector.W;
        return VectorSet1(Sum);
    }

    // ---------------------------------------------------------------------------------------------
    // Reciprocal / sqrt
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128{ Math::Sqrt(Vector.X), Math::Sqrt(Vector.Y), Math::Sqrt(Vector.Z), Math::Sqrt(Vector.W) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128{ 1.0f / Math::Sqrt(Vector.X), 1.0f / Math::Sqrt(Vector.Y), 1.0f / Math::Sqrt(Vector.Z), 1.0f / Math::Sqrt(Vector.W) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecip(FFloat128 Vector) noexcept
    {
        return FFloat128{ 1.0f / Vector.X, 1.0f / Vector.Y, 1.0f / Vector.Z, 1.0f / Vector.W };
    }

    // ---------------------------------------------------------------------------------------------
    // Bitwise float ops (masks/signs/branchless)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorAnd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = Math::FloatToBits(VectorA.X), Ay = Math::FloatToBits(VectorA.Y), Az = Math::FloatToBits(VectorA.Z), Aw = Math::FloatToBits(VectorA.W);
        const uint32 Bx = Math::FloatToBits(VectorB.X), By = Math::FloatToBits(VectorB.Y), Bz = Math::FloatToBits(VectorB.Z), Bw = Math::FloatToBits(VectorB.W);

        return FFloat128{ Math::BitsToFloat(Ax & Bx), Math::BitsToFloat(Ay & By), Math::BitsToFloat(Az & Bz), Math::BitsToFloat(Aw & Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOr(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = Math::FloatToBits(VectorA.X), Ay = Math::FloatToBits(VectorA.Y), Az = Math::FloatToBits(VectorA.Z), Aw = Math::FloatToBits(VectorA.W);
        const uint32 Bx = Math::FloatToBits(VectorB.X), By = Math::FloatToBits(VectorB.Y), Bz = Math::FloatToBits(VectorB.Z), Bw = Math::FloatToBits(VectorB.W);

        return FFloat128{ Math::BitsToFloat(Ax | Bx), Math::BitsToFloat(Ay | By), Math::BitsToFloat(Az | Bz), Math::BitsToFloat(Aw | Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorXor(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const uint32 Ax = Math::FloatToBits(VectorA.X), Ay = Math::FloatToBits(VectorA.Y), Az = Math::FloatToBits(VectorA.Z), Aw = Math::FloatToBits(VectorA.W);
        const uint32 Bx = Math::FloatToBits(VectorB.X), By = Math::FloatToBits(VectorB.Y), Bz = Math::FloatToBits(VectorB.Z), Bw = Math::FloatToBits(VectorB.W);

        return FFloat128{ Math::BitsToFloat(Ax ^ Bx), Math::BitsToFloat(Ay ^ By), Math::BitsToFloat(Az ^ Bz), Math::BitsToFloat(Aw ^ Bw) };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAndNot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // (~A) & B
        const uint32 Ax = Math::FloatToBits(VectorA.X), Ay = Math::FloatToBits(VectorA.Y), Az = Math::FloatToBits(VectorA.Z), Aw = Math::FloatToBits(VectorA.W);
        const uint32 Bx = Math::FloatToBits(VectorB.X), By = Math::FloatToBits(VectorB.Y), Bz = Math::FloatToBits(VectorB.Z), Bw = Math::FloatToBits(VectorB.W);

        return FFloat128{ Math::BitsToFloat((~Ax) & Bx), Math::BitsToFloat((~Ay) & By), Math::BitsToFloat((~Az) & Bz), Math::BitsToFloat((~Aw) & Bw) };
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
            Math::BitsToFloat(Math::BoolMask(VectorA.X == VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y == VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z == VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W == VectorB.W))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareNotEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(VectorA.X != VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y != VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z != VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W != VectorB.W))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(VectorA.X > VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y > VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z > VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W > VectorB.W))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(VectorA.X >= VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y >= VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z >= VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W >= VectorB.W))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(VectorA.X < VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y < VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z < VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W < VectorB.W))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCompareLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(VectorA.X <= VectorB.X)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Y <= VectorB.Y)),
            Math::BitsToFloat(Math::BoolMask(VectorA.Z <= VectorB.Z)),
            Math::BitsToFloat(Math::BoolMask(VectorA.W <= VectorB.W))
        };
    }

    // ---------------------------------------------------------------------------------------------
    // Special compare masks (NaN / Inf / NearEqual)
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorIsNaNMask(FFloat128 Vector) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(Math::IsNaN(Vector.X))),
            Math::BitsToFloat(Math::BoolMask(Math::IsNaN(Vector.Y))),
            Math::BitsToFloat(Math::BoolMask(Math::IsNaN(Vector.Z))),
            Math::BitsToFloat(Math::BoolMask(Math::IsNaN(Vector.W)))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIsInfMask(FFloat128 Vector) noexcept
    {
        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(Math::IsInfinity(Vector.X))),
            Math::BitsToFloat(Math::BoolMask(Math::IsInfinity(Vector.Y))),
            Math::BitsToFloat(Math::BoolMask(Math::IsInfinity(Vector.Z))),
            Math::BitsToFloat(Math::BoolMask(Math::IsInfinity(Vector.W)))
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorNearEqualMask(FFloat128 VectorA, FFloat128 VectorB, FFloat128 Epsilon) noexcept
    {
        const float Dx = Math::Abs(VectorA.X - VectorB.X);
        const float Dy = Math::Abs(VectorA.Y - VectorB.Y);
        const float Dz = Math::Abs(VectorA.Z - VectorB.Z);
        const float Dw = Math::Abs(VectorA.W - VectorB.W);

        return FFloat128
        {
            Math::BitsToFloat(Math::BoolMask(Dx <= Epsilon.X)),
            Math::BitsToFloat(Math::BoolMask(Dy <= Epsilon.Y)),
            Math::BitsToFloat(Math::BoolMask(Dz <= Epsilon.Z)),
            Math::BitsToFloat(Math::BoolMask(Dw <= Epsilon.W))
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
        const float Dot = (VectorA.X * VectorB.X) + (VectorA.Y * VectorB.Y) + (VectorA.Z * VectorB.Z) + (VectorA.W * VectorB.W);
        return VectorSet1(Dot);
    }

    static FORCEINLINE bool VECTORCALL VectorAllEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.X == VectorB.X) && (VectorA.Y == VectorB.Y) && (VectorA.Z == VectorB.Z) && (VectorA.W == VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.X > VectorB.X) && (VectorA.Y > VectorB.Y) && (VectorA.Z > VectorB.Z) && (VectorA.W > VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.X >= VectorB.X) && (VectorA.Y >= VectorB.Y) && (VectorA.Z >= VectorB.Z) && (VectorA.W >= VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.X < VectorB.X) && (VectorA.Y < VectorB.Y) && (VectorA.Z < VectorB.Z) && (VectorA.W < VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return (VectorA.X <= VectorB.X) && (VectorA.Y <= VectorB.Y) && (VectorA.Z <= VectorB.Z) && (VectorA.W <= VectorB.W);
    }

    // ---------------------------------------------------------------------------------------------
    // Min / Max
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FFloat128 VECTORCALL VectorMin(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            (VectorA.X < VectorB.X) ? VectorA.X : VectorB.X,
            (VectorA.Y < VectorB.Y) ? VectorA.Y : VectorB.Y,
            (VectorA.Z < VectorB.Z) ? VectorA.Z : VectorB.Z,
            (VectorA.W < VectorB.W) ? VectorA.W : VectorB.W
        };
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMax(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128
        {
            (VectorA.X > VectorB.X) ? VectorA.X : VectorB.X,
            (VectorA.Y > VectorB.Y) ? VectorA.Y : VectorB.Y,
            (VectorA.Z > VectorB.Z) ? VectorA.Z : VectorB.Z,
            (VectorA.W > VectorB.W) ? VectorA.W : VectorB.W
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
            Math::BitsToFloat(Math::IntToBits(Vector.X)),
            Math::BitsToFloat(Math::IntToBits(Vector.Y)),
            Math::BitsToFloat(Math::IntToBits(Vector.Z)),
            Math::BitsToFloat(Math::IntToBits(Vector.W))
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return FInt128
        {
            Math::BitsToInt(Math::FloatToBits(Vector.X)),
            Math::BitsToInt(Math::FloatToBits(Vector.Y)),
            Math::BitsToInt(Math::FloatToBits(Vector.Z)),
            Math::BitsToInt(Math::FloatToBits(Vector.W))
        };
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept
    {
        Dest[0] = Vector.X;
        Dest[1] = Vector.Y;
        Dest[2] = Vector.Z;
        Dest[3] = Vector.W;
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt3(FInt128 Vector, int32* Dest) noexcept
    {
        Dest[0] = Vector.X;
        Dest[1] = Vector.Y;
        Dest[2] = Vector.Z;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAddInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (wrap-around / modulo 2^32).
        return FInt128
        {
            Math::AddWrapInt32(VectorA.X, VectorB.X),
            Math::AddWrapInt32(VectorA.Y, VectorB.Y),
            Math::AddWrapInt32(VectorA.Z, VectorB.Z),
            Math::AddWrapInt32(VectorA.W, VectorB.W)
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSubInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (wrap-around / modulo 2^32).
        return FInt128
        {
            Math::SubWrapInt32(VectorA.X, VectorB.X),
            Math::SubWrapInt32(VectorA.Y, VectorB.Y),
            Math::SubWrapInt32(VectorA.Z, VectorB.Z),
            Math::SubWrapInt32(VectorA.W, VectorB.W)
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Match SSE2 semantics (mullo / modulo 2^32).
        return FInt128
        {
            Math::MulWrapInt32(VectorA.X, VectorB.X),
            Math::MulWrapInt32(VectorA.Y, VectorB.Y),
            Math::MulWrapInt32(VectorA.Z, VectorB.Z),
            Math::MulWrapInt32(VectorA.W, VectorB.W)
        };
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.X == VectorB.X) && (VectorA.Y == VectorB.Y) && (VectorA.Z == VectorB.Z) && (VectorA.W == VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.X > VectorB.X) && (VectorA.Y > VectorB.Y) && (VectorA.Z > VectorB.Z) && (VectorA.W > VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.X >= VectorB.X) && (VectorA.Y >= VectorB.Y) && (VectorA.Z >= VectorB.Z) && (VectorA.W >= VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.X < VectorB.X) && (VectorA.Y < VectorB.Y) && (VectorA.Z < VectorB.Z) && (VectorA.W < VectorB.W);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return (VectorA.X <= VectorB.X) && (VectorA.Y <= VectorB.Y) && (VectorA.Z <= VectorB.Z) && (VectorA.W <= VectorB.W);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128
        {
            (VectorA.X < VectorB.X) ? VectorA.X : VectorB.X,
            (VectorA.Y < VectorB.Y) ? VectorA.Y : VectorB.Y,
            (VectorA.Z < VectorB.Z) ? VectorA.Z : VectorB.Z,
            (VectorA.W < VectorB.W) ? VectorA.W : VectorB.W
        };
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128
        {
            (VectorA.X > VectorB.X) ? VectorA.X : VectorB.X,
            (VectorA.Y > VectorB.Y) ? VectorA.Y : VectorB.Y,
            (VectorA.Z > VectorB.Z) ? VectorA.Z : VectorB.Z,
            (VectorA.W > VectorB.W) ? VectorA.W : VectorB.W
        };
    }
};
