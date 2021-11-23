#pragma once
#include "Core/CoreModule.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace NTime
{
    /* Returns the number of microseconds from nanoseconds */
    template<typename T>
    inline constexpr T ToMicroseconds( T Nanoseconds )
    {
        return Nanoseconds / T( 1000 );
    }

    /* Returns the number of milliseconds from nanoseconds */
    template<typename T>
    inline constexpr T ToMilliseconds( T Nanoseconds )
    {
        return Nanoseconds / T( 1000 * 1000 );
    }

    /* Returns the number of seconds from nanoseconds */
    template<typename T>
    inline constexpr T ToSeconds( T Nanoseconds )
    {
        return Nanoseconds / T( 1000 * 1000 * 1000 );
    }

    /* Returns the number of nanoseconds from microseconds */
    template<typename T>
    inline constexpr T FromMicroseconds( T Microseconds )
    {
        return Microseconds * T( 1000 );
    }

    /* Returns the number of nanoseconds from milliseconds */
    template<typename T>
    inline constexpr T FromMilliseconds( T Milliseconds )
    {
        return Milliseconds * T( 1000 * 1000 );
    }

    /* Returns the number of nanoseconds from seconds */
    template<typename T>
    inline constexpr T FromSeconds( T Seconds )
    {
        return Seconds * T( 1000 * 1000 * 1000 );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

class CTimestamp
{
public:

    FORCEINLINE CTimestamp( uint64 InNanoseconds = 0 )
        : Nanoseconds( InNanoseconds )
    {
    }

    FORCEINLINE double AsSeconds() const
    {
        return NTime::ToSeconds( static_cast<double>(Nanoseconds) );
    }

    FORCEINLINE double AsMilliSeconds() const
    {
        return NTime::ToMilliseconds( static_cast<double>(Nanoseconds) );
    }

    FORCEINLINE double AsMicroSeconds() const
    {
        return NTime::ToMicroseconds( static_cast<double>(Nanoseconds) );
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
        return CTimestamp( static_cast<uint64>(NTime::FromSeconds( Seconds )) );
    }

    static FORCEINLINE CTimestamp MilliSeconds( double Milliseconds )
    {
        return CTimestamp( static_cast<uint64>(NTime::FromMilliseconds( Milliseconds )) );
    }

    static FORCEINLINE CTimestamp MicroSeconds( double Microseconds )
    {
        return CTimestamp( static_cast<uint64>(NTime::FromMicroseconds( Microseconds )) );
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