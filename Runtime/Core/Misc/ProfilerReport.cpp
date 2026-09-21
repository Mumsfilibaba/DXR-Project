#include "Core/Misc/ProfilerReport.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Time/Time.h"
#include "Core/Math/Math.h"
#include "Core/Containers/Map.h"

static String FormatMilliseconds(uint64 Nanoseconds)
{
    return String::Printf("%.3f", Time::ToMilliseconds(static_cast<double>(Nanoseconds)));
}

static String ResolveThreadName(void* ThreadHandle, int32 FallbackIndex)
{
    if (FThreadManager::Get().IsMainThread(ThreadHandle))
    {
        return String("GameThread");
    }

    if (IPlatformThread* Thread = FThreadManager::Get().GetThreadFromHandle(ThreadHandle))
    {
        return Thread->GetName();
    }

    return String::Printf("Thread %d", FallbackIndex);
}

static void AppendIndent(String& Out, int32 Depth)
{
    for (int32 Index = 0; Index < Depth; ++Index)
    {
        Out.Append("  ");
    }
}

static void AppendIntervalTree(String& Out, const TArray<FProfilerInterval>& Intervals, int32 ParentIndex, int32 Depth)
{
    for (int32 Index = 0; Index < Intervals.Size(); ++Index)
    {
        const FProfilerInterval& Interval = Intervals[Index];
        if (Interval.ParentIndex != ParentIndex)
        {
            continue;
        }

        AppendIndent(Out, Depth);

        const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";
        if (Interval.bInstant)
        {
            Out.Append(String::Printf("%s  event\n", Name));
        }
        else
        {
            Out.Append(String::Printf("%s  inclusive=%sms  exclusive=%sms\n", Name,
                *FormatMilliseconds(Interval.InclusiveNanoseconds),
                *FormatMilliseconds(Interval.ExclusiveNanoseconds)));
        }

        if (Interval.StackDepth > 0)
        {
            FPlatformStackTrace::InitializeSymbols();
            
            AppendIndent(Out, Depth + 1);
            Out.Append("native stack:\n");
            
            for (int32 Frame = 0; Frame < Interval.StackDepth && Frame < NUM_PROFILER_STACK_FRAMES; ++Frame)
            {
                FStackTraceEntry Entry;
                FPlatformStackTrace::GetStackTraceEntryFromAddress(Interval.StackFrames[Frame], Entry);

                AppendIndent(Out, Depth + 2);
                
                Out.Append(String::Printf("%s  %s:%u\n", Entry.FunctionName, Entry.Filename, Entry.Line));
            }

            FPlatformStackTrace::ReleaseSymbols();
        }

        AppendIntervalTree(Out, Intervals, Index, Depth + 1);
    }
}

struct FHotScope
{
    String Name;
    uint64 Inclusive = 0;
    uint64 Exclusive = 0;
    int32  Calls     = 0;
};

static void CollectHotScopes(TMap<String, FHotScope>& OutHotScopes, double& OutCapturedFrameMilliseconds)
{
    OutHotScopes.Clear();
    OutCapturedFrameMilliseconds = 0.0;

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();

    const int32 StoredFrames = FrameProfiler.GetStoredFrameCount();
    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (!Frame)
        {
            continue;
        }

        OutCapturedFrameMilliseconds += static_cast<double>(Frame->CpuMilliseconds);

        for (const FProfilerThreadFrame& ThreadFrame : Frame->Threads)
        {
            for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
            {
                const String Name = Interval.Name ? Interval.Name : "<unnamed>";

                FHotScope* Entry = OutHotScopes.Find(Name);
                if (!Entry)
                {
                    FHotScope& Added = OutHotScopes.Add(Name);
                    Added.Name = Name;
                    Entry = &Added;
                }

                Entry->Inclusive += Interval.InclusiveNanoseconds;
                Entry->Exclusive += Interval.ExclusiveNanoseconds;
                Entry->Calls++;
            }
        }
    }
}

static void AppendChromeComma(String& Out)
{
    if (!Out.IsEmpty() && Out[Out.Length() - 1] != '[' && Out[Out.Length() - 1] != ',')
    {
        Out.Append(",");
    }
}

