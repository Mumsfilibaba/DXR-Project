#pragma once
#include "Core/CoreDefines.h"

namespace TimeUtilities
{
    template<typename T>
    CONSTEXPR T ToMicroseconds(T Nanoseconds) { return Nanoseconds / T(1000); }

    template<typename T>
    CONSTEXPR T ToMilliseconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000); }

    template<typename T>
    CONSTEXPR T ToSeconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000 * 1000); }

    template<typename T>
    CONSTEXPR T FromMicroseconds(T Microseconds) { return Microseconds * T(1000); }

    template<typename T>
    CONSTEXPR T FromMilliseconds(T Milliseconds) { return Milliseconds * T(1000 * 1000); }

    template<typename T>
    CONSTEXPR T FromSeconds(T Seconds) { return Seconds * T(1000 * 1000 * 1000); }
}
