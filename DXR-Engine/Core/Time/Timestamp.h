#pragma once
#include "Core.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class CTimestamp
{
public:

    FORCEINLINE CTimestamp( uint64 InNanoseconds = 0 )
        : Nanoseconds( InNanoseconds )
    {
    }

    FORCEINLINE double AsSeconds() const
    {
        constexpr double SECONDS = 1000.0 * 1000.0 * 1000.0;
        return double( Nanoseconds ) / SECONDS;
    }

    FORCEINLINE double AsMilliSeconds() const
    {
        constexpr double MILLISECONDS = 1000.0 * 1000.0;
        return double( Nanoseconds ) / MILLISECONDS;
    }

    FORCEINLINE double AsMicroSeconds() const
    {
        constexpr double MICROSECONDS = 1000.0;
        return double( Nanoseconds ) / MICROSECONDS;
    }

    FORCEINLINE uint64 AsNanoSeconds() const
    {
        return Nanoseconds;
    }

    FORCEINLINE bool operator==( const CTimestamp& Other ) const
    {
        return Nanoseconds == Other.Nanoseconds;
    }

    FORCEINLINE bool operator!=( const CTimestamp& Other ) const
    {
        return Nanoseconds != Other.Nanoseconds;
    }

    FORCEINLINE CTimestamp& operator+=( const CTimestamp& Right )
    {
        Nanoseconds += Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE CTimestamp& operator-=( const CTimestamp& Right )
    {
        Nanoseconds -= Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE CTimestamp& operator*=( const CTimestamp& Right )
    {
        Nanoseconds *= Right.Nanoseconds;
        return *this;
    }

    FORCEINLINE CTimestamp& operator/=( const CTimestamp& Right )
    {
        Nanoseconds /= Right.Nanoseconds;
        return *this;
    }

    static FORCEINLINE CTimestamp Seconds( double Seconds )
    {
        constexpr double SECOND = 1000.0 * 1000.0 * 1000.0;
        return CTimestamp( static_cast<uint64>(Seconds * SECOND) );
    }

    static FORCEINLINE CTimestamp MilliSeconds( double Milliseconds )
    {
        constexpr double MILLISECOND = 1000.0 * 1000.0;
        return CTimestamp( static_cast<uint64>(Milliseconds * MILLISECOND) );
    }

    static FORCEINLINE CTimestamp MicroSeconds( double Microseconds )
    {
        constexpr double MICROSECOND = 1000.0;
        return CTimestamp( static_cast<uint64>(Microseconds * MICROSECOND) );
    }

    static FORCEINLINE CTimestamp NanoSeconds( uint64 Nanoseconds )
    {
        return CTimestamp( Nanoseconds );
    }

public:

    FORCEINLINE friend CTimestamp operator+( const CTimestamp& Left, const CTimestamp& Right )
    {
        return CTimestamp( Left.Nanoseconds + Right.Nanoseconds );
    }

    FORCEINLINE friend CTimestamp operator-( const CTimestamp& Left, const CTimestamp& Right )
    {
        return CTimestamp( Left.Nanoseconds - Right.Nanoseconds );
    }

    FORCEINLINE friend CTimestamp operator*( const CTimestamp& Left, const CTimestamp& Right )
    {
        return CTimestamp( Left.Nanoseconds * Right.Nanoseconds );
    }

    FORCEINLINE friend CTimestamp operator/( const CTimestamp& Left, const CTimestamp& Right )
    {
        return CTimestamp( Left.Nanoseconds / Right.Nanoseconds );
    }

    FORCEINLINE friend bool operator>( const CTimestamp& Left, const CTimestamp& Right )
    {
        return Left.Nanoseconds > Right.Nanoseconds;
    }

    FORCEINLINE friend bool operator<( const CTimestamp& Left, const CTimestamp& Right )
    {
        return Left.Nanoseconds < Right.Nanoseconds;
    }

    FORCEINLINE friend bool operator>=( const CTimestamp& Left, const CTimestamp& Right )
    {
        return (Left.Nanoseconds >= Right.Nanoseconds);
    }

    FORCEINLINE friend bool operator<=( const CTimestamp& Left, const CTimestamp& Right )
    {
        return (Left.Nanoseconds <= Right.Nanoseconds);
    }

private:
    uint64 Nanoseconds = 0;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif