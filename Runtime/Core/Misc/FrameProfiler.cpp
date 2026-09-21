#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/PlatformInterface/IPlatformStackTrace.h"
#include "Core/PlatformInterface/IPlatformThreadMisc.h"
#include "Core/Containers/Map.h"
#include "Core/Math/Math.h"
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

FFrameProfiler& FFrameProfiler::Get()
{
    static FFrameProfiler StaticFrameProfiler;
    return StaticFrameProfiler;
}

void FFrameProfiler::Enable()
{
    TScopedLock Lock(CurrentSamplesLock);
    bEnabled = true;
}

void FFrameProfiler::Disable()
{
    TScopedLock Lock(CurrentSamplesLock);
    bEnabled = false;
}

void FFrameProfiler::SetRetainAllFrames(bool bInRetainAllFrames)
{
    StoredFrames.SetRetainAll(bInRetainAllFrames);
}

void FFrameProfiler::SetCaptureNativeStacks(bool bInCaptureNativeStacks)
{
    TScopedLock Lock(CurrentSamplesLock);
    bCaptureNativeStacks = bInCaptureNativeStacks;
}

bool FFrameProfiler::IsEnabled() const
{
    TScopedLock Lock(CurrentSamplesLock);
    return bEnabled;
}

bool FFrameProfiler::CapturesNativeStacks() const
{
    TScopedLock Lock(CurrentSamplesLock);
    return bCaptureNativeStacks;
}

FFrameProfiler::FFrameProfiler()
    : Clock()
    , CurrentFps(0)
    , Fps(0)
    , NextFrameIndex(0)
    , bEnabled(false)
    , bCaptureNativeStacks(false)
    , CurrentSamples()
    , CurrentSamplesLock()
    , StoredFrames()
{
    Frequency = FPlatformTime::QueryPerformanceFrequency();
}

FFrameProfiler::~FFrameProfiler() = default;

void FFrameProfiler::Tick()
{
    Clock.Tick();

    CurrentFps++;
    if (Clock.GetTotalTime().AsSeconds() > 1.0f)
    {
        Fps = CurrentFps;
        CurrentFps = 0;

        Clock.Reset();
    }

    const double DeltaMilliseconds = Clock.GetDeltaTime().AsMilliseconds();

    TArray<FFrameProfilerSample> Samples;
    {
        TScopedLock Lock(CurrentSamplesLock);
        if (!bEnabled)
        {
            CurrentSamples.Clear();
            return;
        }

        Samples = Move(CurrentSamples);
    }

    FProfilerFrame Frame;
    Frame.CpuMilliseconds = static_cast<float>(DeltaMilliseconds);
    Frame.StartTimeStamp  = 0;
    Frame.EndTimeStamp    = 0;

    TMap<void*, int32> ThreadToFrameIndex;

    for (const FFrameProfilerSample& Sample : Samples)
    {
        int32 ThreadFrameIndex;
        if (int32* Existing = ThreadToFrameIndex.Find(Sample.ThreadHandle))
        {
            ThreadFrameIndex = *Existing;
        }
        else
        {
            ThreadFrameIndex = Frame.Threads.Size();
            FProfilerThreadFrame& ThreadFrame = Frame.Threads.Emplace();
            ThreadFrame.ThreadHandle = Sample.ThreadHandle;
            ThreadToFrameIndex.Add(Sample.ThreadHandle, ThreadFrameIndex);
        }

        FProfilerInterval Interval;
        Interval.Name            = Sample.Name;
        Interval.ThreadHandle    = Sample.ThreadHandle;
        Interval.StartTimeStamp  = Sample.StartTimeStamp;
        Interval.EndTimeStamp    = Sample.EndTimeStamp;
        Interval.Depth           = Sample.Depth;
        Interval.StackFrames     = Sample.StackFrames;
        Interval.StackDepth      = Sample.StackDepth;
        Interval.bInstant        = Sample.bInstant || (Sample.EndTimeStamp == Sample.StartTimeStamp);
        Frame.Threads[ThreadFrameIndex].Intervals.Add(Interval);

        if (Frame.StartTimeStamp == 0 || Sample.StartTimeStamp < Frame.StartTimeStamp)
        {
            Frame.StartTimeStamp = Sample.StartTimeStamp;
        }

        if (Sample.EndTimeStamp > Frame.EndTimeStamp)
        {
            Frame.EndTimeStamp = Sample.EndTimeStamp;
        }
    }

    for (FProfilerThreadFrame& ThreadFrame : Frame.Threads)
    {
        FinalizeProfilerIntervals(ThreadFrame.Intervals, Frequency);
    }

    SortProfilerThreadFrames(Frame.Threads);

    {
        TScopedLock Lock(CurrentSamplesLock);
        Frame.FrameIndex = NextFrameIndex;
        StoredFrames.Push(Move(Frame));
        ++NextFrameIndex;
    }
}

