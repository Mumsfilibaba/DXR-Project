#pragma once
#include "Core/Time/Timer.h"

#include "CoreRHI/RHICommandList.h"

#include "Core/Containers/HashTable.h"

#define ENABLE_PROFILER      (1)
#define NUM_PROFILER_SAMPLES (200)

#if ENABLE_PROFILER
#define TRACE_SCOPE(Name)      SScopedTrace PREPROCESS_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
#define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(FUNCTION_SIGNATURE)

#define GPU_TRACE_SCOPE(CmdList, Name) SGPUScopedTrace PREPROCESS_CONCAT(GPUScopedTrace_Line_, __LINE__)(CmdList, Name)

#else
#define TRACE_SCOPE(Name)
#define TRACE_FUNCTION_SCOPE()

#define GPU_TRACE_SCOPE(CmdList, Name)

#endif

class CORE_API CProfiler
{
public:
    static void Init();
    static void Tick();

    static void Enable();
    static void Disable();
    static void Reset();

    static void BeginTraceScope( const char* Name );
    static void EndTraceScope( const char* Name );

    static void BeginGPUFrame( CRHICommandList& CmdList );
    static void BeginGPUTrace( CRHICommandList& CmdList, const char* Name );
    static void EndGPUTrace( CRHICommandList& CmdList, const char* Name );
    static void EndGPUFrame( CRHICommandList& CmdList );

    static void SetGPUProfiler( class CGPUProfiler* Profiler );
};

struct SScopedTrace
{
public:
    FORCEINLINE SScopedTrace( const char* InName )
        : Name( InName )
    {
        CProfiler::BeginTraceScope( Name );
    }

    FORCEINLINE ~SScopedTrace()
    {
        CProfiler::EndTraceScope( Name );
    }

private:
    const char* Name = nullptr;
};

struct SGPUScopedTrace
{
public:
    FORCEINLINE SGPUScopedTrace( CRHICommandList& InCmdList, const char* InName )
        : CmdList( InCmdList )
        , Name( InName )
    {
        CProfiler::BeginGPUTrace( CmdList, Name );
    }

    FORCEINLINE ~SGPUScopedTrace()
    {
        CProfiler::EndGPUTrace( CmdList, Name );
    }

private:
    CRHICommandList& CmdList;
    const char* Name = nullptr;
};