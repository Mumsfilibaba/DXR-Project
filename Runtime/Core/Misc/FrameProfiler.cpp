#include "FrameProfiler.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"

FFrameProfiler& FFrameProfiler::Get()
{
    static FFrameProfiler Instance;
    return Instance;
}

void FFrameProfiler::Enable()
{
    bEnabled = true;
}

void FFrameProfiler::Disable()
{
    bEnabled = false;
}

FFrameProfiler::FFrameProfiler()
    : CPUFrameTime()
    , Clock()
    , CurrentFps(0)
    , Fps(0)
    , bEnabled(true)
    , CurrentSamples()
    , CurrentSamplesLock()
    , FunctionInfoTable()
{
    Frequency = FPlatformTime::QueryPerformanceFrequency();
}

FFrameProfiler::~FFrameProfiler()
{
}

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

    if (!bEnabled)
    {
        return;
    }

    // FrameTime
    {
        const double Delta = Clock.GetDeltaTime().AsMilliseconds();
        CPUFrameTime.AddSample(static_cast<float>(Delta));
    }

    // Handle function samples
    TArray<FFrameProfilerSample> Samples;
    {
        TScopedLock Lock(CurrentSamplesLock);
        Samples = Move(CurrentSamples);
    }

    for (const FFrameProfilerSample& Sample : Samples)
    {
        int32 TableIndex;
        if (int32* Index = ThreadHandleToIndexMap.Find(Sample.ThreadHandle))
        {
            TableIndex = *Index;
        }
        else
        {
            TableIndex = FunctionInfoTable.Size();
            FunctionInfoTable.Emplace(Sample.ThreadHandle);
            ThreadHandleToIndexMap.Add(Sample.ThreadHandle, TableIndex);
        }

        const FString ScopeName   = Sample.Name;
        const uint64  Delta       = Sample.EndTimeStamp - Sample.StartTimeStamp;
        const uint64  Nanoseconds = TimeUtilities::FromSeconds(Delta) / Frequency;

        FFrameProfilerThreadInfo& ThreadInfo = FunctionInfoTable[TableIndex];
        if (FFrameProfilerFunctionInfo* Entry = ThreadInfo.FunctionInfoMap.Find(ScopeName))
        {
            Entry->AddSample(static_cast<float>(Nanoseconds));
        }
        else
        {
            FFrameProfilerFunctionInfo& NewSample = ThreadInfo.FunctionInfoMap.Add(ScopeName);
            NewSample.AddSample(static_cast<float>(Nanoseconds));
        }
    }
}

void FFrameProfiler::Reset()
{
    CPUFrameTime.Reset();

    for (FFrameProfilerThreadInfo& ThreadInfo : FunctionInfoTable)
    {
        for (auto FunctionInfo : ThreadInfo.FunctionInfoMap)
        {
            FunctionInfo.Second.Reset();
        }
    }
}

void FFrameProfiler::AddSample(const FFrameProfilerSample& InSample)
{
    if (bEnabled)
    {
        TScopedLock Lock(CurrentSamplesLock);

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
}

void FFrameProfiler::GetFunctionInfo(TArray<FFrameProfilerThreadInfo>& OutFunctionThreadInfo)
{
    OutFunctionThreadInfo = FunctionInfoTable;
}