static void AppendChromeMetadata(String& Out, const CHAR* Name, int32 Pid, int64 Tid)
{
    AppendChromeComma(Out);
    Out.Append(String::Printf("{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":%d,\"tid\":%lld,\"args\":{\"name\":\"%s\"}}",
        Pid,
        static_cast<long long>(Tid),
        *FProfilerReport::EscapeChromeString(Name)));
}

static void AppendFrameChromeEvents(String& Out, const FProfilerFrame& Frame, uint64 BaseStamp, uint64 Frequency, int32 Pid)
{
    for (int32 ThreadIndex = 0; ThreadIndex < Frame.Threads.Size(); ++ThreadIndex)
    {
        const FProfilerThreadFrame& ThreadFrame = Frame.Threads[ThreadIndex];

        const int64 Tid = static_cast<int64>(reinterpret_cast<UPTR_INT>(ThreadFrame.ThreadHandle));
        for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
        {
            const uint64 StartDelta  = Interval.StartTimeStamp >= BaseStamp ? (Interval.StartTimeStamp - BaseStamp) : 0;
            const double TimestampUs = Time::ToMicroseconds(static_cast<double>(ProfilerTicksToNanoseconds(StartDelta, Frequency)));

            const CHAR* Name = Interval.Name ? Interval.Name : "<unnamed>";
            if (Interval.bInstant || Interval.InclusiveNanoseconds == 0)
            {
                FProfilerReport::AppendChromeInstantEvent(Out, Name, "cpu", TimestampUs, Pid, Tid);
            }
            else
            {
                const double DurationUs = Time::ToMicroseconds(static_cast<double>(Interval.InclusiveNanoseconds));
                FProfilerReport::AppendChromeCompleteEvent(Out, Name, "cpu", TimestampUs, DurationUs, Pid, Tid);
            }
        }
    }
}

static uint64 ComputeChromeBaseTimeStamp()
{
    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();
    uint64 BaseStamp = 0;

    const FProfilerFrame& Boot = FBootProfiler::Get().GetSession();
    if (Boot.StartTimeStamp != 0)
    {
        BaseStamp = Boot.StartTimeStamp;
    }

    const int32 StoredFrames = FrameProfiler.GetStoredFrameCount();
    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (Frame && Frame->StartTimeStamp != 0 && (BaseStamp == 0 || Frame->StartTimeStamp < BaseStamp))
        {
            BaseStamp = Frame->StartTimeStamp;
        }
    }

    return BaseStamp;
}

TArray<FProfilerOptimizationTarget> FProfilerReport::CollectOptimizationTargets(int32 MaxTargets)
{
    TMap<String, FHotScope> HotScopes;

    double CapturedFrameMilliseconds = 0.0;
    CollectHotScopes(HotScopes, CapturedFrameMilliseconds);

    TArray<FHotScope> SortedSelf;
    for (auto Pair : HotScopes)
    {
        SortedSelf.Add(Pair.Second);
    }

    SortedSelf.SortWithPredicate([](const FHotScope& Left, const FHotScope& Right)
    {
        return Left.Exclusive > Right.Exclusive;
    });

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();

    const int32  StoredFrames           = Math::Max(FrameProfiler.GetStoredFrameCount(), 1);
    const double FrameDivisor           = static_cast<double>(StoredFrames);
    const double TotalBudgetNanoseconds = CapturedFrameMilliseconds * 1000000.0;

    TArray<FProfilerOptimizationTarget> Targets;
    const int32 Limit = Math::Min(SortedSelf.Size(), Math::Max(MaxTargets, 0));
    Targets.Reserve(Limit);

    for (int32 Index = 0; Index < Limit; ++Index)
    {
        const FHotScope& Scope = SortedSelf[Index];

        FProfilerOptimizationTarget Target;
        Target.Name                          = Scope.Name;
        Target.SelfMillisecondsPerFrame      = Time::ToMilliseconds(static_cast<double>(Scope.Exclusive)) / FrameDivisor;
        Target.InclusiveMillisecondsPerFrame = Time::ToMilliseconds(static_cast<double>(Scope.Inclusive)) / FrameDivisor;
        Target.CallsPerFrame                 = static_cast<double>(Scope.Calls) / FrameDivisor;
        
        Target.BudgetPercent = TotalBudgetNanoseconds > 0.0
            ? (static_cast<double>(Scope.Exclusive) * 100.0) / TotalBudgetNanoseconds
            : 0.0;

        Targets.Add(Target);
    }

    return Targets;
}

