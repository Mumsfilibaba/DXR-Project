#pragma once
#include "Float.h"

struct Math
{
    static constexpr Float PI      = 3.14159265359f;
    static constexpr Float TWO_PI  = PI * 2.0f;
    static constexpr Float HALF_PI = PI / 2.0f;

    static FORCEINLINE Float RadicalInverse(UInt32 Bits)
    {
        Bits = (Bits << 16u) | (Bits >> 16u);
        Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
        Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
        Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
        Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
        return Float(Bits) * 2.3283064365386963e-10f;
    }

    static FORCEINLINE XMFLOAT2 Hammersley(UInt32 i, UInt32 n)
    {
        return XMFLOAT2(Float(i) / Float(n), RadicalInverse(i));
    }

    template <typename T>
    static FORCEINLINE T DivideByMultiple(T Value, UInt32 Alignment)
    {
        return static_cast<T>((Value + Alignment - 1) / Alignment);
    }

    template <typename T>
    static FORCEINLINE T AlignUp(T Value, T Alignment)
    {
        static_assert(std::is_integral<T>());

        const T mask = Alignment - 1;
        return ((Value + mask) & (~mask));
    }

    template <typename T>
    static FORCEINLINE T AlignDown(T Value, T Alignment)
    {
        static_assert(std::is_integral<T>());

        const T mask = Alignment - 1;
        return ((Value) & (~mask));
    }

    static FORCEINLINE Float Lerp(Float a, Float b, Float f)
    {
        return (-f * b) + ((a * f) + b);
    }

    template <typename T>
    static FORCEINLINE T Min(T a, T b)
    {
        return a <= b ? a : b;
    }

    template <typename T>
    static FORCEINLINE T Max(T a, T b)
    {
        return a >= b ? a : b;
    }

    template <typename T>
    static FORCEINLINE T Abs(T a)
    {
        return (a * a) / a;
    }
};

inline XMFLOAT2 operator*(XMFLOAT2 LHS, Float RHS)
{
    return XMFLOAT2(LHS.x * RHS, LHS.y * RHS);
}

inline XMFLOAT2 operator*(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x * RHS.x, LHS.y * RHS.y);
}

inline XMFLOAT2 operator/(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x / RHS.x, LHS.y / RHS.y);
}

inline XMFLOAT2 operator+(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x + RHS.x, LHS.y + RHS.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x - RHS.x, LHS.y - RHS.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 LHS, Float RHS)
{
    return XMFLOAT2(LHS.x - RHS, LHS.y - RHS);
}

inline XMFLOAT2 operator-(XMFLOAT2 Value)
{
    return XMFLOAT2(-Value.x, -Value.y);
}

inline XMFLOAT3 operator*(XMFLOAT3 LHS, Float RHS)
{
    return XMFLOAT3(LHS.x * RHS, LHS.y * RHS, LHS.z * RHS);
}

inline XMFLOAT3 operator*(XMFLOAT3 LHS, XMFLOAT3 RHS)
{
    return XMFLOAT3(LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z);
}

inline XMFLOAT3 operator/(XMFLOAT3 LHS, XMFLOAT3 RHS)
{
    return XMFLOAT3(LHS.x / RHS.x, LHS.y / RHS.y, LHS.z / RHS.z);
}

inline XMFLOAT3 operator+(XMFLOAT3 LHS, XMFLOAT3 RHS)
{
    return XMFLOAT3(LHS.x + RHS.x, LHS.y + RHS.y, LHS.z + RHS.z);
}

inline XMFLOAT3 operator-(XMFLOAT3 LHS, XMFLOAT3 RHS)
{
    return XMFLOAT3(LHS.x - RHS.x, LHS.y - RHS.y, LHS.z - RHS.z);
}

inline XMFLOAT3 operator-(XMFLOAT3 LHS, Float RHS)
{
    return XMFLOAT3(LHS.x - RHS, LHS.y - RHS, LHS.z - RHS);
}

inline XMFLOAT3 operator-(XMFLOAT3 Value)
{
    return XMFLOAT3(-Value.x, -Value.y, -Value.z);
}

inline XMFLOAT4 operator*(XMFLOAT4 LHS, Float RHS)
{
    return XMFLOAT4(LHS.x * RHS, LHS.y * RHS, LHS.z * RHS, LHS.w * RHS);
}

inline XMFLOAT4 operator*(XMFLOAT4 LHS, XMFLOAT4 RHS)
{
    return XMFLOAT4(LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z, LHS.w * RHS.w);
}

inline XMFLOAT4 operator/(XMFLOAT4 LHS, XMFLOAT4 RHS)
{
    return XMFLOAT4(LHS.x / RHS.x, LHS.y / RHS.y, LHS.z / RHS.z, LHS.w / RHS.w);
}

inline XMFLOAT4 operator+(XMFLOAT4 LHS, XMFLOAT4 RHS)
{
    return XMFLOAT4(LHS.x + RHS.x, LHS.y + RHS.y, LHS.z + RHS.z, LHS.w + RHS.w);
}

inline XMFLOAT4 operator-(XMFLOAT4 LHS, XMFLOAT4 RHS)
{
    return XMFLOAT4(LHS.x - RHS.x, LHS.y - RHS.y, LHS.z - RHS.z, LHS.w - RHS.w);
}

inline XMFLOAT4 operator-(XMFLOAT4 LHS, Float RHS)
{
    return XMFLOAT4(LHS.x - RHS, LHS.y - RHS, LHS.z - RHS, LHS.w - RHS);
}

inline XMFLOAT4 operator-(XMFLOAT4 Value)
{
    return XMFLOAT4(-Value.x, -Value.y, -Value.z, -Value.w);
}

inline std::string ToString(const XMFLOAT4X4& Matrix)
{
    std::string Result;
    for (UInt32 y = 0; y < 4; y++)
    {
        for (UInt32 x = 0; x < 3; x++)
        {
            Result += std::to_string(Matrix.m[y][x]) + ", ";
        }

        Result += std::to_string(Matrix.m[y][3]) + "\n";
    }

    return Result;
}