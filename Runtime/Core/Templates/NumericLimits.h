#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

#include <limits>

template<typename T>
struct TNumericLimits;

template<>
struct TNumericLimits<bool>
{
    NODISCARD static constexpr bool Min() noexcept 
    { 
        return false; 
    }

    NODISCARD static constexpr bool Max() noexcept
    { 
        return true; 
    }

    NODISCARD static constexpr bool Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int8>
{
    NODISCARD static constexpr int8 Min() noexcept 
    { 
        return int8(-128); 
    }

    NODISCARD static constexpr int8 Max() noexcept 
    { 
        return int8(127);
    }

    NODISCARD static constexpr int8 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint8>
{
    NODISCARD static constexpr uint8 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static constexpr uint8 Max() noexcept 
    { 
        return uint8(0xff);
    }

    NODISCARD static constexpr uint8 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int16>
{
    NODISCARD static constexpr int16 Min() noexcept 
    { 
        return int16(-32768);
    }

    NODISCARD static constexpr int16 Max() noexcept 
    { 
        return int16(32767);
    }

    NODISCARD static constexpr int16 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint16>
{
    NODISCARD static constexpr uint16 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static constexpr uint16 Max() noexcept 
    { 
        return uint16(0xffff);
    }

    NODISCARD static constexpr uint16 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int32>
{
    NODISCARD static constexpr int32 Min() noexcept 
    { 
        return int32(-2147483647 - 1);
    }

    NODISCARD static constexpr int32 Max() noexcept 
    { 
        return int32(2147483647);
    }

    NODISCARD static constexpr int32 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint32>
{
    NODISCARD static constexpr uint32 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static constexpr uint32 Max() noexcept 
    { 
        return uint32(0xffffffff);
    }

    NODISCARD static constexpr uint32 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<int64>
{
    NODISCARD static constexpr int64 Min() noexcept 
    { 
        return int64(-9223372036854775807 - 1);
    }

    NODISCARD static constexpr int64 Max() noexcept 
    { 
        return int64(9223372036854775807); 
    }

    NODISCARD static constexpr int64 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<uint64>
{
    NODISCARD static constexpr uint64 Min() noexcept 
    { 
        return 0;
    }

    NODISCARD static constexpr uint64 Max() noexcept 
    { 
        return uint64(0xffffffffffffffff);
    }

    NODISCARD static constexpr uint64 Lowest() noexcept 
    { 
        return Min();
    }
};

template<>
struct TNumericLimits<float>
{
    NODISCARD static constexpr float Min() noexcept 
    { 
        return float(1.175494351e-38F);
    }

    NODISCARD static constexpr float Max() noexcept 
    { 
        return float(3.402823466e+38F);
    }

    NODISCARD static constexpr float Lowest() noexcept 
    { 
        return -Min();
    }
};

template<>
struct TNumericLimits<double>
{
    NODISCARD static constexpr double Min() noexcept 
    { 
        return double(2.2250738585072014e-308); 
    }

    NODISCARD static constexpr double Max() noexcept 
    { 
        return double(1.7976931348623158e+308); 
    }

    NODISCARD static constexpr double Lowest() noexcept 
    { 
        return -Min();
    }
};
