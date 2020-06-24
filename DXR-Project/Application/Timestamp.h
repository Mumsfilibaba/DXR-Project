#pragma once
#include "Defines.h"
#include "Types.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class Timestamp
{
public:
	FORCEINLINE Timestamp(Uint64 Nanoseconds = 0)
		: TimestampInNS(Nanoseconds)
	{
	}

	FORCEINLINE Float64 AsSeconds() const
	{
		constexpr Float64 SECONDS = 1000.0 * 1000.0 * 1000.0;
		return Float64(TimestampInNS) / SECONDS;
	}

	FORCEINLINE Float64 AsMilliSeconds() const
	{
		constexpr Float64 MILLISECONDS = 1000.0 * 1000.0;
		return Float64(TimestampInNS) / MILLISECONDS;
	}

	FORCEINLINE Float64 AsMicroSeconds() const
	{
		constexpr Float64 MICROSECONDS = 1000.0;
		return Float64(TimestampInNS) / MICROSECONDS;
	}

	FORCEINLINE Uint64 AsNanoSeconds() const
	{
		return TimestampInNS;
	}

	FORCEINLINE bool operator==(const Timestamp& Other) const
	{
		return TimestampInNS == Other.TimestampInNS;
	}

	FORCEINLINE bool operator!=(const Timestamp& Other) const
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

	FORCEINLINE static Timestamp Seconds(Float64 Seconds)
	{
		constexpr Float64 SECOND = 1000.0 * 1000.0 * 1000.0;
		return Timestamp(Float64(Seconds * SECOND));
	}

	FORCEINLINE static Timestamp MilliSeconds(Float64 Milliseconds)
	{
		constexpr Float64 MILLISECOND = 1000.0 * 1000.0;
		return Timestamp(Float64(Milliseconds * MILLISECOND));
	}

	FORCEINLINE static Timestamp MicroSeconds(Float64 Microseconds)
	{
		constexpr Float64 MICROSECOND = 1000.0;
		return Timestamp(Float64(Microseconds * MICROSECOND));
	}

	FORCEINLINE static Timestamp NanoSeconds(Uint64 Nanoseconds)
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

	FORCEINLINE friend bool operator>(const Timestamp& Left, const Timestamp& Right)
	{
		return Left.TimestampInNS > Right.TimestampInNS;
	}

	FORCEINLINE friend bool operator<(const Timestamp& Left, const Timestamp& Right)
	{
		return Left.TimestampInNS < Right.TimestampInNS;
	}

	FORCEINLINE friend bool operator>=(const Timestamp& Left, const Timestamp& Right)
	{
		return (Left.TimestampInNS >= Right.TimestampInNS);
	}

	FORCEINLINE friend bool operator<=(const Timestamp& Left, const Timestamp& Right)
	{
		return (Left.TimestampInNS <= Right.TimestampInNS);
	}

private:
	Uint64 TimestampInNS = 0;
};

#pragma warning(pop)
