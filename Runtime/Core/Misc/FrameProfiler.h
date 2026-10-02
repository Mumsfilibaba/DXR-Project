#pragma once
#include "Core/Misc/ProfilerTypes.h"
#include "Core/Time/ElapsedTime.h"
#include "Core/Threading/Spinlock.h"

#define ENABLE_PROFILER 1

#if ENABLE_PROFILER
    #define TRACE_SCOPE(Name) FFrameProfilerScopedTrace STRING_CONCAT(ScopedTrace_Line_, __LINE__)(Name)
    #define TRACE_FUNCTION_SCOPE() TRACE_SCOPE(FUNCTION_SIGNATURE)
    #define TRACE_EVENT(Name) FFrameProfiler::Get().AddInstantEvent(Name)
#else
    #define TRACE_SCOPE(Name)
    #define TRACE_FUNCTION_SCOPE()
    #define TRACE_EVENT(Name)
#endif

struct FFrameProfilerSample
{
    FFrameProfilerSample()
        : Name(nullptr)
        , ThreadHandle(nullptr)
        , StartTimeStamp(0)
        , EndTimeStamp(0)
        , Depth(0)
        , StackDepth(0)
        , bInstant(false)
    {
        StackFrames.Fill(0);
    }

    FFrameProfilerSample(const CHAR* InName)
        : Name(InName)
        , ThreadHandle(nullptr)
        , StartTimeStamp(0)
        , EndTimeStamp(0)
        , Depth(0)
        , StackDepth(0)
        , bInstant(false)
    {
        StackFrames.Fill(0);
    }

    const CHAR* Name;
    void*       ThreadHandle;
    uint64      StartTimeStamp;
    uint64      EndTimeStamp;
    int32       Depth;
    TStaticArray<uint64, NUM_PROFILER_STACK_FRAMES> StackFrames;
    int32       StackDepth;
    bool        bInstant;
};

class CORE_API FFrameProfiler
{
public:
    static FFrameProfiler& Get();

public:
    void Enable();
    void Disable();
    void Tick();
    void Reset();
    void AddSample(const FFrameProfilerSample& InSample);
    void AddInstantEvent(const CHAR* Name);
    void SetRetainAllFrames(bool bInRetainAllFrames);
    void SetCaptureNativeStacks(bool bInCaptureNativeStacks);

    NODISCARD bool IsEnabled() const;
    NODISCARD bool CapturesNativeStacks() const;
    NODISCARD bool RetainsAllFrames() const { return StoredFrames.RetainsAll(); }
    
    NODISCARD int32 GetLatestFinishedFrameIndex() const;
    NODISCARD float GetLatestCpuMilliseconds() const;
    NODISCARD int32 GetStoredFrameCount() const;
    
    NODISCARD const FProfilerFrame* GetStoredFrame(int32 OldestIndex) const;
    NODISCARD const FProfilerFrame* FindFrame(int32 FrameIndex) const;
    NODISCARD const FProfilerFrame* GetLatestFrame() const;

    int32 GetFramesPerSecond() const
    {
        return Fps;
    }

    NODISCARD uint64 GetFrequency() const
    {
        return Frequency;
    }

private:
    FFrameProfiler();
    ~FFrameProfiler();

    FElapsedTime                     Clock;
    int32                            CurrentFps;
    int32                            Fps;
    int32                            NextFrameIndex;
    bool                             bEnabled;
    bool                             bCaptureNativeStacks;
    uint64                           Frequency;
    TArray<FFrameProfilerSample>     CurrentSamples;
    mutable FSpinLock                CurrentSamplesLock;
    TProfilerFrameRing<FProfilerFrame> StoredFrames;
};

struct CORE_API FFrameProfilerScopedTrace
{
public:
    FFrameProfilerScopedTrace(const CHAR* InName);
    ~FFrameProfilerScopedTrace();

private:
    FFrameProfilerSample Sample;
};
