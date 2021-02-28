#pragma once
#include "Time/Clock.h"

#include <unordered_map>

#define ENABLE_PROFILER      1
#define NUM_PROFILER_SAMPLES 200

#if ENABLE_PROFILER
    #define TRACE_SCOPE(Name)      ScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(__FUNCTION_SIG__)
#else
    #define TRACE_SCOPE(Name)
    #define TRACE_FUNCTION_SCOPE()
#endif

class Profiler
{
public:
    static void Init();
    static void Tick();

    static void BeginTraceScope(const Char* Name);
    static void EndTraceScope(const Char* Name);
    
    static void SetGPUProfiler(class GPUProfiler* Profiler);
};

struct ScopedTrace
{
public:
    ScopedTrace(const Char* InName)
        : Name(InName)
    {
        Profiler::BeginTraceScope(Name);
    }

    ~ScopedTrace()
    {
        Profiler::EndTraceScope(Name);
    }

private:
    const Char* Name     = nullptr;
    Clock       Clock;
};