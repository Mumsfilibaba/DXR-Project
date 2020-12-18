#include "Clock.h"

#include "Windows/Windows.h"

Clock::Clock()
{
	LARGE_INTEGER Freq = { };
	if (::QueryPerformanceFrequency(&Freq))
	{
		Frequency = Freq.QuadPart;
	}

	Tick();
}

void Clock::Tick()
{
	UInt64			Now		= 0;
	LARGE_INTEGER	Count	= { };
	if (::QueryPerformanceCounter(&Count))
	{
		Now = Count.QuadPart;
	}
	
	constexpr UInt64 NANOSECONDS = 1000 * 1000 * 1000;
	UInt64 Delta		= Now - LastTime;
	UInt64 Nanoseconds	= (Delta * NANOSECONDS) / Frequency;

	VALIDATE(LastTime < Now);

	DeltaTime	= Timestamp(Nanoseconds);
	LastTime	= Now;
	TotalTime	+= DeltaTime;
}