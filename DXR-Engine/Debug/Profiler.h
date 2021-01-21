#pragma once
#include "Time/Clock.h"

#include <unordered_map>

#define ENABLE_PROFILER			1
#define NUM_PROFILER_SAMPLES	75

#if ENABLE_PROFILER
	#define TRACE_SCOPE(Name)		ScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(&GlobalProfiler, Name)
	#define TRACE_FUNCTION_SCOPE()	TRACE_SCOPE(__FUNCTION_SIG__)
#else
	#define TRACE_SCOPE(Name)
	#define TRACE_FUNCTION_SCOPE()
#endif

/*
* Profiler
*/

class Profiler
{
	struct Sample
	{
		Sample() = default;

		Sample(Float FirstSample)
			: CurrentSample(1)
			, SampleCount(1)
			, Samples()
		{
			Samples.Front() = FirstSample;
		}

		FORCEINLINE void AddSample(Float NewSample)
		{
			Samples[CurrentSample] = NewSample;
			CurrentSample++;
			SampleCount = Math::Min<Int32>(Samples.Size(), SampleCount + 1);

			if (CurrentSample >= Int32(Samples.Size()))
			{
				CurrentSample	= 0;
			}
		}

		FORCEINLINE Float GetAverage() const
		{
			Float Average = 0.0f;
			for (Int32 n = 0; n < SampleCount; n++)
			{
				Average += Samples[n];
			}
			
			return Average / Float(SampleCount);
		}

		TStaticArray<Float, NUM_PROFILER_SAMPLES> Samples;
		Int32 SampleCount	= 0;
		Int32 CurrentSample	= 0;
	};

public:
	Profiler();
	~Profiler() = default;

	void Init();
	void Tick();

	void AddSample(const Char* Name, Float Sample);
	
private:
	Clock	Clock;
	Sample	FrameTime;
	Int32	Fps			= 0;
	Int32	CurrentFps	= 0;
	std::unordered_map<std::string, Sample> Samples;
};

/*
* ScopedTrace
*/

struct ScopedTrace
{
public:
	ScopedTrace(Profiler* InProfiler, const Char* InName)
		: Profiler(InProfiler)
		, Name(InName)
		, Clock()
	{
		Clock.Tick();
	}

	~ScopedTrace()
	{
		Clock.Tick();
		
		if (Profiler)
		{
			const Float Nanoseconds = Float(Clock.GetDeltaTime().AsNanoSeconds());
			Profiler->AddSample(Name, Nanoseconds);
		}
	}

private:
	Profiler*	Profiler	= nullptr;
	const Char*	Name		= nullptr;
	Clock		Clock;
};