#pragma once
#include "Core/Misc/FrameProfiler.h"

#define ENABLE_BOOT_PROFILER 1

#if ENABLE_BOOT_PROFILER
    #define TRACE_BOOT_SCOPE(Name) FBootProfilerScopedTrace STRING_CONCAT(BootScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_BOOT_FUNCTION_SCOPE() TRACE_BOOT_SCOPE(FUNCTION_SIGNATURE)
    #define TRACE_BOOT_EVENT(Name) FBootProfiler::Get().AddInstantEvent(Name)
#else
    #define TRACE_BOOT_SCOPE(Name)
    #define TRACE_BOOT_FUNCTION_SCOPE()
    #define TRACE_BOOT_EVENT(Name)
#endif

class CORE_API FBootProfiler
{
public:
    static FBootProfiler& Get();

public:
    void Enable();
    void Disable();
    void Finish();
    void Reset();
    void AddSample(const FFrameProfilerSample& InSample);
    void AddInstantEvent(const CHAR* Name);
    void SetCaptureNativeStacks(bool bInCaptureNativeStacks);

    NODISCARD bool IsEnabled() const
    {
        return bEnabled;
    }

    NODISCARD bool IsFinished() const
    {
        return bFinished;
    }

    NODISCARD bool CapturesNativeStacks() const
    {
        return bCaptureNativeStacks;
    }

    NODISCARD const FProfilerFrame& GetSession() const
    {
        return Session;
    }

private:
    FBootProfiler();
    ~FBootProfiler();

    bool                         bEnabled;
    bool                         bFinished;
    bool                         bCaptureNativeStacks;
    uint64                       Frequency;
    uint64                       StartTimeStamp;
    TArray<FFrameProfilerSample> Samples;
    FSpinLock                    SamplesLock;
    FProfilerFrame               Session;
};

struct CORE_API FBootProfilerScopedTrace
{
public:
    FBootProfilerScopedTrace(const CHAR* InName);
    ~FBootProfilerScopedTrace();

private:
    FFrameProfilerSample Sample;
};
