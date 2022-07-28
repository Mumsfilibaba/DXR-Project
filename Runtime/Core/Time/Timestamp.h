#pragma once
#include "TimeUtilities.h"

#include "Core/Core.h"

#ifdef COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4251)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTimestamp

class FTimestamp
{
public:
    FORCEINLINE FTimestamp(uint64 InTimespanNS = 0)
        : TimespanNS(InTimespanNS)
    { }

    FORCEINLINE double AsSeconds() const
    {
        return NTime::ToSeconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE double AsMilliseconds() const
    {
        return NTime::ToMilliseconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE double AsMicroseconds() const
    {
        return NTime::ToMicroseconds(static_cast<double>(TimespanNS));
    }

    FORCEINLINE uint64 AsNanoseconds() const
    {
        return TimespanNS;
    }

public:
    FORCEINLINE bool operator==(const FTimestamp& Other) const
    {
        return TimespanNS == Other.TimespanNS;
    }

    FORCEINLINE bool operator!=(const FTimestamp& Other) const
    {
        return TimespanNS != Other.TimespanNS;
    }

    FORCEINLINE FTimestamp& operator+=(const FTimestamp& Right)
    {
        TimespanNS += Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimestamp& operator-=(const FTimestamp& Right)
    {
        TimespanNS -= Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimestamp& operator*=(const FTimestamp& Right)
    {
        TimespanNS *= Right.TimespanNS;
        return *this;
    }

    FORCEINLINE FTimestamp& operator/=(const FTimestamp& Right)
    {
        TimespanNS /= Right.TimespanNS;
        return *this;
    }

public:
    static FORCEINLINE FTimestamp Seconds(double InSeconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromSeconds(InSeconds)));
    }

    static FORCEINLINE FTimestamp Milliseconds(double InMilliseconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromMilliseconds(InMilliseconds)));
    }

    static FORCEINLINE FTimestamp Microseconds(double InMicroseconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromMicroseconds(InMicroseconds)));
    }

    static FORCEINLINE FTimestamp Nanoseconds(uint64 InNanoseconds)
    {
        return FTimestamp(InNanoseconds);
    }

    static FORCEINLINE FTimestamp Infinity()
    {
        return FTimestamp(uint64(~0));
    }

public:
    FORCEINLINE friend FTimestamp operator+(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.TimespanNS + Right.TimespanNS);
    }

    FORCEINLINE friend FTimestamp operator-(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.TimespanNS - Right.TimespanNS);
    }

    FORCEINLINE friend FTimestamp operator*(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.TimespanNS * Right.TimespanNS);
    }

    FORCEINLINE friend FTimestamp operator/(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.TimespanNS / Right.TimespanNS);
    }

    FORCEINLINE friend bool operator>(const FTimestamp& Left, const FTimestamp& Right)
    {
        return Left.TimespanNS > Right.TimespanNS;
    }

    FORCEINLINE friend bool operator<(const FTimestamp& Left, const FTimestamp& Right)
    {
        return Left.TimespanNS < Right.TimespanNS;
    }

    FORCEINLINE friend bool operator>=(const FTimestamp& Left, const FTimestamp& Right)
    {
        return (Left.TimespanNS >= Right.TimespanNS);
    }

    FORCEINLINE friend bool operator<=(const FTimestamp& Left, const FTimestamp& Right)
    {
        return (Left.TimespanNS <= Right.TimespanNS);
    }

private:
    uint64 TimespanNS = 0;
};

#ifdef COMPILER_MSVC
    #pragma warning(pop)
#endif