TArray<FProfilerWorstFrame> FProfilerReport::CollectWorstFrames(int32 MaxFrames)
{
    TArray<FProfilerWorstFrame> Worst;
    const int32 Limit = Math::Max(MaxFrames, 0);
    if (Limit <= 0)
    {
        return Worst;
    }

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();

    const int32 StoredFrames = FrameProfiler.GetStoredFrameCount();
    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (!Frame)
        {
            continue;
        }

        int32 InsertAt = Worst.Size();
        for (int32 Rank = 0; Rank < Worst.Size(); ++Rank)
        {
            if (Frame->CpuMilliseconds > Worst[Rank].CpuMilliseconds)
            {
                InsertAt = Rank;
                break;
            }
        }

        if (InsertAt >= Limit)
        {
            continue;
        }

        FProfilerWorstFrame Entry;
        Entry.FrameIndex      = Frame->FrameIndex;
        Entry.CpuMilliseconds = Frame->CpuMilliseconds;
        Worst.Insert(InsertAt, Entry);

        if (Worst.Size() > Limit)
        {
            Worst.RemoveAt(Worst.Size() - 1);
        }
    }

    return Worst;
}

void FProfilerReport::CollectThreadAggregates(TArray<FProfilerThreadAggregate>& OutThreads)
{
    OutThreads.Clear();

    TMap<void*, int32> HandleToIndex;

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();

    const int32 StoredFrames = FrameProfiler.GetStoredFrameCount();
    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (!Frame)
        {
            continue;
        }

        for (const FProfilerThreadFrame& ThreadFrame : Frame->Threads)
        {
            int32 ThreadIndex;
            if (int32* Existing = HandleToIndex.Find(ThreadFrame.ThreadHandle))
            {
                ThreadIndex = *Existing;
            }
            else
            {
                ThreadIndex = OutThreads.Size();

                FProfilerThreadAggregate& Added = OutThreads.Emplace();
                Added.ThreadHandle = ThreadFrame.ThreadHandle;
                HandleToIndex.Add(ThreadFrame.ThreadHandle, ThreadIndex);
            }

            FProfilerThreadAggregate& ThreadAggregate = OutThreads[ThreadIndex];
            for (const FProfilerInterval& Interval : ThreadFrame.Intervals)
            {
                const String Name = Interval.Name ? Interval.Name : "<unnamed>";

                FProfilerScopeAggregate* Entry = nullptr;
                for (FProfilerScopeAggregate& Scope : ThreadAggregate.Scopes)
                {
                    if (Scope.Name == Name)
                    {
                        Entry = &Scope;
                        break;
                    }
                }

                if (!Entry)
                {
                    FProfilerScopeAggregate& Added = ThreadAggregate.Scopes.Emplace();
                    Added.Name = Name;
                    Entry = &Added;
                }

                Entry->Calls++;
                Entry->InclusiveNanoseconds += Interval.InclusiveNanoseconds;
                Entry->ExclusiveNanoseconds += Interval.ExclusiveNanoseconds;
            }
        }
    }
}

