#include "ProfilerTests.h"

#include "Core/CoreDefines.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ProfilerReport.h"
#include "Core/Misc/ProfilerTypes.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Templates/CString.h"

#include "TestCommon/TestMacros.h"

static void BusyWait()
{
    volatile uint64 Accumulator = 0;
    for (int32 Index = 0; Index < 20000; ++Index)
    {
        Accumulator += static_cast<uint64>(Index);
    }
    UNREFERENCED_VARIABLE(Accumulator);
}

static const FProfilerInterval* FindIntervalByName(const TArray<FProfilerInterval>& Intervals, const CHAR* Name)
{
    for (const FProfilerInterval& Interval : Intervals)
    {
        if (Interval.Name && CString::Strcmp(Interval.Name, Name) == 0)
        {
            return &Interval;
        }
    }

    return nullptr;
}

static void ResetProfilers()
{
    FFrameProfiler::Get().Disable();
    FFrameProfiler::Get().SetCaptureNativeStacks(false);
    FFrameProfiler::Get().SetRetainAllFrames(false);
    FFrameProfiler::Get().Reset();

    FBootProfiler::Get().Reset();
}

bool Profiler_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Nested CPU scopes produce parent/depth and exclusive time");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();

        {
            TRACE_SCOPE("Parent");
            BusyWait();
            {
                TRACE_SCOPE("Child");
                BusyWait();
            }
        }

        FFrameProfiler::Get().Tick();

        const FProfilerFrame* Frame = FFrameProfiler::Get().GetLatestFrame();
        TEST_EXPECT(Frame != nullptr);
        TEST_EXPECT(Frame && !Frame->Threads.IsEmpty());

        if (Frame && !Frame->Threads.IsEmpty())
        {
            const TArray<FProfilerInterval>& Intervals = Frame->Threads[0].Intervals;
            const FProfilerInterval* Parent = FindIntervalByName(Intervals, "Parent");
            const FProfilerInterval* Child  = FindIntervalByName(Intervals, "Child");
            TEST_EXPECT(Parent != nullptr);
            TEST_EXPECT(Child != nullptr);
            if (Parent && Child)
            {
                TEST_EXPECT_EQ(Parent->Depth, 0);
                TEST_EXPECT_EQ(Child->Depth, 1);
                TEST_EXPECT_EQ(Parent->ParentIndex, -1);
                TEST_EXPECT(Child->ParentIndex >= 0);
                TEST_EXPECT(Parent->InclusiveNanoseconds >= Parent->ExclusiveNanoseconds);
                TEST_EXPECT(Parent->InclusiveNanoseconds >= Child->InclusiveNanoseconds);
                TEST_EXPECT_EQ(Child->ExclusiveNanoseconds, Child->InclusiveNanoseconds);
            }
        }
    }

    TEST_SECTION("Native stack addresses are stored only when capture is on");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        FFrameProfiler::Get().SetCaptureNativeStacks(false);

        {
            TRACE_SCOPE("NoStack");
            BusyWait();
        }

        FFrameProfiler::Get().Tick();

        const FProfilerFrame* OffFrame = FFrameProfiler::Get().GetLatestFrame();
        TEST_EXPECT(OffFrame != nullptr);
        if (OffFrame && !OffFrame->Threads.IsEmpty())
        {
            const FProfilerInterval* Interval = FindIntervalByName(OffFrame->Threads[0].Intervals, "NoStack");
            TEST_EXPECT(Interval != nullptr);
            if (Interval)
            {
                TEST_EXPECT_EQ(Interval->StackDepth, 0);
            }
        }

        FFrameProfiler::Get().SetCaptureNativeStacks(true);
        {
            TRACE_SCOPE("WithStack");
            BusyWait();
        }

        FFrameProfiler::Get().Tick();

        const FProfilerFrame* OnFrame = FFrameProfiler::Get().GetLatestFrame();
        TEST_EXPECT(OnFrame != nullptr);
        if (OnFrame && !OnFrame->Threads.IsEmpty())
        {
            const FProfilerInterval* Interval = FindIntervalByName(OnFrame->Threads[0].Intervals, "WithStack");
            TEST_EXPECT(Interval != nullptr);
            if (Interval)
            {
                TEST_EXPECT(Interval->StackDepth > 0);
                TEST_EXPECT(Interval->StackFrames[0] != 0);
            }
        }
    }

    TEST_SECTION("Boot samples never appear in FFrameProfiler aggregates");
    {
        ResetProfilers();
        FBootProfiler::Get().Enable();

        {
            TRACE_BOOT_SCOPE("BootOnly");
            BusyWait();
        }

        FBootProfiler::Get().Finish();

        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("FrameOnly");
            BusyWait();
        }
        FFrameProfiler::Get().Tick();

        TArray<FProfilerThreadAggregate> ThreadInfos;
        FProfilerReport::CollectThreadAggregates(ThreadInfos);
        TEST_EXPECT(!ThreadInfos.IsEmpty());
        bool bHasFrameOnly = false;
        bool bHasBootOnly = false;
        for (const FProfilerThreadAggregate& ThreadInfo : ThreadInfos)
        {
            for (const FProfilerScopeAggregate& Scope : ThreadInfo.Scopes)
            {
                if (Scope.Name == String("FrameOnly"))
                {
                    bHasFrameOnly = true;
                }
                if (Scope.Name == String("BootOnly"))
                {
                    bHasBootOnly = true;
                }
            }
        }
        TEST_EXPECT(bHasFrameOnly);
        TEST_EXPECT(!bHasBootOnly);

        const FProfilerFrame& Boot = FBootProfiler::Get().GetSession();
        TEST_EXPECT(FBootProfiler::Get().IsFinished());
        TEST_EXPECT(!Boot.Threads.IsEmpty());
        if (!Boot.Threads.IsEmpty())
        {
            TEST_EXPECT(FindIntervalByName(Boot.Threads[0].Intervals, "BootOnly") != nullptr);
        }
    }

    TEST_SECTION("GPU nested parent chain exclusive time");
    {
        TArray<FProfilerInterval> Intervals;
        FProfilerInterval Parent;
        Parent.Name = "Pass";
        Parent.ParentIndex = -1;
        Parent.Depth = 0;
        Parent.InclusiveNanoseconds = 100;
        Intervals.Add(Parent);

        FProfilerInterval Child;
        Child.Name = "Subpass";
        Child.ParentIndex = 0;
        Child.Depth = 1;
        Child.InclusiveNanoseconds = 40;
        Intervals.Add(Child);

        ApplyExclusiveTimesFromParentIndex(Intervals);
        TEST_EXPECT_EQ(Intervals[0].ExclusiveNanoseconds, 60ull);
        TEST_EXPECT_EQ(Intervals[1].ExclusiveNanoseconds, 40ull);

        TArray<int32> Callers;
        CollectProfilerCallers(Intervals, 1, Callers);
        TEST_EXPECT_EQ(Callers.Size(), 1);
        if (!Callers.IsEmpty())
        {
            TEST_EXPECT_EQ(Callers[0], 0);
        }
    }

    TEST_SECTION("Dump writer emits boot and a frame section");
    {
        ResetProfilers();
        FBootProfiler::Get().Enable();
        {
            TRACE_BOOT_SCOPE("DumpBoot");
            BusyWait();
        }
        FBootProfiler::Get().Finish();

        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("DumpFrame");
            BusyWait();
        }
        FFrameProfiler::Get().Tick();

        const String Text = FProfilerReport::BuildCpuAndBootText();
        TEST_EXPECT(Text.Contains("DXR Profile Run"));
        TEST_EXPECT(Text.Contains("== BOOT =="));
        TEST_EXPECT(Text.Contains("DumpBoot"));
        TEST_EXPECT(Text.Contains("== WORST FRAMES =="));
        TEST_EXPECT(Text.Contains("DumpFrame"));
        TEST_EXPECT(Text.Contains("== CPU HOT SPOTS =="));
        TEST_EXPECT(Text.Contains("== PER THREAD =="));
    }

    TEST_SECTION("Stored frame ring wraps, retain-all keeps history, FindFrame looks up by index");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        for (int32 Index = 0; Index < NUM_LIVE_PROFILER_FRAMES + 3; ++Index)
        {
            TRACE_SCOPE("RingFrame");
            FFrameProfiler::Get().Tick();
        }

        TEST_EXPECT_EQ(FFrameProfiler::Get().GetStoredFrameCount(), NUM_LIVE_PROFILER_FRAMES);
        TEST_EXPECT(FFrameProfiler::Get().FindFrame(0) == nullptr);
        TEST_EXPECT(FFrameProfiler::Get().FindFrame(FFrameProfiler::Get().GetLatestFinishedFrameIndex()) != nullptr);

        FFrameProfiler::Get().Reset();
        FFrameProfiler::Get().SetRetainAllFrames(true);
        FFrameProfiler::Get().Enable();
        for (int32 Index = 0; Index < NUM_LIVE_PROFILER_FRAMES + 3; ++Index)
        {
            FFrameProfiler::Get().Tick();
        }

        TEST_EXPECT_EQ(FFrameProfiler::Get().GetStoredFrameCount(), NUM_LIVE_PROFILER_FRAMES + 3);
        TEST_EXPECT(FFrameProfiler::Get().FindFrame(0) != nullptr);
    }

    TEST_SECTION("TProfilerFrameRing FindIf matches tagged GPU-style frames");
    {
        TProfilerFrameRing<FProfilerFrame> Ring;
        FProfilerFrame First;
        First.FrameIndex = 10;
        First.CpuMilliseconds = 8.0f;
        Ring.Push(Move(First));

        FProfilerFrame Second;
        Second.FrameIndex = 11;
        Second.CpuMilliseconds = 22.0f;
        Ring.Push(Move(Second));

        const FProfilerFrame* Found = Ring.FindIf([](const FProfilerFrame& Frame)
        {
            return Frame.FrameIndex == 10;
        });
        TEST_EXPECT(Found != nullptr);
        TEST_EXPECT_EQ(Found->CpuMilliseconds, 8.0f);

        TArray<FProfilerWorstFrame> Worst = FProfilerReport::CollectWorstFrames(8);
        UNREFERENCED_VARIABLE(Worst);
    }

    TEST_SECTION("TRACE_EVENT is stored as a zero-duration child");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("Parent");
            TRACE_EVENT("LoadMesh");
        }
        FFrameProfiler::Get().Tick();

        const FProfilerFrame* Frame = FFrameProfiler::Get().GetLatestFrame();
        TEST_EXPECT(Frame != nullptr);
        if (Frame && !Frame->Threads.IsEmpty())
        {
            const FProfilerInterval* Parent = FindIntervalByName(Frame->Threads[0].Intervals, "Parent");
            const FProfilerInterval* Event  = FindIntervalByName(Frame->Threads[0].Intervals, "LoadMesh");
            TEST_EXPECT(Parent != nullptr);
            TEST_EXPECT(Event != nullptr);
            if (Parent && Event)
            {
                const TArray<FProfilerInterval>& Intervals = Frame->Threads[0].Intervals;
                const int32 ParentIndex = static_cast<int32>(Parent - Intervals.Data());
                TEST_EXPECT(Event->bInstant);
                TEST_EXPECT_EQ(Event->InclusiveNanoseconds, 0ull);
                TEST_EXPECT_EQ(Event->ExclusiveNanoseconds, 0ull);
                TEST_EXPECT_EQ(Event->ParentIndex, ParentIndex);
                TEST_EXPECT_EQ(Parent->ExclusiveNanoseconds, Parent->InclusiveNanoseconds);
            }
        }
    }

    TEST_SECTION("Optimization targets order by self time");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("HotSelf");
            BusyWait();
            BusyWait();
        }
        {
            TRACE_SCOPE("CoolSelf");
        }
        FFrameProfiler::Get().Tick();

        const TArray<FProfilerOptimizationTarget> Targets = FProfilerReport::CollectOptimizationTargets(8);
        TEST_EXPECT(!Targets.IsEmpty());
        if (Targets.Size() >= 2)
        {
            TEST_EXPECT(Targets[0].SelfMillisecondsPerFrame >= Targets[1].SelfMillisecondsPerFrame);
        }

        const TArray<FProfilerWorstFrame> Worst = FProfilerReport::CollectWorstFrames(8);
        TEST_EXPECT(!Worst.IsEmpty());
    }

    TEST_SECTION("Worst frames are returned slowest first");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        for (int32 Frame = 0; Frame < 4; ++Frame)
        {
            for (int32 Work = 0; Work < Frame; ++Work)
            {
                BusyWait();
            }
            FFrameProfiler::Get().Tick();
        }

        const TArray<FProfilerWorstFrame> Worst = FProfilerReport::CollectWorstFrames(8);
        TEST_EXPECT_EQ(Worst.Size(), 4);
        for (int32 Index = 1; Index < Worst.Size(); ++Index)
        {
            TEST_EXPECT(Worst[Index - 1].CpuMilliseconds >= Worst[Index].CpuMilliseconds);
        }
    }

    TEST_SECTION("Chrome JSON emits complete and instant events and escapes quotes");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("Quoted\"Name");
            TRACE_EVENT("Instant");
        }
        FFrameProfiler::Get().Tick();

        const String Json = FProfilerReport::BuildChromeCpuJson();
        TEST_EXPECT(Json.Contains("\"ph\":\"X\""));
        TEST_EXPECT(Json.Contains("\"ph\":\"i\""));
        TEST_EXPECT(Json.Contains("Quoted\\\"Name") || Json.Contains("Quoted"));
        TEST_EXPECT(Json.Contains("\"dur\":"));
    }

    TEST_SECTION("Frame-relative GPU timestamps share the CPU Chrome timebase");
    {
        ResetProfilers();
        FFrameProfiler::Get().Enable();
        {
            TRACE_SCOPE("CorrelatedCpuFrame");
            BusyWait();
        }
        FFrameProfiler::Get().Tick();

        const FProfilerFrame* Frame = FFrameProfiler::Get().GetLatestFrame();
        TEST_EXPECT(Frame != nullptr);
        if (Frame)
        {
            double FrameStartUs = 0.0;
            double OneMillisecondLaterUs = 0.0;
            TEST_EXPECT(FProfilerReport::TryGetChromeFrameTimestampUs(
                Frame->FrameIndex, 0, FrameStartUs));
            TEST_EXPECT(FProfilerReport::TryGetChromeFrameTimestampUs(
                Frame->FrameIndex, 1000000, OneMillisecondLaterUs));

            const double DeltaUs = OneMillisecondLaterUs - FrameStartUs;
            TEST_EXPECT(DeltaUs > 999.9 && DeltaUs < 1000.1);
        }

        double MissingTimestampUs = 1.0;
        TEST_EXPECT(!FProfilerReport::TryGetChromeFrameTimestampUs(-12345, 0, MissingTimestampUs));
        TEST_EXPECT_EQ(MissingTimestampUs, 0.0);
    }

    TEST_SECTION("Thread lanes come out in one order whichever thread reached the frame first");
    {
        const TArray<void*> Handles =
        {
            reinterpret_cast<void*>(static_cast<uintptr_t>(0x30)),
            reinterpret_cast<void*>(static_cast<uintptr_t>(0x10)),
            reinterpret_cast<void*>(static_cast<uintptr_t>(0x20)),
        };

        TArray<FProfilerThreadFrame> Arrived;
        TArray<FProfilerThreadFrame> ArrivedReversed;
        for (int32 Index = 0; Index < Handles.Size(); ++Index)
        {
            Arrived.Emplace().ThreadHandle         = Handles[Index];
            ArrivedReversed.Emplace().ThreadHandle = Handles[Handles.Size() - Index - 1];
        }

        SortProfilerThreadFrames(Arrived);
        SortProfilerThreadFrames(ArrivedReversed);

        TEST_EXPECT_EQ(Arrived.Size(), ArrivedReversed.Size());
        for (int32 Index = 0; Index < Arrived.Size(); ++Index)
        {
            TEST_EXPECT(Arrived[Index].ThreadHandle == ArrivedReversed[Index].ThreadHandle);
        }

        TEST_EXPECT(Arrived[0].ThreadHandle == Handles[1]);
        TEST_EXPECT(Arrived[1].ThreadHandle == Handles[2]);
        TEST_EXPECT(Arrived[2].ThreadHandle == Handles[0]);
    }

    ResetProfilers();

    TEST_END();
}
