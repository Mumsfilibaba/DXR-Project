#include "MemoryStackTests.h"

#include <Core/Math/Math.h>
#include <Core/Memory/MemoryPagePool.h>
#include <Core/Memory/MemoryStack.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Threading/Atomic/AtomicInt.h>

#include "TestCommon/TestMacros.h"

bool MemoryStack_Test()
{
    TEST_BEGIN();

    FMemoryPagePool& Pool = FMemoryPagePool::Get();
    Pool.Flush();

    TEST_SECTION("Allocations are aligned and do not overlap");
    {
        FMemoryStack Stack;
        uint8* First  = Stack.PushBytes(24, 16);
        uint8* Second = Stack.PushBytes(24, 16);

        const UPTR_INT FirstAddress  = reinterpret_cast<UPTR_INT>(First);
        const UPTR_INT SecondAddress = reinterpret_cast<UPTR_INT>(Second);
        TEST_EXPECT_EQ(Math::AlignUp<UPTR_INT>(FirstAddress, 16), FirstAddress);
        TEST_EXPECT_EQ(Math::AlignUp<UPTR_INT>(SecondAddress, 16), SecondAddress);
        TEST_EXPECT(Second >= First + 24);
    }

    TEST_SECTION("A page holds exactly PageSize bytes before it spills");
    {
        FMemoryStack Stack;

        uint8* First = Stack.PushBytes(64, 16);
        uint8* Rest  = Stack.PushBytes(FMemoryStack::PageSize - 64, 1);

        TEST_EXPECT_EQ(Rest, First + 64);
        TEST_EXPECT_EQ(Stack.GetNumAllocatedBytes(), FMemoryStack::PageSize);

        Stack.PushBytes(1, 1);
        TEST_EXPECT_EQ(Stack.GetNumAllocatedBytes(), FMemoryStack::PageSize * 2);
    }

    TEST_SECTION("A reset page comes back to the next stack");
    {
        Pool.Flush();

        void* FirstPage = nullptr;
        {
            FMemoryStack Stack;
            FirstPage = Stack.Allocate(64);
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 1);
        TEST_EXPECT_EQ(Pool.GetNumPooledBytes(), FMemoryPagePool::BlockSize);

        FMemoryStack Reused;
        TEST_EXPECT_EQ(Reused.Allocate(64), FirstPage);
        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 0);
    }

    TEST_SECTION("Oversized allocations are not pooled");
    {
        Pool.Flush();
        {
            FMemoryStack Stack;
            Stack.Allocate(FMemoryStack::PageSize * 2);
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 0);
    }

    TEST_SECTION("The free list is capped at MaxRetainedPages");
    {
        Pool.Flush();

        constexpr int32 NumStacks = FMemoryPagePool::MaxRetainedPages + 8;
        {
            FMemoryStack Stacks[NumStacks];
            for (int32 Index = 0; Index < NumStacks; Index++)
            {
                Stacks[Index].Allocate(64);
            }
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), FMemoryPagePool::MaxRetainedPages);
        Pool.Flush();
    }

    TEST_SECTION("A page nothing asked for over a whole window is pruned");
    {
        Pool.Flush();
        {
            FMemoryStack Idle;
            Idle.Allocate(64);
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 1);

        for (int32 Frame = 0; Frame < FMemoryPagePool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 1);

        for (int32 Frame = 0; Frame < FMemoryPagePool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 0);
    }

    TEST_SECTION("A page in demand every frame survives the window");
    {
        Pool.Flush();
        {
            FMemoryStack Warmup;
            Warmup.Allocate(64);
        }

        for (int32 Frame = 0; Frame < FMemoryPagePool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 1);

        for (int32 Frame = 0; Frame < FMemoryPagePool::PruneIntervalFrames; Frame++)
        {
            FMemoryStack InUse;
            InUse.Allocate(64);
            InUse.Reset();

            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 1);
    }

    TEST_SECTION("Flush empties the pool");
    {
        {
            FMemoryStack Stack;
            Stack.Allocate(64);
        }

        TEST_EXPECT(Pool.GetNumFreePages() > 0);

        Pool.Flush();
        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 0);
        TEST_EXPECT_EQ(Pool.GetNumPooledBytes(), 0);
    }

    TEST_SECTION("IsEmpty tracks whether the stack holds a page");
    {
        FMemoryStack Stack;
        TEST_EXPECT(Stack.IsEmpty());

        Stack.Allocate(64);
        TEST_EXPECT(!Stack.IsEmpty());

        Stack.Reset();
        TEST_EXPECT(Stack.IsEmpty());
    }

    TEST_SECTION("Stacks on many threads do not hand out the same page twice");
    {
        Pool.Flush();

        constexpr int32 NumIterations = 256;
        constexpr int32 NumBytes      = 1024;

        AtomicInt32 NumCorrupted(0);
        Tasks::ParallelFor(NumIterations, [&NumCorrupted](int32 Index)
        {
            FMemoryStack Stack;

            const uint8 Pattern = static_cast<uint8>(Index);
            for (int32 Round = 0; Round < 4; Round++)
            {
                uint8* Bytes = Stack.PushBytes(NumBytes, 16);
                for (int32 Byte = 0; Byte < NumBytes; Byte++)
                {
                    Bytes[Byte] = Pattern;
                }

                for (int32 Byte = 0; Byte < NumBytes; Byte++)
                {
                    if (Bytes[Byte] != Pattern)
                    {
                        NumCorrupted.Increment();
                        break;
                    }
                }

                Stack.Reset();
            }
        });

        TEST_EXPECT_EQ(NumCorrupted.Load(), 0);
        TEST_EXPECT(Pool.GetNumFreePages() <= FMemoryPagePool::MaxRetainedPages);

        Pool.Flush();
        TEST_EXPECT_EQ(Pool.GetNumFreePages(), 0);
    }

    Pool.Flush();
    TEST_END();
}