String FProfilerReport::BuildCpuAndBootText()
{
    String Text;
    Text.Append("DXR Profile Run\n");
    Text.Append(String::Printf("Engine=%s %s\n", BuildInfo::GetEngineName(), BuildInfo::GetVersionString()));
    Text.Append(String::Printf("Configuration=%s (%s)\n", BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName()));
    Text.Append(String::Printf("Platform=%s %s\n", BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName()));
    Text.Append(String::Printf("CommandLine=%s\n", CommandLine::GetOriginal()));
    Text.Append(String::Printf("NativeStacks=%s\n", CommandLine::FindOption("ProfileNativeStacks") ? "enabled" : "disabled"));

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();
    const int32 StoredFrames = FrameProfiler.GetStoredFrameCount();

    float  CpuMin = TNumericLimits<float>::Max();
    float  CpuMax = TNumericLimits<float>::Lowest();
    double CpuSum = 0.0;

    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (!Frame)
        {
            continue;
        }

        CpuMin = Math::Min(CpuMin, Frame->CpuMilliseconds);
        CpuMax = Math::Max(CpuMax, Frame->CpuMilliseconds);
        CpuSum += static_cast<double>(Frame->CpuMilliseconds);
    }

    const float CpuAvg = StoredFrames > 0 ? static_cast<float>(CpuSum / static_cast<double>(StoredFrames)) : 0.0f;
    if (CpuMin == TNumericLimits<float>::Max())
    {
        CpuMin = 0.0f;
    }
    if (CpuMax == TNumericLimits<float>::Lowest())
    {
        CpuMax = 0.0f;
    }

    Text.Append(String::Printf("Frames=%d  FPS=%d  CPU avg=%.3fms min=%.3fms max=%.3fms\n\n",
        StoredFrames,
        FrameProfiler.GetFramesPerSecond(),
        CpuAvg,
        CpuMin,
        CpuMax));

    Text.Append("== BOOT ==\n");

    const FProfilerFrame& Boot = FBootProfiler::Get().GetSession();

    Text.Append(String::Printf("Boot total=%.3fms\n", Boot.CpuMilliseconds));

    for (int32 ThreadIndex = 0; ThreadIndex < Boot.Threads.Size(); ++ThreadIndex)
    {
        const FProfilerThreadFrame& ThreadFrame = Boot.Threads[ThreadIndex];
        Text.Append(String::Printf("Thread %s\n", *ResolveThreadName(ThreadFrame.ThreadHandle, ThreadIndex)));
        AppendIntervalTree(Text, ThreadFrame.Intervals, -1, 1);
    }

    Text.Append("\n== CPU HOT SPOTS ==\n");

    TMap<String, FHotScope> HotScopes;

    double CapturedFrameMilliseconds = 0.0;
    CollectHotScopes(HotScopes, CapturedFrameMilliseconds);

    TArray<FHotScope> SortedHot;
    for (auto Pair : HotScopes)
    {
        SortedHot.Add(Pair.Second);
    }

    SortedHot.SortWithPredicate([](const FHotScope& Left, const FHotScope& Right)
    {
        return Left.Inclusive > Right.Inclusive;
    });

    const int32 HotLimit = Math::Min(SortedHot.Size(), 32);
    for (int32 Index = 0; Index < HotLimit; ++Index)
    {
        const FHotScope& Scope = SortedHot[Index];
        Text.Append(String::Printf("%s  calls=%d  inclusive=%sms  exclusive=%sms\n", *Scope.Name, 
            Scope.Calls, *FormatMilliseconds(Scope.Inclusive), *FormatMilliseconds(Scope.Exclusive)));
    }

    Text.Append("\n== OPTIMIZATION TARGETS (SELF TIME) ==\n");
    Text.Append("Self time excludes traced children. budget% compares aggregate self time with wall-frame time; concurrent threads overlap, so totals may exceed 100%.\n");

    const TArray<FProfilerOptimizationTarget> Targets = CollectOptimizationTargets();
    for (int32 Index = 0; Index < Targets.Size(); ++Index)
    {
        const FProfilerOptimizationTarget& Target = Targets[Index];
        Text.Append(String::Printf("%2d. %s  self=%.3fms/frame  inclusive=%.3fms/frame  budget=%.1f%%  calls=%.2f/frame\n",
            Index + 1, *Target.Name, Target.SelfMillisecondsPerFrame, Target.InclusiveMillisecondsPerFrame,
            Target.BudgetPercent, Target.CallsPerFrame));
    }

    Text.Append("\n== WORST FRAMES ==\n");
    const TArray<FProfilerWorstFrame> Worst = CollectWorstFrames();
    for (const FProfilerWorstFrame& Hitch : Worst)
    {
        const FProfilerFrame* Frame = FrameProfiler.FindFrame(Hitch.FrameIndex);
        if (!Frame)
        {
            continue;
        }

        Text.Append(String::Printf("Frame %d  %.3fms\n", Frame->FrameIndex, Frame->CpuMilliseconds));
        for (int32 ThreadIndex = 0; ThreadIndex < Frame->Threads.Size(); ++ThreadIndex)
        {
            const FProfilerThreadFrame& ThreadFrame = Frame->Threads[ThreadIndex];
            Text.Append(String::Printf("  %s\n", *ResolveThreadName(ThreadFrame.ThreadHandle, ThreadIndex)));
            AppendIntervalTree(Text, ThreadFrame.Intervals, -1, 2);
        }
    }

    Text.Append("\n== PER THREAD ==\n");
    TArray<FProfilerThreadAggregate> ThreadInfos;
    CollectThreadAggregates(ThreadInfos);
    for (int32 ThreadIndex = 0; ThreadIndex < ThreadInfos.Size(); ++ThreadIndex)
    {
        const FProfilerThreadAggregate& ThreadInfo = ThreadInfos[ThreadIndex];
        Text.Append(String::Printf("%s\n", *ResolveThreadName(ThreadInfo.ThreadHandle, ThreadIndex)));

        for (const FProfilerScopeAggregate& Scope : ThreadInfo.Scopes)
        {
            const double AverageNs = Scope.Calls > 0
                ? (static_cast<double>(Scope.InclusiveNanoseconds) / static_cast<double>(Scope.Calls))
                : 0.0;

            Text.Append(String::Printf("  %s  avg=%.3fms  calls=%d\n", *Scope.Name,
                Time::ToMilliseconds(AverageNs), Scope.Calls));
        }
    }

    return Text;
}

