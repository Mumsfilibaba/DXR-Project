#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Time conversion utilities

namespace NTime
{
    template<typename T>
    inline constexpr T ToMicroseconds(T Nanoseconds) { return Nanoseconds / T(1000); }

    template<typename T>
    inline constexpr T ToMilliseconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000); }

    template<typename T>
    inline constexpr T ToSeconds(T Nanoseconds) { return Nanoseconds / T(1000 * 1000 * 1000); }

    template<typename T>
    inline constexpr T FromMicroseconds(T Microseconds) { return Microseconds * T(1000); }

    template<typename T>
    inline constexpr T FromMilliseconds(T Milliseconds) { return Milliseconds * T(1000 * 1000); }

    template<typename T>
    inline constexpr T FromSeconds(T Seconds) { return Seconds * T(1000 * 1000 * 1000); }
}