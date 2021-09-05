#pragma once
#include "Core.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class Timestamp
{
public:
    FORCEINLINE Timestamp( uint64 Nanoseconds = 0 )
        : TimestampInNS( Nanoseconds )
    {
    }

    FORCEINLINE double AsSeconds() const
    {
        constexpr double SECONDS = 1000.0 * 1000.0 * 1000.0;
        return double( TimestampInNS ) / SECONDS;
    }

    FORCEINLINE double AsMilliSeconds() const
    {
        constexpr double MILLISECONDS = 1000.0 * 1000.0;
        return double( TimestampInNS ) / MILLISECONDS;
    }

    FORCEINLINE double AsMicroSeconds() const
    {
        constexpr double MICROSECONDS = 1000.0;
        return double( TimestampInNS ) / MICROSECONDS;
    }

    FORCEINLINE uint64 AsNanoSeconds() const
    {
        return TimestampInNS;
    }

    FORCEINLINE bool operator==( const Timestamp& Other ) const
    {
        return TimestampInNS == Other.TimestampInNS;
    }

    FORCEINLINE bool operator!=( const Timestamp& Other ) const
    {
        return TimestampInNS != Other.TimestampInNS;
    }

    FORCEINLINE Timestamp& operator+=( const Timestamp& Right )
    {
        TimestampInNS += Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator-=( const Timestamp& Right )
    {
        TimestampInNS -= Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator*=( const Timestamp& Right )
    {
        TimestampInNS *= Right.TimestampInNS;
        return *this;
    }

    FORCEINLINE Timestamp& operator/=( const Timestamp& Right )
    {
        TimestampInNS /= Right.TimestampInNS;
        return *this;
    }

    static FORCEINLINE Timestamp Seconds( double Seconds )
    {
        constexpr double SECOND = 1000.0 * 1000.0 * 1000.0;
        return Timestamp( static_cast<uint64>(Seconds * SECOND) );
    }

    static FORCEINLINE Timestamp MilliSeconds( double Milliseconds )
    {
        constexpr double MILLISECOND = 1000.0 * 1000.0;
        return Timestamp( static_cast<uint64>(Milliseconds * MILLISECOND) );
    }

    static FORCEINLINE Timestamp MicroSeconds( double Microseconds )
    {
        constexpr double MICROSECOND = 1000.0;
        return Timestamp( static_cast<uint64>(Microseconds * MICROSECOND) );
    }

    static FORCEINLINE Timestamp NanoSeconds( uint64 Nanoseconds )
    {
        return Timestamp( Nanoseconds );
    }

public:
    FORCEINLINE friend Timestamp operator+( const Timestamp& Left, const Timestamp& Right )
    {
        return Timestamp( Left.TimestampInNS + Right.TimestampInNS );
    }

    FORCEINLINE friend Timestamp operator-( const Timestamp& Left, const Timestamp& Right )
    {
        return Timestamp( Left.TimestampInNS - Right.TimestampInNS );
    }

    FORCEINLINE friend Timestamp operator*( const Timestamp& Left, const Timestamp& Right )
    {
        return Timestamp( Left.TimestampInNS * Right.TimestampInNS );
    }

    FORCEINLINE friend Timestamp operator/( const Timestamp& Left, const Timestamp& Right )
    {
        return Timestamp( Left.TimestampInNS / Right.TimestampInNS );
    }

    FORCEINLINE friend bool operator>( const Timestamp& Left, const Timestamp& Right )
    {
        return Left.TimestampInNS > Right.TimestampInNS;
    }

    FORCEINLINE friend bool operator<( const Timestamp& Left, const Timestamp& Right )
    {
        return Left.TimestampInNS < Right.TimestampInNS;
    }

    FORCEINLINE friend bool operator>=( const Timestamp& Left, const Timestamp& Right )
    {
        return (Left.TimestampInNS >= Right.TimestampInNS);
    }

    FORCEINLINE friend bool operator<=( const Timestamp& Left, const Timestamp& Right )
    {
        return (Left.TimestampInNS <= Right.TimestampInNS);
    }

private:
    uint64 TimestampInNS = 0;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif