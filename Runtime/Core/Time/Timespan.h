#pragma once
#include "TimeUtilities.h"
#include "Core/Core.h"

class FTimespan
{
public:
    FORCEINLINE FTimespan(uint64 InTimespanNS = 0)
        : TimespanNS(InTimespanNS)
    {
    }

    FORCEINLINE double AsSeconds() const
    {
        return TimeUtilities::ToSeconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE double AsMilliseconds() const
    {
        return TimeUtilities::ToMilliseconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE double AsMicroseconds() const
    {
        return TimeUtilities::ToMicroseconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE uint64 AsNanoseconds() const
    {
        return TimespanNS;
    }

public:
    FORCEINLINE bool operator==(const FTimespan& Other) const
    {
        return TimespanNS == Other.TimespanNS;
    }

    FORCEINLINE bool operator!=(const FTimespan& Other) const
    {
        return TimespanNS != Other.TimespanNS;
    }

    FORCEINLINE FTimespan& operator+=(const FTimespan& Right)
    {
        TimespanNS += Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimespan& operator-=(const FTimespan& Right)
    {
        TimespanNS -= Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimespan& operator*=(const FTimespan& Right)
    {
        TimespanNS *= Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimespan& operator/=(const FTimespan& Right)
    {
        TimespanNS /= Right.TimespanNS;
        return *this;
    }

public:
    static FORCEINLINE FTimespan Seconds(double InSeconds)
    {
        return FTimespan(static_cast<uint64>(TimeUtilities::FromSeconds(InSeconds)));
    }

    static FORCEINLINE FTimespan Milliseconds(double InMilliseconds)
    {
        return FTimespan(static_cast<uint64>(TimeUtilities::FromMilliseconds(InMilliseconds)));
    }

    static FORCEINLINE FTimespan Microseconds(double InMicroseconds)
    {
        return FTimespan(static_cast<uint64>(TimeUtilities::FromMicroseconds(InMicroseconds)));
    }

    static FORCEINLINE FTimespan Nanoseconds(uint64 InNanoseconds)
    {
        return FTimespan(InNanoseconds);
    }

    static FORCEINLINE FTimespan Infinity()
    {
        return FTimespan(uint64(~0));
    }

public:
    FORCEINLINE friend FTimespan operator+(const FTimespan& Left, const FTimespan& Right)
    {
        return FTimespan(Left.TimespanNS + Right.TimespanNS);
    }

    FORCEINLINE friend FTimespan operator-(const FTimespan& Left, const FTimespan& Right)
    {
        return FTimespan(Left.TimespanNS - Right.TimespanNS);
    }

    FORCEINLINE friend FTimespan operator*(const FTimespan& Left, const FTimespan& Right)
    {
        return FTimespan(Left.TimespanNS * Right.TimespanNS);
    }

    FORCEINLINE friend FTimespan operator/(const FTimespan& Left, const FTimespan& Right)
    {
        return FTimespan(Left.TimespanNS / Right.TimespanNS);
    }

    FORCEINLINE friend bool operator>(const FTimespan& Left, const FTimespan& Right)
    {
        return Left.TimespanNS > Right.TimespanNS;
    }

    FORCEINLINE friend bool operator<(const FTimespan& Left, const FTimespan& Right)
    {
        return Left.TimespanNS < Right.TimespanNS;
    }

    FORCEINLINE friend bool operator>=(const FTimespan& Left, const FTimespan& Right)
    {
        return (Left.TimespanNS >= Right.TimespanNS);
    }

    FORCEINLINE friend bool operator<=(const FTimespan& Left, const FTimespan& Right)
    {
        return (Left.TimespanNS <= Right.TimespanNS);
    }

private:
    uint64 TimespanNS = 0;
};