String FProfilerReport::EscapeChromeString(const CHAR* Text)
{
    String Out;
    if (!Text)
    {
        return Out;
    }

    for (const CHAR* Cursor = Text; *Cursor; ++Cursor)
    {
        if (*Cursor == '"' || *Cursor == '\\')
        {
            Out.Append('\\');
        }

        if (*Cursor == '\n' || *Cursor == '\r')
        {
            Out.Append(' ');
            continue;
        }

        Out.Append(*Cursor);
    }

    return Out;
}

void FProfilerReport::AppendChromeCompleteEvent(String& Out, const CHAR* Name, const CHAR* Category,
    double TimestampUs, double DurationUs, int32 Pid, int64 Tid)
{
    AppendChromeComma(Out);
    Out.Append(String::Printf("{\"name\":\"%s\",\"cat\":\"%s\",\"ph\":\"X\",\"ts\":%.3f,\"dur\":%.3f,\"pid\":%d,\"tid\":%lld}",
        *EscapeChromeString(Name), Category ? Category : "cpu", TimestampUs, DurationUs, Pid, static_cast<long long>(Tid)));
}

void FProfilerReport::AppendChromeInstantEvent(String& Out, const CHAR* Name, const CHAR* Category,
    double TimestampUs, int32 Pid, int64 Tid)
{
    AppendChromeComma(Out);
    Out.Append(String::Printf("{\"name\":\"%s\",\"cat\":\"%s\",\"ph\":\"i\",\"s\":\"t\",\"ts\":%.3f,\"pid\":%d,\"tid\":%lld}",
        *EscapeChromeString(Name), Category ? Category : "cpu", TimestampUs, Pid, static_cast<long long>(Tid)));
}

uint64 FProfilerReport::GetChromeBaseTimeStamp()
{
    return ComputeChromeBaseTimeStamp();
}

bool FProfilerReport::TryGetChromeFrameTimestampUs(int32 CpuFrameIndex, uint64 FrameRelativeNanoseconds, double& OutTimestampUs)
{
    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();
    const FProfilerFrame* Frame         = FrameProfiler.FindFrame(CpuFrameIndex);
    if (!Frame)
    {
        OutTimestampUs = 0.0;
        return false;
    }

    const uint64 BaseStamp = ComputeChromeBaseTimeStamp();
    const uint64 Delta     = (BaseStamp != 0 && Frame->StartTimeStamp >= BaseStamp)
        ? (Frame->StartTimeStamp - BaseStamp)
        : 0;

    const double FrameStartUs =
        Time::ToMicroseconds(static_cast<double>(ProfilerTicksToNanoseconds(Delta, FrameProfiler.GetFrequency())));

    OutTimestampUs = FrameStartUs + Time::ToMicroseconds(static_cast<double>(FrameRelativeNanoseconds));
    return true;
}

