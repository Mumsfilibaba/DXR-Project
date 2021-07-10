#pragma once
#include "Time/Timer.h"

#include "RenderLayer/CommandList.h"

#include <unordered_map>

#define ENABLE_PROFILER      1
#define NUM_PROFILER_SAMPLES 200

#if ENABLE_PROFILER
#define TRACE_SCOPE(Name)      ScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
#define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(__FUNCTION_SIG__)

#define GPU_TRACE_SCOPE(CmdList, Name) GPUScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(CmdList, Name)
#else
#define TRACE_SCOPE(Name)
#define TRACE_FUNCTION_SCOPE()

#define GPU_TRACE_SCOPE(CmdList, Name)
#endif

class Profiler
{
public:
    static void Init();
    static void Tick();

    static void Enable();
    static void Disable();
    static void Reset();

    static void BeginTraceScope( const char* Name );
    static void EndTraceScope( const char* Name );

    static void BeginGPUFrame( CommandList& CmdList );
    static void BeginGPUTrace( CommandList& CmdList, const char* Name );
    static void EndGPUTrace( CommandList& CmdList, const char* Name );
    static void EndGPUFrame( CommandList& CmdList );

    static void SetGPUProfiler( class GPUProfiler* Profiler );
};

struct ScopedTrace
{
public:
    ScopedTrace( const char* InName )
        : Name( InName )
    {
        Profiler::BeginTraceScope( Name );
    }

    ~ScopedTrace()
    {
        Profiler::EndTraceScope( Name );
    }

private:
    const char* Name = nullptr;
};

struct GPUScopedTrace
{
public:
    GPUScopedTrace( CommandList& InCmdList, const char* InName )
        : CmdList( InCmdList )
        , Name( InName )
    {
        Profiler::BeginGPUTrace( CmdList, Name );
    }

    ~GPUScopedTrace()
    {
        Profiler::EndGPUTrace( CmdList, Name );
    }

private:
    CommandList& CmdList;
    const char* Name = nullptr;
};