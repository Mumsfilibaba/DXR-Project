#pragma once
#include "Float.h"

/*
* Math
*/

class Math
{
public:
	template <typename T>
	FORCEINLINE static T DivideByMultiple(T Value, UInt32 Alignment)
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

	FORCEINLINE static Float Lerp(Float a, Float b, Float f)
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
};

/*
* Helpers
*/

inline XMFLOAT2 operator*(XMFLOAT2 Left, Float Right)
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

inline XMFLOAT3 operator*(XMFLOAT3 Left, Float Right)
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

inline XMFLOAT4 operator*(XMFLOAT4 Left, Float Right)
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