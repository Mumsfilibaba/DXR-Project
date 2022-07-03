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

    FORCEINLINE FTimestamp(uint64 InNanoseconds = 0)
        : Nanoseconds(InNanoseconds)
    { }

    FORCEINLINE double AsSeconds() const
    {
        return NTime::ToSeconds(static_cast<double>(Nanoseconds));
    }

    FORCEINLINE double AsMilliSeconds() const
    {
        return NTime::ToMilliseconds(static_cast<double>(Nanoseconds));
    }

    FORCEINLINE double AsMicroSeconds() const
    {
        return NTime::ToMicroseconds(static_cast<double>(Nanoseconds));
    }

    FORCEINLINE uint64 AsNanoSeconds() const
    {
        return Nanoseconds;
    }

    FORCEINLINE bool operator==(const FTimestamp& Other) const
    {
        return Nanoseconds == Other.Nanoseconds;
    }

    FORCEINLINE bool operator!=(const FTimestamp& Other) const
    {
        return Nanoseconds != Other.Nanoseconds;
    }

    FORCEINLINE FTimestamp& operator+=(const FTimestamp& Right)
    {
        Nanoseconds += Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE FTimestamp& operator-=(const FTimestamp& Right)
    {
        Nanoseconds -= Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE FTimestamp& operator*=(const FTimestamp& Right)
    {
        Nanoseconds *= Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE FTimestamp& operator/=(const FTimestamp& Right)
    {
        Nanoseconds /= Right.Nanoseconds;
        return *this;
    }

    static FORCEINLINE FTimestamp Seconds(double Seconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromSeconds(Seconds)));
    }

    static FORCEINLINE FTimestamp MilliSeconds(double Milliseconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromMilliseconds(Milliseconds)));
    }

    static FORCEINLINE FTimestamp MicroSeconds(double Microseconds)
    {
        return FTimestamp(static_cast<uint64>(NTime::FromMicroseconds(Microseconds)));
    }

    static FORCEINLINE FTimestamp NanoSeconds(uint64 Nanoseconds)
    {
        return FTimestamp(Nanoseconds);
    }

public:

    FORCEINLINE friend FTimestamp operator+(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.Nanoseconds + Right.Nanoseconds);
    }

    FORCEINLINE friend FTimestamp operator-(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.Nanoseconds - Right.Nanoseconds);
    }

    FORCEINLINE friend FTimestamp operator*(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.Nanoseconds * Right.Nanoseconds);
    }

    FORCEINLINE friend FTimestamp operator/(const FTimestamp& Left, const FTimestamp& Right)
    {
        return FTimestamp(Left.Nanoseconds / Right.Nanoseconds);
    }

    FORCEINLINE friend bool operator>(const FTimestamp& Left, const FTimestamp& Right)
    {
        return Left.Nanoseconds > Right.Nanoseconds;
    }

    FORCEINLINE friend bool operator<(const FTimestamp& Left, const FTimestamp& Right)
    {
        return Left.Nanoseconds < Right.Nanoseconds;
    }

    FORCEINLINE friend bool operator>=(const FTimestamp& Left, const FTimestamp& Right)
    {
        return (Left.Nanoseconds >= Right.Nanoseconds);
    }

    FORCEINLINE friend bool operator<=(const FTimestamp& Left, const FTimestamp& Right)
    {
        return (Left.Nanoseconds <= Right.Nanoseconds);
    }

private:
    uint64 Nanoseconds = 0;
};

#ifdef COMPILER_MSVC
    #pragma warning(pop)
#endif