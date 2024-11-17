#pragma once
#include "Core/Core.h"
#include <cmath>

// bool constants
#define LIMITS_DIGITS_BOOL (1)
#define LIMITS_MIN_BOOL (false)
#define LIMITS_MAX_BOOL (true)

// int8 constants
#define LIMITS_DIGITS_INT8 (7)
#define LIMITS_MIN_INT8 ((int8)(-128))
#define LIMITS_MAX_INT8 ((int8)(127))

// uint8 constants
#define LIMITS_DIGITS_UINT8 (8)
#define LIMITS_MIN_UINT8 ((uint8)(0))
#define LIMITS_MAX_UINT8 ((uint8)(255))

// int16 constants
#define LIMITS_DIGITS_INT16 (15)
#define LIMITS_MIN_INT16 ((int16)(-32768))
#define LIMITS_MAX_INT16 ((int16)(32767))

// uint16 constants
#define LIMITS_DIGITS_UINT16 (16)
#define LIMITS_MIN_UINT16 ((uint16)(0))
#define LIMITS_MAX_UINT16 ((uint16)(65535))

// int32 constants
#define LIMITS_DIGITS_INT32 (31)
#define LIMITS_MIN_INT32 ((int32)(-2147483647 - 1))
#define LIMITS_MAX_INT32 ((int32)(2147483647))

// uint32 constants
#define LIMITS_DIGITS_UINT32 (32)
#define LIMITS_MIN_UINT32 ((uint32)(0))
#define LIMITS_MAX_UINT32 ((uint32)(4294967295U))

// int64 constants
#define LIMITS_DIGITS_INT64 (63)
#define LIMITS_MIN_INT64 ((int64)(-9223372036854775807LL - 1))
#define LIMITS_MAX_INT64 ((int64)(9223372036854775807LL))

// uint64 constants
#define LIMITS_DIGITS_UINT64 (64)
#define LIMITS_MIN_UINT64 ((uint64)(0))
#define LIMITS_MAX_UINT64 ((uint64)(18446744073709551615ULL))

// float constants
#define LIMITS_DIGITS_FLT (24)  // Number of mantissa bits for float
#define LIMITS_MIN_FLT ((float)(1.175494351e-38F))
#define LIMITS_MAX_FLT ((float)(3.402823466e+38F))
#define LIMITS_EPS_FLT ((float)(1.1920928955078125e-07F))

// double constants
#define LIMITS_DIGITS_DBL (53)  // Number of mantissa bits for double
#define LIMITS_MIN_DBL ((double)(2.2250738585072014e-308))
#define LIMITS_MAX_DBL ((double)(1.7976931348623158e+308))
#define LIMITS_EPS_DBL ((double)(2.2204460492503131e-16))

// long double constants
// Note: The exact values for long double depend on the platform and compiler.
// Below are typical values for an 80-bit long double (x87) on x86 architectures.
#define LIMITS_DIGITS_LDBL (64)  // Number of mantissa bits for long double (commonly 64)
#define LIMITS_MIN_LDBL ((long double)(3.362103143112093506262677817e-4932L))
#define LIMITS_MAX_LDBL ((long double)(1.18973149535723176502e+4932L))
#define LIMITS_EPS_LDBL ((long double)(1.08420217248550443401e-19L))

template<typename T>
struct TNumericLimits;

template<typename T>
struct TNumericLimits<const T> : TNumericLimits<T> { };

template<typename T>
struct TNumericLimits<volatile T> : TNumericLimits<T> { };

template<typename T>
struct TNumericLimits<const volatile T> : TNumericLimits<T> { };

// bool specialization
template<>
struct TNumericLimits<bool>
{
public:
    typedef bool ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_BOOL;
    }
    
    NODISCARD static constexpr ValueType Min() noexcept
    { 
        return LIMITS_MIN_BOOL; 
    }

    NODISCARD static constexpr ValueType Max() noexcept
    { 
        return LIMITS_MAX_BOOL; 
    }

    NODISCARD static constexpr ValueType Lowest() noexcept 
    { 
        return LIMITS_MIN_BOOL;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept 
    { 
        return false;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// int8 specialization
template<>
struct TNumericLimits<int8>
{
public:
    typedef int8 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_INT8;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_INT8;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_INT8;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_INT8;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// uint8 specialization
template<>
struct TNumericLimits<uint8>
{
public:
    typedef uint8 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_UINT8;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_UINT8;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_UINT8;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_UINT8;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// int16 specialization
template<>
struct TNumericLimits<int16>
{
public:
    typedef int16 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_INT16;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_INT16;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_INT16;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_INT16;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// uint16 specialization
template<>
struct TNumericLimits<uint16>
{
public:
    typedef uint16 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_UINT16;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_UINT16;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_UINT16;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_UINT16;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// int32 specialization
template<>
struct TNumericLimits<int32>
{
public:
    typedef int32 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_INT32;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_INT32;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_INT32;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_INT32;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// uint32 specialization
template<>
struct TNumericLimits<uint32>
{
public:
    typedef uint32 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_UINT32;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_UINT32;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_UINT32;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_UINT32;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// int64 specialization
template<>
struct TNumericLimits<int64>
{
public:
    typedef int64 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_INT64;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_INT64;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_INT64;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_INT64;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// uint64 specialization
template<>
struct TNumericLimits<uint64>
{
public:
    typedef uint64 ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_UINT64;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_UINT64;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_UINT64;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return LIMITS_MIN_UINT64;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return 0;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return ValueType();
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return ValueType();
    }
};

// float specialization
template<>
struct TNumericLimits<float>
{
public:
    typedef float ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_FLT;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_FLT;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_FLT;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return -LIMITS_MAX_FLT;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return LIMITS_EPS_FLT;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return INFINITY;
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return NAN;
    }
};

// double specialization
template<>
struct TNumericLimits<double>
{
public:
    typedef double ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_DBL;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_DBL;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_DBL;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return -LIMITS_MAX_DBL;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return LIMITS_EPS_DBL;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return INFINITY;
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return NAN;
    }
};

// long double specialization
template<>
struct TNumericLimits<long double>
{
public:
    typedef long double ValueType;

public:
    NODISCARD static constexpr int32 Digits() noexcept
    {
        return LIMITS_DIGITS_LDBL;
    }

    NODISCARD static constexpr ValueType Min() noexcept
    {
        return LIMITS_MIN_LDBL;
    }

    NODISCARD static constexpr ValueType Max() noexcept
    {
        return LIMITS_MAX_LDBL;
    }

    NODISCARD static constexpr ValueType Lowest() noexcept
    {
        return -LIMITS_MAX_LDBL;
    }

    NODISCARD static constexpr ValueType Epsilon() noexcept
    {
        return LIMITS_EPS_LDBL;
    }

    NODISCARD static constexpr ValueType Infinity() noexcept
    {
        return INFINITY;
    }

    NODISCARD static constexpr ValueType NaN() noexcept
    {
        return NAN;
    }
};
