#pragma once
#include "Time/Clock.h"

#include <unordered_map>

#define ENABLE_PROFILER 1

#if ENABLE_PROFILER
	#define TRACE_SCOPE(Name)		ScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(GlobalProfiler, Name)
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

		Sample(Int64 InCurrentSample)
			: CurrentSample(InCurrentSample)
			, SampleCount(1)
		{
		}

		FORCEINLINE void AddSample(Int64 NewSample)
		{
			CurrentSample = CurrentSample + (NewSample - CurrentSample) / SampleCount;
			SampleCount++;
		}

		Int64 CurrentSample = 0;
		Int64 SampleCount	= 1;
	};

public:
	Profiler();

	void BeginFrame();
	void EndFrame();

	void AddSample(const Char* Name, Int64 Nanoseconds);

	void DrawUI();

private:
	Clock	Clock;
	Sample	FrameTime;

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
			const Int64 Nanoseconds = (UInt64)Clock.GetDeltaTime().AsNanoSeconds();
			Profiler->AddSample(Name, Nanoseconds);
		}
	}

private:
	Profiler*	Profiler	= nullptr;
	const Char*	Name		= nullptr;
	Clock		Clock;
};