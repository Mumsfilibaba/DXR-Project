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
	uint64			Now		= 0;
	LARGE_INTEGER	Count	= { };
	if (::QueryPerformanceCounter(&Count))
	{
		Now = Count.QuadPart;
	}
	
	constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
	uint64 Delta		= Now - LastTime;
	uint64 Nanoseconds	= (Delta * NANOSECONDS) / Frequency;

	VALIDATE(LastTime < Now);

	DeltaTime	= Timestamp(Nanoseconds);
	LastTime	= Now;
	TotalTime	+= DeltaTime;
}