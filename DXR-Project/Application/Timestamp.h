#pragma once
#include "Defines.h"
#include "Types.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class Timestamp
{
public:
	FORCEINLINE Timestamp(Uint64 InNanoseconds = 0)
		: TimestampInNS(InNanoseconds)
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

	FORCEINLINE bool operator==(const Timestamp& InOther) const
	{
		return TimestampInNS == InOther.TimestampInNS;
	}

	FORCEINLINE bool operator!=(const Timestamp& InOther) const
	{
		return TimestampInNS != InOther.TimestampInNS;
	}

	FORCEINLINE Timestamp& operator+=(const Timestamp& InRight)
	{
		TimestampInNS += InRight.TimestampInNS;
		return *this;
	}

	FORCEINLINE Timestamp& operator-=(const Timestamp& InRight)
	{
		TimestampInNS -= InRight.TimestampInNS;
		return *this;
	}

	FORCEINLINE Timestamp& operator*=(const Timestamp& InRight)
	{
		TimestampInNS *= InRight.TimestampInNS;
		return *this;
	}

	FORCEINLINE Timestamp& operator/=(const Timestamp& InRight)
	{
		TimestampInNS /= InRight.TimestampInNS;
		return *this;
	}

	FORCEINLINE static Timestamp Seconds(Float64 InSeconds)
	{
		constexpr Float64 SECOND = 1000.0 * 1000.0 * 1000.0;
		return Timestamp(Float64(InSeconds * SECOND));
	}

	FORCEINLINE static Timestamp MilliSeconds(Float64 InMilliseconds)
	{
		constexpr Float64 MILLISECOND = 1000.0 * 1000.0;
		return Timestamp(Float64(InMilliseconds * MILLISECOND));
	}

	FORCEINLINE static Timestamp MicroSeconds(Float64 InMicroseconds)
	{
		constexpr Float64 MICROSECOND = 1000.0;
		return Timestamp(Float64(InMicroseconds * MICROSECOND));
	}

	FORCEINLINE static Timestamp NanoSeconds(Uint64 InNanoseconds)
	{
		return Timestamp(InNanoseconds);
	}

public:
	FORCEINLINE friend Timestamp operator+(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return Timestamp(InLeft.TimestampInNS + InRight.TimestampInNS);
	}

	FORCEINLINE friend Timestamp operator-(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return Timestamp(InLeft.TimestampInNS - InRight.TimestampInNS);
	}

	FORCEINLINE friend Timestamp operator*(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return Timestamp(InLeft.TimestampInNS * InRight.TimestampInNS);
	}

	FORCEINLINE friend Timestamp operator/(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return Timestamp(InLeft.TimestampInNS / InRight.TimestampInNS);
	}

	FORCEINLINE friend bool operator>(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return InLeft.TimestampInNS > InRight.TimestampInNS;
	}

	FORCEINLINE friend bool operator<(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return InLeft.TimestampInNS < InRight.TimestampInNS;
	}

	FORCEINLINE friend bool operator>=(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return (InLeft.TimestampInNS >= InRight.TimestampInNS);
	}

	FORCEINLINE friend bool operator<=(const Timestamp& InLeft, const Timestamp& InRight)
	{
		return (InLeft.TimestampInNS <= InRight.TimestampInNS);
	}

private:
	Uint64 TimestampInNS = 0;
};

#pragma warning(pop)
