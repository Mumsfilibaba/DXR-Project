#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

#include <limits>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNumericLimits

template<typename T>
class TNumericLimits;

template<>
struct TNumericLimits<bool>
{
    NODISCARD static CONSTEXPR bool Min() noexcept 
    { 
        return false; 
    }

    NODISCARD static CONSTEXPR bool Max() noexcept
    { 
        return true; 
    }

    NODISCARD static CONSTEXPR bool Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int8>
{
    NODISCARD static CONSTEXPR int8 Min() noexcept 
    { 
        return int8(-128); 
    }

    NODISCARD static CONSTEXPR int8 Max() noexcept 
    { 
        return int8(127);
    }

    NODISCARD static CONSTEXPR int8 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint8>
{
    NODISCARD static CONSTEXPR uint8 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static CONSTEXPR uint8 Max() noexcept 
    { 
        return uint8(0xff);
    }

    NODISCARD static CONSTEXPR uint8 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int16>
{
    NODISCARD static CONSTEXPR int16 Min() noexcept 
    { 
        return int16(-32768);
    }

    NODISCARD static CONSTEXPR int16 Max() noexcept 
    { 
        return int16(32767);
    }

    NODISCARD static CONSTEXPR int16 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint16>
{
    NODISCARD static CONSTEXPR uint16 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static CONSTEXPR uint16 Max() noexcept 
    { 
        return uint16(0xffff);
    }

    NODISCARD static CONSTEXPR uint16 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int32>
{
    NODISCARD static CONSTEXPR int32 Min() noexcept 
    { 
        return int32(-2147483647 - 1);
    }

    NODISCARD static CONSTEXPR int32 Max() noexcept 
    { 
        return int32(2147483647);
    }

    NODISCARD static CONSTEXPR int32 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint32>
{
    NODISCARD static CONSTEXPR uint32 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static CONSTEXPR uint32 Max() noexcept 
    { 
        return uint32(0xffffffff);
    }

    NODISCARD static CONSTEXPR uint32 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int64>
{
    NODISCARD static CONSTEXPR int64 Min() noexcept 
    { 
        return int64(-9223372036854775807 - 1);
    }

    NODISCARD static CONSTEXPR int64 Max() noexcept 
    { 
        return int64(9223372036854775807); 
    }

    NODISCARD static CONSTEXPR int64 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint64>
{
    NODISCARD static CONSTEXPR uint64 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static CONSTEXPR uint64 Max() noexcept 
    { 
        return uint64(0xffffffffffffffff);
    }

    NODISCARD static CONSTEXPR uint64 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<float>
{
    NODISCARD static CONSTEXPR float Min() noexcept 
    { 
        return float(1.175494351e-38F);
    }

    NODISCARD static CONSTEXPR float Max() noexcept 
    { 
        return float(3.402823466e+38F);
    }

    NODISCARD static CONSTEXPR float Lowest() noexcept 
    { 
        return -Min();
    }
};

template<>
struct TNumericLimits<double>
{
    NODISCARD static CONSTEXPR double Min() noexcept 
    { 
        return double(2.2250738585072014e-308); 
    }

    NODISCARD static CONSTEXPR double Max() noexcept 
    { 
        return double(1.7976931348623158e+308); 
    }

    NODISCARD static CONSTEXPR double Lowest() noexcept 
    { 
        return -Min();
    }
};
