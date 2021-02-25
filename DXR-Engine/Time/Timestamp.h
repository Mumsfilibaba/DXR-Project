#pragma once
#include "Core.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4251)
#endif

class Timestamp
{
public:
    FORCEINLINE Timestamp(UInt64 Nanoseconds = 0)
        : TimestampInNS(Nanoseconds)
    {
    }

    FORCEINLINE Double AsSeconds() const
    {
        constexpr Double SECONDS = 1000.0 * 1000.0 * 1000.0;
        return Double(TimestampInNS) / SECONDS;
    }

    FORCEINLINE Double AsMilliSeconds() const
    {
        constexpr Double MILLISECONDS = 1000.0 * 1000.0;
        return Double(TimestampInNS) / MILLISECONDS;
    }

    FORCEINLINE Double AsMicroSeconds() const
    {
        constexpr Double MICROSECONDS = 1000.0;
        return Double(TimestampInNS) / MICROSECONDS;
    }

    FORCEINLINE UInt64 AsNanoSeconds() const
    {
        return TimestampInNS;
    }

    FORCEINLINE Bool operator==(const Timestamp& Other) const
    {
        return TimestampInNS == Other.TimestampInNS;
    }

    FORCEINLINE Bool operator!=(const Timestamp& Other) const
    {
        return TimestampInNS != Other.TimestampInNS;
    }

    FORCEINLINE Timestamp& operator+=(const Timestamp& Right)
    {
        TimestampInNS += Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator-=(const Timestamp& Right)
    {
        TimestampInNS -= Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator*=(const Timestamp& Right)
    {
        TimestampInNS *= Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator/=(const Timestamp& Right)
    {
        TimestampInNS /= Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE static Timestamp Seconds(Double Seconds)
    {
        constexpr Double SECOND = 1000.0 * 1000.0 * 1000.0;
        return Timestamp(static_cast<UInt64>(Seconds * SECOND));
    }

    FORCEINLINE static Timestamp MilliSeconds(Double Milliseconds)
    {
        constexpr Double MILLISECOND = 1000.0 * 1000.0;
        return Timestamp(static_cast<UInt64>(Milliseconds * MILLISECOND));
    }

    FORCEINLINE static Timestamp MicroSeconds(Double Microseconds)
    {
        constexpr Double MICROSECOND = 1000.0;
        return Timestamp(static_cast<UInt64>(Microseconds * MICROSECOND));
    }

    FORCEINLINE static Timestamp NanoSeconds(UInt64 Nanoseconds)
    {
        return Timestamp(Nanoseconds);
    }

public:
    FORCEINLINE friend Timestamp operator+(const Timestamp& Left, const Timestamp& Right)
    {
        return Timestamp(Left.TimestampInNS + Right.TimestampInNS);
    }

    FORCEINLINE friend Timestamp operator-(const Timestamp& Left, const Timestamp& Right)
    {
        return Timestamp(Left.TimestampInNS - Right.TimestampInNS);
    }

    FORCEINLINE friend Timestamp operator*(const Timestamp& Left, const Timestamp& Right)
    {
        return Timestamp(Left.TimestampInNS * Right.TimestampInNS);
    }

    FORCEINLINE friend Timestamp operator/(const Timestamp& Left, const Timestamp& Right)
    {
        return Timestamp(Left.TimestampInNS / Right.TimestampInNS);
    }

    FORCEINLINE friend Bool operator>(const Timestamp& Left, const Timestamp& Right)
    {
        return Left.TimestampInNS > Right.TimestampInNS;
    }

    FORCEINLINE friend Bool operator<(const Timestamp& Left, const Timestamp& Right)
    {
        return Left.TimestampInNS < Right.TimestampInNS;
    }

    FORCEINLINE friend Bool operator>=(const Timestamp& Left, const Timestamp& Right)
    {
        return (Left.TimestampInNS >= Right.TimestampInNS);
    }

    FORCEINLINE friend Bool operator<=(const Timestamp& Left, const Timestamp& Right)
    {
        return (Left.TimestampInNS <= Right.TimestampInNS);
    }

private:
    UInt64 TimestampInNS = 0;
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif