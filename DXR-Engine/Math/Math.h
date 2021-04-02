#pragma once
#include "float.h"

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
};

inline XMFLOAT2 operator*(XMFLOAT2 Left, float Right)
{
    return XMFLOAT2(Left.x * Right, Left.y * Right);
}

inline XMFLOAT2 operator*(XMFLOAT2 Left, XMFLOAT2 Right)
{
    return XMFLOAT2(Left.x * Right.x, Left.y * Right.y);
}

inline XMFLOAT2 operator+(XMFLOAT2 Left, XMFLOAT2 Right)
{
    return XMFLOAT2(Left.x + Right.x, Left.y + Right.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 Left, XMFLOAT2 Right)
{
    return XMFLOAT2(Left.x - Right.x, Left.y - Right.y);
}

inline XMFLOAT2 operator-(XMFLOAT2 Value)
{
    return XMFLOAT2(-Value.x, -Value.y);
}

inline XMFLOAT3 operator*(XMFLOAT3 Left, float Right)
{
    return XMFLOAT3(Left.x * Right, Left.y * Right, Left.z * Right);
}

inline XMFLOAT3 operator*(XMFLOAT3 Left, XMFLOAT3 Right)
{
    return XMFLOAT3(Left.x * Right.x, Left.y * Right.y, Left.z * Right.z);
}

inline XMFLOAT3 operator+(XMFLOAT3 Left, XMFLOAT3 Right)
{
    return XMFLOAT3(Left.x + Right.x, Left.y + Right.y, Left.z + Right.z);
}

inline XMFLOAT3 operator-(XMFLOAT3 Left, XMFLOAT3 Right)
{
    return XMFLOAT3(Left.x - Right.x, Left.y - Right.y, Left.z - Right.z);
}

inline XMFLOAT3 operator-(XMFLOAT3 Value)
{
    return XMFLOAT3(-Value.x, -Value.y, -Value.z);
}

inline XMFLOAT4 operator*(XMFLOAT4 Left, float Right)
{
    return XMFLOAT4(Left.x * Right, Left.y * Right, Left.z * Right, Left.w * Right);
}

inline XMFLOAT4 operator*(XMFLOAT4 Left, XMFLOAT4 Right)
{
    return XMFLOAT4(Left.x * Right.x, Left.y * Right.y, Left.z * Right.z, Left.w * Right.w);
}

inline XMFLOAT4 operator+(XMFLOAT4 Left, XMFLOAT4 Right)
{
    return XMFLOAT4(Left.x + Right.x, Left.y + Right.y, Left.z + Right.z, Left.w + Right.w);
}

inline XMFLOAT4 operator-(XMFLOAT4 Left, XMFLOAT4 Right)
{
    return XMFLOAT4(Left.x - Right.x, Left.y - Right.y, Left.z - Right.z, Left.w - Right.w);
}

inline XMFLOAT4 operator-(XMFLOAT4 Value)
{
    return XMFLOAT4(-Value.x, -Value.y, -Value.z, -Value.w);
}