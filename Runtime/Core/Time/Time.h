#pragma once
#include "Core/CoreDefines.h"

struct Time
{
    template<typename T>
    static constexpr T ToMicroseconds(T Nanoseconds) { return Nanoseconds / T(1000); }

    template<typename T>
    static constexpr T ToMilliseconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000); }

    template<typename T>
    static constexpr T ToSeconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000 * 1000); }

    template<typename T>
    static constexpr T FromMicroseconds(T Microseconds) { return Microseconds * T(1000); }

    template<typename T>
    static constexpr T FromMilliseconds(T Milliseconds) { return Milliseconds * T(1000 * 1000); }

    template<typename T>
    static constexpr T FromSeconds(T Seconds) { return Seconds * T(1000 * 1000 * 1000); }
};