String FProfilerReport::BuildChromeCpuJson()
{
    String Events("[");

    const FFrameProfiler& FrameProfiler = FFrameProfiler::Get();
    const uint64          Frequency     = FrameProfiler.GetFrequency();
    const uint64          BaseStamp     = ComputeChromeBaseTimeStamp();
    const FProfilerFrame& Boot          = FBootProfiler::Get().GetSession();
    const int32           StoredFrames  = FrameProfiler.GetStoredFrameCount();

    TMap<void*, int32> NamedThreads;
    auto NameThread = [&Events, &NamedThreads](void* Handle, int32 FallbackIndex)
    {
        if (!Handle || NamedThreads.Find(Handle))
        {
            return;
        }

        NamedThreads.Add(Handle, FallbackIndex);
        AppendChromeMetadata(Events, *ResolveThreadName(Handle, FallbackIndex), 0,
            static_cast<int64>(reinterpret_cast<UPTR_INT>(Handle)));
    };

    for (int32 ThreadIndex = 0; ThreadIndex < Boot.Threads.Size(); ++ThreadIndex)
    {
        NameThread(Boot.Threads[ThreadIndex].ThreadHandle, ThreadIndex);
    }

    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (!Frame)
        {
            continue;
        }

        for (int32 ThreadIndex = 0; ThreadIndex < Frame->Threads.Size(); ++ThreadIndex)
        {
            NameThread(Frame->Threads[ThreadIndex].ThreadHandle, ThreadIndex);
        }
    }

    if (!Boot.Threads.IsEmpty())
    {
        AppendFrameChromeEvents(Events, Boot, BaseStamp, Frequency, 0);
    }

    for (int32 FrameIndex = 0; FrameIndex < StoredFrames; ++FrameIndex)
    {
        const FProfilerFrame* Frame = FrameProfiler.GetStoredFrame(FrameIndex);
        if (Frame)
        {
            AppendFrameChromeEvents(Events, *Frame, BaseStamp, Frequency, 0);
        }
    }

    Events.Append("]");
    return Events;
}

String FProfilerReport::MergeChromeJson(const String& CpuJson, const String& ExtraEvents)
{
    if (ExtraEvents.IsEmpty())
    {
        return CpuJson;
    }

    String Merged = CpuJson;
    if (Merged.IsEmpty())
    {
        Merged = String("[");
        Merged.Append(ExtraEvents);
        Merged.Append("]");
        return Merged;
    }

    if (Merged[Merged.Length() - 1] == ']')
    {
        Merged.Pop();
    }

    if (!Merged.IsEmpty() && Merged[Merged.Length() - 1] != '[' && Merged[Merged.Length() - 1] != ',')
    {
        Merged.Append(",");
    }

    Merged.Append(ExtraEvents);
    Merged.Append("]");
    return Merged;
}

bool FProfilerReport::WriteFile(const String& Path, const String& Text)
{
    const String Directory = File::GetDirectoryOf(Path);
    if (!Directory.IsEmpty() && !File::CreateDirectoryTree(Directory))
    {
        LOG_ERROR("Failed to create profiling directory '%s'", *Directory);
        return false;
    }

    IPlatformFile* Output = FPlatformFile::OpenForWrite(Path, true);
    if (!Output || !Output->IsValid())
    {
        LOG_ERROR("Failed to open profiling report '%s'", *Path);
        if (Output)
        {
            Output->Close();
        }
        return false;
    }

    const bool bWrote = File::WriteTextFile(Output, Text);
    Output->Close();

    if (!bWrote)
    {
        LOG_ERROR("Failed to write profiling report '%s'", *Path);
        return false;
    }

    LOG_INFO("Wrote profiling report '%s'", *Path);
    return true;
}
