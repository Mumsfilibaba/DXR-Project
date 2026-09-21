#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/PlatformInterface/IPlatformStackTrace.h"
#include "Core/PlatformInterface/IPlatformThreadMisc.h"
#include "Core/Math/Math.h"
#include "Core/Containers/Map.h"
#include "Core/Time/Time.h"

static uint32 GetOpenDepthTLSSlot()
{
    static const uint32 Slot = FPlatformTLS::AllocTLSSlot();
    return Slot;
}

static int32 GetOpenDepth()
{
    const uint32 Slot = GetOpenDepthTLSSlot();
    if (Slot == CORE_INVALID_TLS_INDEX)
    {
        return 0;
    }

    return static_cast<int32>(reinterpret_cast<uintptr_t>(FPlatformTLS::GetTLSValue(Slot)));
}

static void SetOpenDepth(int32 Depth)
{
    const uint32 Slot = GetOpenDepthTLSSlot();
    if (Slot == CORE_INVALID_TLS_INDEX)
    {
        return;
    }

    FPlatformTLS::SetTLSValue(Slot, reinterpret_cast<void*>(static_cast<uintptr_t>(Math::Max(Depth, 0))));
}

FBootProfiler& FBootProfiler::Get()
{
    static FBootProfiler StaticBootProfiler;
    return StaticBootProfiler;
}

FBootProfiler::FBootProfiler()
    : bEnabled(false)
    , bFinished(false)
    , bCaptureNativeStacks(false)
    , StartTimeStamp(0)
    , Samples()
    , SamplesLock()
    , Session()
{
    Frequency = FPlatformTime::QueryPerformanceFrequency();
}

FBootProfiler::~FBootProfiler() = default;

void FBootProfiler::Enable()
{
    if (bFinished)
    {
        return;
    }

    bEnabled       = true;
    StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
}

void FBootProfiler::Disable()
{
    bEnabled = false;
}

void FBootProfiler::SetCaptureNativeStacks(bool bInCaptureNativeStacks)
{
    bCaptureNativeStacks = bInCaptureNativeStacks;
}

void FBootProfiler::Reset()
{
    TScopedLock Lock(SamplesLock);

    Samples.Clear();

    Session   = FProfilerFrame();
    bFinished = false;
    bEnabled  = false;
}

void FBootProfiler::AddSample(const FFrameProfilerSample& InSample)
{
    if (!bEnabled || bFinished)
    {
        return;
    }

    TScopedLock Lock(SamplesLock);
    Samples.Add(InSample);
}

void FBootProfiler::Finish()
{
    if (bFinished)
    {
        return;
    }

    TArray<FFrameProfilerSample> Closed;
    {
        TScopedLock Lock(SamplesLock);
        Closed    = Move(Samples);
        bEnabled  = false;
        bFinished = true;
    }

    Session = FProfilerFrame();
    Session.FrameIndex     = 0;
    Session.StartTimeStamp = StartTimeStamp;
    Session.EndTimeStamp   = FPlatformTime::QueryPerformanceCounter();

    TMap<void*, int32> ThreadToIndex;
    for (const FFrameProfilerSample& Sample : Closed)
    {
        int32 ThreadIndex;
        if (int32* Existing = ThreadToIndex.Find(Sample.ThreadHandle))
        {
            ThreadIndex = *Existing;
        }
        else
        {
            ThreadIndex = Session.Threads.Size();
            FProfilerThreadFrame& ThreadFrame = Session.Threads.Emplace();
            ThreadFrame.ThreadHandle = Sample.ThreadHandle;
            ThreadToIndex.Add(Sample.ThreadHandle, ThreadIndex);
        }

        FProfilerInterval Interval;
        Interval.Name           = Sample.Name;
        Interval.ThreadHandle   = Sample.ThreadHandle;
        Interval.StartTimeStamp = Sample.StartTimeStamp;
        Interval.EndTimeStamp   = Sample.EndTimeStamp;
        Interval.Depth          = Sample.Depth;
        Interval.StackFrames    = Sample.StackFrames;
        Interval.StackDepth     = Sample.StackDepth;
        Interval.bInstant       = Sample.bInstant || (Sample.EndTimeStamp == Sample.StartTimeStamp);
        Session.Threads[ThreadIndex].Intervals.Add(Interval);
    }

    uint64 Inclusive = 0;
    for (FProfilerThreadFrame& ThreadFrame : Session.Threads)
    {
        FinalizeProfilerIntervals(ThreadFrame.Intervals, Frequency);
        for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
        {
            if (Interval.Depth == 0)
            {
                Inclusive += Interval.InclusiveNanoseconds;
            }
        }
    }

    SortProfilerThreadFrames(Session.Threads);

    Session.CpuMilliseconds = Time::ToMilliseconds(static_cast<float>(Inclusive));
}

FBootProfilerScopedTrace::FBootProfilerScopedTrace(const CHAR* InName)
    : Sample(InName)
{
    Sample.ThreadHandle   = FPlatformThreadMisc::GetCurrentThreadHandle();
    Sample.StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    Sample.Depth          = GetOpenDepth();
    SetOpenDepth(Sample.Depth + 1);
}

FBootProfilerScopedTrace::~FBootProfilerScopedTrace()
{
    SetOpenDepth(GetOpenDepth() - 1);

    Sample.EndTimeStamp = FPlatformTime::QueryPerformanceCounter();

    FBootProfiler& Profiler = FBootProfiler::Get();
    if (Profiler.CapturesNativeStacks())
    {
        Sample.StackDepth = IPlatformStackTrace::CaptureStackTrace(Sample.StackFrames.Data(), NUM_PROFILER_STACK_FRAMES, 2);
    }

    Profiler.AddSample(Sample);
}

void FBootProfiler::AddInstantEvent(const CHAR* Name)
{
    if (!bEnabled || bFinished)
    {
        return;
    }

    FFrameProfilerSample Sample(Name);
    Sample.ThreadHandle   = FPlatformThreadMisc::GetCurrentThreadHandle();
    Sample.StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    Sample.EndTimeStamp   = Sample.StartTimeStamp;
    Sample.Depth          = GetOpenDepth();
    Sample.bInstant       = true;

    if (bCaptureNativeStacks)
    {
        Sample.StackDepth = IPlatformStackTrace::CaptureStackTrace(Sample.StackFrames.Data(), NUM_PROFILER_STACK_FRAMES, 1);
    }

    AddSample(Sample);
}
