#pragma once
#include "float.h"

#include <math.h>

class Math
{
public:
    static constexpr float PI      = 3.14159265359f;
    static constexpr float TWO_PI  = PI * 2.0f;
    static constexpr float HALF_PI = PI / 2.0f;

public:
    template <typename T>
    FORCEINLINE static T DivideByMultiple(T Value, uint32 Alignment)
    {
        return static_cast<T>((Value + Alignment - 1) / Alignment);
    }

    template <typename T>
    FORCEINLINE static T AlignUp(T Value, T Alignment)
    {
        static_assert(std::is_integral<T>());

        const T mask = Alignment - 1;
        return ((Value + mask) & (~mask));
    }

    template <typename T>
    FORCEINLINE static T AlignDown(T Value, T Alignment)
    {
        static_assert(std::is_integral<T>());

        const T mask = Alignment - 1;
        return ((Value) & (~mask));
    }

    FORCEINLINE static float Lerp(float a, float b, float f)
    {
        return (-f * b) + ((a * f) + b);
    }

    template <typename T>
    FORCEINLINE static T Min(T a, T b)
    {
        return a <= b ? a : b;
    }

    template <typename T>
    FORCEINLINE static T Max(T a, T b)
    {
        return a >= b ? a : b;
    }

    template <typename T>
    FORCEINLINE static T Abs(T a)
    {
        return (a * a) / a;
    }

    template <typename T>
    FORCEINLINE static T ToRadians(T Degrees)
    {
        return (T)XMConvertToRadians((T)Degrees);
    }

    template <typename T>
    FORCEINLINE static T ToDegrees(T Radians)
    {
        return (T)XMConvertToDegrees((T)Radians);
    }

    template <typename T>
    FORCEINLINE static T Log2(T x)
    {
        return (T)std::log2((double)x);
    }

    FORCEINLINE static uint32 BytesToNum32BitConstants(uint32 Bytes)
    {
        return Bytes / 4;
    }
};

// XMFLOAT2
inline XMFLOAT2 operator*(XMFLOAT2 LHS, float RHS)
{
    return XMFLOAT2(LHS.x * RHS, LHS.y * RHS);
}

inline XMFLOAT2 operator*(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x * RHS.x, LHS.y * RHS.y);
}

inline XMFLOAT2 operator+(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x + RHS.x, LHS.y + RHS.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 LHS, XMFLOAT2 RHS)
{
    return XMFLOAT2(LHS.x - RHS.x, LHS.y - RHS.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 Value)
{
    return XMFLOAT2(-Value.x, -Value.y);
}

inline float Length(const XMFLOAT2& Vector)
{
    return (float)sqrt(Vector.x * Vector.x + Vector.y * Vector.y);
}

// XMFLOAT3
inline XMFLOAT3 operator*(XMFLOAT3 LHS, float RHS)
{
    return XMFLOAT3(LHS.x * RHS, LHS.y * RHS, LHS.z * RHS);
}

inline XMFLOAT3 operator*(XMFLOAT3 LHS, XMFLOAT3 RHS)
{
    return XMFLOAT3(LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z);
}

inline XMFLOAT3 operator/(XMFLOAT3 LHS, float RHS)
{
    return XMFLOAT3(LHS.x / RHS, LHS.y / RHS, LHS.z / RHS);
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

inline XMFLOAT3 operator-(XMFLOAT3 Value)
{
    return XMFLOAT3(-Value.x, -Value.y, -Value.z);
}

inline float Length(const XMFLOAT3& Vector)
{
    return (float)sqrt(Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z);
}

// XMFLOAT4
inline XMFLOAT4 operator*(XMFLOAT4 LHS, float RHS)
{
    return XMFLOAT4(LHS.x * RHS, LHS.y * RHS, LHS.z * RHS, LHS.w * RHS);
}

inline XMFLOAT4 operator*(XMFLOAT4 LHS, XMFLOAT4 RHS)
{
    return XMFLOAT4(LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z, LHS.w * RHS.w);
}

inline XMFLOAT4 operator/(XMFLOAT4 LHS, float RHS)
{
    return XMFLOAT4(LHS.x / RHS, LHS.y / RHS, LHS.z / RHS, LHS.w / RHS);
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

inline XMFLOAT4 operator-(XMFLOAT4 Value)
{
    return XMFLOAT4(-Value.x, -Value.y, -Value.z, -Value.w);
}

inline float Length(const XMFLOAT4& Vector)
{
    return (float)sqrt(Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z + Vector.w * Vector.w);
}