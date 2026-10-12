#include "MemoryTrackingTests.h"

#include <Core/Memory/Memory.h>
#include <Core/Memory/MemoryProfiler.h>
#include <Core/Platform/PlatformMemory.h>
#include <Core/Platform/PlatformThread.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Tasks/Tasks.h>
#include <Core/Templates/CString.h>
#include <Core/Threading/Runnable.h>

#include "TestCommon/TestMacros.h"

#if ENABLE_MEMORY_PROFILER

struct FAllocatingRunnable : public FRunnable
{
    explicit FAllocatingRunnable(FMemoryNameId InTag)
        : Tag(InTag)
    {
    }

    virtual int32 Run() override
    {
        FMemoryProfiler::Get().PushTag(Tag);
        Block = Memory::Malloc(256);
        FMemoryProfiler::Get().PopTag();
        return 0;
    }

    FMemoryNameId Tag;
    void*         Block = nullptr;
};

#endif

bool MemoryTracking_Test()
{
    TEST_BEGIN();

#if ENABLE_MEMORY_PROFILER
    FMemoryProfiler& Profiler = FMemoryProfiler::Get();
    TEST_CHECK(Profiler.IsTrackingEnabled());

    TEST_SECTION("An allocation is billed to the current tag, and freeing it anywhere gives the bytes back");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.Balance");

        Profiler.PushTag(Tag);
        void* Block = Memory::Malloc(100);
        Profiler.PopTag();

        FMemoryCounterSnapshot Stats = Profiler.GetTagStats(Tag);
        TEST_EXPECT_EQ(Stats.CurrentBytes, 100);
        TEST_EXPECT_EQ(Stats.NumAllocations, 1);
        TEST_EXPECT_EQ(Stats.TotalAllocations, 1);
        TEST_EXPECT_EQ(Stats.PeakBytes, 100);

        Memory::Free(Block);

        Stats = Profiler.GetTagStats(Tag);
        TEST_EXPECT_EQ(Stats.CurrentBytes, 0);
        TEST_EXPECT_EQ(Stats.NumAllocations, 0);
        TEST_EXPECT_EQ(Stats.TotalAllocations, 1);
        TEST_EXPECT_EQ(Stats.PeakBytes, 100);
    }

    TEST_SECTION("The peak is the most that was live at once");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.Peak");

        Profiler.PushTag(Tag);
        void* First  = Memory::Malloc(1000);
        void* Second = Memory::Malloc(500);
        Memory::Free(First);
        void* Third  = Memory::Malloc(200);
        Profiler.PopTag();

        FMemoryCounterSnapshot Stats = Profiler.GetTagStats(Tag);
        TEST_EXPECT_EQ(Stats.CurrentBytes, 700);
        TEST_EXPECT_EQ(Stats.PeakBytes, 1500);
        TEST_EXPECT_EQ(Stats.NumAllocations, 2);
        TEST_EXPECT_EQ(Stats.TotalAllocations, 3);

        Memory::Free(Second);
        Memory::Free(Third);

        Stats = Profiler.GetTagStats(Tag);
        TEST_EXPECT_EQ(Stats.CurrentBytes, 0);
        TEST_EXPECT_EQ(Stats.PeakBytes, 1500);
    }

    TEST_SECTION("A realloc bills the new block to the tag current at the realloc");
    {
        const FMemoryNameId First  = Profiler.InternDynamic("MemoryTrackingTest.ReallocFrom");
        const FMemoryNameId Second = Profiler.InternDynamic("MemoryTrackingTest.ReallocTo");

        Profiler.PushTag(First);
        void* Block = Memory::Malloc(100);
        Profiler.PopTag();

        Profiler.PushTag(Second);
        Block = Memory::Realloc(Block, 300);
        Profiler.PopTag();

        TEST_EXPECT_EQ(Profiler.GetTagStats(First).CurrentBytes, 0);
        TEST_EXPECT_EQ(Profiler.GetTagStats(First).NumAllocations, 0);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Second).CurrentBytes, 300);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Second).NumAllocations, 1);

        Memory::Free(Block);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Second).CurrentBytes, 0);
    }

    TEST_SECTION("A realloc from nullptr is an allocation, and one to zero bytes gives the old bytes back");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.ReallocEdges");

        Profiler.PushTag(Tag);
        void* Block = Memory::Realloc(nullptr, 64);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 64);

        void* Shrunk = Memory::Realloc(Block, 0);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 0);
        Profiler.PopTag();

        if (Shrunk)
        {
            Memory::Free(Shrunk);
        }

        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).NumAllocations, 0);
    }

    TEST_SECTION("Freeing a block that was allocated outside the tracker is ignored");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.Untracked");

        Profiler.PushTag(Tag);
        void* Untracked = FPlatformMemory::Malloc(64, STANDARD_ALIGNMENT);
        TEST_CHECK(Untracked != nullptr);
        Memory::Free(Untracked);
        Profiler.PopTag();

        const FMemoryCounterSnapshot Stats = Profiler.GetTagStats(Tag);
        TEST_EXPECT_EQ(Stats.CurrentBytes, 0);
        TEST_EXPECT_EQ(Stats.NumAllocations, 0);
        TEST_EXPECT_EQ(Stats.TotalAllocations, 0);
    }

    TEST_SECTION("A block freed on another thread still comes off the tag it was allocated under");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.CrossThread");

        FAllocatingRunnable* Runnable = new FAllocatingRunnable(Tag);
        IPlatformThread*     Thread   = FPlatformThread::Create(Runnable, "MemoryTrackingTest", false);
        Thread->WaitForCompletion();

        TEST_EXPECT(Runnable->Block != nullptr);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 256);

        Memory::Free(Runnable->Block);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 0);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).NumAllocations, 0);

        delete Thread;
        delete Runnable;
    }

    TEST_SECTION("Allocations inside a scope count against that scope node");
    {
        const FMemoryScopeId PreviousScope = Profiler.PushScope(Profiler.InternDynamic("MemoryTrackingTest.Scope"));
        const FMemoryScopeId Node          = Profiler.GetCurrentScope();
        void* Block = Memory::Malloc(128);
        Profiler.PopScope(PreviousScope);

        TEST_EXPECT_EQ(Profiler.GetScopeStats(Node).CurrentBytes, 128);
        TEST_EXPECT_EQ(Profiler.GetScopeStats(Node).NumAllocations, 1);

        Memory::Free(Block);
        TEST_EXPECT_EQ(Profiler.GetScopeStats(Node).CurrentBytes, 0);
    }

    TEST_SECTION("A launched task runs under the tag and scope it was launched from, and bills its allocations to them");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.Task");

        Profiler.PushTag(Tag);
        const FMemoryScopeId PreviousScope = Profiler.PushScope(Profiler.InternDynamic("MemoryTrackingTest.TaskScope"));
        const FMemoryScopeId Node          = Profiler.GetCurrentScope();

        constexpr int64 BlockSize = 4096;

        FMemoryNameId  SeenTag          = MEMORY_NAME_UNTAGGED;
        FMemoryScopeId SeenScope        = MEMORY_SCOPE_ROOT;
        int64          TagBytesBefore   = 0;
        int64          TagBytesAfter    = 0;
        int64          ScopeBytesBefore = 0;
        int64          ScopeBytesAfter  = 0;
        void*          Block            = nullptr;

        FTaskHandle Handle = Tasks::Launch("MemoryTrackingTest", [&]()
        {
            SeenTag          = Profiler.GetCurrentTag();
            SeenScope        = Profiler.GetCurrentScope();
            TagBytesBefore   = Profiler.GetTagStats(Tag).CurrentBytes;
            ScopeBytesBefore = Profiler.GetScopeStats(Node).CurrentBytes;
            Block            = Memory::Malloc(BlockSize);
            TagBytesAfter    = Profiler.GetTagStats(Tag).CurrentBytes;
            ScopeBytesAfter  = Profiler.GetScopeStats(Node).CurrentBytes;
        });

        Profiler.PopScope(PreviousScope);
        Profiler.PopTag();
        Handle.Wait();

        TEST_EXPECT_EQ(SeenTag, Tag);
        TEST_EXPECT_EQ(SeenScope, Node);
        TEST_EXPECT_EQ(TagBytesAfter - TagBytesBefore, BlockSize);
        TEST_EXPECT_EQ(ScopeBytesAfter - ScopeBytesBefore, BlockSize);

        const int64 TagBytesBeforeFree = Profiler.GetTagStats(Tag).CurrentBytes;
        Memory::Free(Block);
        TEST_EXPECT_EQ(TagBytesBeforeFree - Profiler.GetTagStats(Tag).CurrentBytes, BlockSize);

        TEST_SECTION("A task launched with nothing pushed stays untagged, so a worker carries nothing over from the last task");
        FMemoryNameId  UntaggedTag   = Tag;
        FMemoryScopeId UntaggedScope = Node;

        Tasks::Launch("MemoryTrackingTestUntagged", [&]()
        {
            UntaggedTag   = Profiler.GetCurrentTag();
            UntaggedScope = Profiler.GetCurrentScope();
        }).Wait();

        TEST_EXPECT_EQ(UntaggedTag, MEMORY_NAME_UNTAGGED);
        TEST_EXPECT_EQ(UntaggedScope, MEMORY_SCOPE_ROOT);
    }

    TEST_SECTION("Every chunk of a ParallelFor runs under the caller's tag");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.ParallelFor");

        AtomicInt32 NumUnderTag(0);
        Profiler.PushTag(Tag);
        Tasks::ParallelFor(256, [&](int32)
        {
            if (Profiler.GetCurrentTag() == Tag)
            {
                NumUnderTag.Increment();
            }
        });
        Profiler.PopTag();

        TEST_EXPECT_EQ(NumUnderTag.Load(), 256);
    }

    TEST_SECTION("New-expressions are tracked like any other allocation");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.New");

        Profiler.PushTag(Tag);
        int64* Value = new int64(7);
        Profiler.PopTag();

        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, static_cast<int64>(sizeof(int64)));

        delete Value;
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 0);
    }

    TEST_SECTION("The profiler's own pages are reported under its own tag");
    {
        const FMemoryCounterSnapshot Stats = Profiler.GetTagStats(Profiler.GetProfilerTag());
        TEST_EXPECT(Stats.CurrentBytes > 0);
        TEST_EXPECT(Stats.PeakBytes >= Stats.CurrentBytes);
        TEST_EXPECT_EQ(CString::Strcmp(Profiler.GetRegistry().GetName(Profiler.GetProfilerTag()), "MemoryProfiler"), 0);
    }

    TEST_SECTION("With tracking off nothing is counted, and turning it back on starts clean");
    {
        const FMemoryNameId Tag = Profiler.InternDynamic("MemoryTrackingTest.Toggle");

        Profiler.SetTrackingEnabled(false);
        TEST_EXPECT(!Profiler.IsTrackingEnabled());
        TEST_EXPECT_EQ(Profiler.GetNumTrackedAllocations(), 0);

        Profiler.PushTag(Tag);
        void* Untracked = Memory::Malloc(64);
        Profiler.PopTag();

        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 0);

        Profiler.SetTrackingEnabled(true);
        Memory::Free(Untracked);

        Profiler.PushTag(Tag);
        void* Tracked = Memory::Malloc(32);
        Profiler.PopTag();

        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 32);
        Memory::Free(Tracked);
        TEST_EXPECT_EQ(Profiler.GetTagStats(Tag).CurrentBytes, 0);
    }
#endif

    TEST_END();
}