int32 FFrameProfiler::GetLatestFinishedFrameIndex() const
{
    TScopedLock Lock(CurrentSamplesLock);
    return NextFrameIndex > 0 ? (NextFrameIndex - 1) : -1;
}

float FFrameProfiler::GetLatestCpuMilliseconds() const
{
    const FProfilerFrame* Frame = GetLatestFrame();
    return Frame ? Frame->CpuMilliseconds : 0.0f;
}

int32 FFrameProfiler::GetStoredFrameCount() const
{
    return StoredFrames.Num();
}

const FProfilerFrame* FFrameProfiler::GetStoredFrame(int32 OldestIndex) const
{
    return StoredFrames.GetOldest(OldestIndex);
}

const FProfilerFrame* FFrameProfiler::FindFrame(int32 FrameIndex) const
{
    return StoredFrames.FindIf([FrameIndex](const FProfilerFrame& Frame)
    {
        return Frame.FrameIndex == FrameIndex;
    });
}

const FProfilerFrame* FFrameProfiler::GetLatestFrame() const
{
    return StoredFrames.GetLatest();
}

void FFrameProfiler::Reset()
{
    TScopedLock Lock(CurrentSamplesLock);
    NextFrameIndex = 0;
    StoredFrames.Clear();
    CurrentSamples.Clear();
}

void FFrameProfiler::AddSample(const FFrameProfilerSample& InSample)
{
    TScopedLock Lock(CurrentSamplesLock);
    if (!bEnabled)
    {
        return;
    }

    if (InSample.ThreadHandle)
    {
        CurrentSamples.Add(InSample);
    }
    else
    {
        LOG_WARNING("Sample does not have a valid ThreadID");
        DEBUG_BREAK();
    }
}

void FFrameProfiler::AddInstantEvent(const CHAR* Name)
{
    if (!IsEnabled())
    {
        return;
    }

    FFrameProfilerSample Sample(Name);
    Sample.ThreadHandle   = FPlatformThreadMisc::GetCurrentThreadHandle();
    Sample.StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    Sample.EndTimeStamp   = Sample.StartTimeStamp;
    Sample.Depth          = GetOpenDepth();
    Sample.bInstant       = true;

    if (CapturesNativeStacks())
    {
        Sample.StackDepth = IPlatformStackTrace::CaptureStackTrace(Sample.StackFrames.Data(), NUM_PROFILER_STACK_FRAMES, 1);
    }

    AddSample(Sample);
}

FFrameProfilerScopedTrace::FFrameProfilerScopedTrace(const CHAR* InName)
    : Sample(InName)
{
    Sample.ThreadHandle   = FPlatformThreadMisc::GetCurrentThreadHandle();
    Sample.StartTimeStamp = FPlatformTime::QueryPerformanceCounter();
    Sample.Depth          = GetOpenDepth();
    SetOpenDepth(Sample.Depth + 1);
}

FFrameProfilerScopedTrace::~FFrameProfilerScopedTrace()
{
    SetOpenDepth(GetOpenDepth() - 1);

    Sample.EndTimeStamp = FPlatformTime::QueryPerformanceCounter();

    FFrameProfiler& Profiler = FFrameProfiler::Get();
    if (Profiler.CapturesNativeStacks())
    {
        Sample.StackDepth = IPlatformStackTrace::CaptureStackTrace(Sample.StackFrames.Data(), NUM_PROFILER_STACK_FRAMES, 2);
    }

    Profiler.AddSample(Sample);
}
