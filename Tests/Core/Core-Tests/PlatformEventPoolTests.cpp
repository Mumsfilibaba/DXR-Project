#include "PlatformEventPoolTests.h"

#include <Core/Containers/Set.h>
#include <Core/Platform/PlatformEvent.h>
#include <Core/Platform/PlatformTime.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Threading/Atomic/AtomicInt.h>
#include <Core/PlatformInterface/PlatformEventPool.h>
#include <Core/Threading/ScopedLock.h>
#include <Core/Time/Timespan.h>

#include "TestCommon/TestMacros.h"

bool PlatformEventPool_Test()
{
    TEST_BEGIN();

    FPlatformEventPool& Pool = FPlatformEventPool::Get();
    Pool.Flush();

    TEST_SECTION("Recycled identity");
    {
        Pool.Flush();

        IPlatformEvent* First = FPlatformEvent::Create(false);
        TEST_EXPECT(First != nullptr);

        FPlatformEvent::Recycle(First);
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 1);

        IPlatformEvent* Second = FPlatformEvent::Create(false);
        TEST_EXPECT_EQ(Second, First);
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 0);

        FPlatformEvent::Recycle(Second);
    }

    TEST_SECTION("Reset modes do not cross");
    {
        Pool.Flush();

        IPlatformEvent* ManualEvent = FPlatformEvent::Create(true);
        TEST_EXPECT(ManualEvent != nullptr);
        FPlatformEvent::Recycle(ManualEvent);

        IPlatformEvent* AutoEvent = FPlatformEvent::Create(false);
        TEST_EXPECT(AutoEvent != nullptr);
        TEST_EXPECT(AutoEvent != ManualEvent);
        TEST_EXPECT(!AutoEvent->IsManualReset());
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(true), 1);
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 0);

        FPlatformEvent::Recycle(AutoEvent);
    }

    TEST_SECTION("A recycled event comes back unsignalled");
    {
        Pool.Flush();

        IPlatformEvent* Event = FPlatformEvent::Create(true);
        TEST_EXPECT(Event != nullptr);
        Event->Trigger();
        FPlatformEvent::Recycle(Event);

        IPlatformEvent* Recycled = FPlatformEvent::Create(true);
        TEST_EXPECT_EQ(Recycled, Event);

        const uint64 Frequency = FPlatformTime::QueryPerformanceFrequency();
        const uint64 Start     = FPlatformTime::QueryPerformanceCounter();
        Recycled->Wait(100);
        const uint64 End = FPlatformTime::QueryPerformanceCounter();

        const double ElapsedMS = (static_cast<double>(End - Start) / static_cast<double>(Frequency)) * 1000.0;
        TEST_EXPECT(ElapsedMS >= 50.0);

        FPlatformEvent::Recycle(Recycled);
    }

    TEST_SECTION("The free list is capped at MaxRetainedEvents");
    {
        Pool.Flush();

        constexpr int32 NumEvents = FPlatformEventPool::MaxRetainedEvents + 8;
        IPlatformEvent* Events[NumEvents] = {};
        for (int32 Index = 0; Index < NumEvents; Index++)
        {
            Events[Index] = FPlatformEvent::Create(false);
            TEST_EXPECT(Events[Index] != nullptr);
        }

        for (int32 Index = 0; Index < NumEvents; Index++)
        {
            FPlatformEvent::Recycle(Events[Index]);
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), FPlatformEventPool::MaxRetainedEvents);
        Pool.Flush();
    }

    TEST_SECTION("An event nothing asked for over a whole window is pruned");
    {
        Pool.Flush();
        {
            IPlatformEvent* Idle = FPlatformEvent::Create(false);
            TEST_EXPECT(Idle != nullptr);
            FPlatformEvent::Recycle(Idle);
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 1);

        for (int32 Frame = 0; Frame < FPlatformEventPool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 1);

        for (int32 Frame = 0; Frame < FPlatformEventPool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 0);
    }

    TEST_SECTION("An event in demand every frame survives the window");
    {
        Pool.Flush();
        {
            IPlatformEvent* Warmup = FPlatformEvent::Create(false);
            TEST_EXPECT(Warmup != nullptr);
            FPlatformEvent::Recycle(Warmup);
        }

        for (int32 Frame = 0; Frame < FPlatformEventPool::PruneIntervalFrames; Frame++)
        {
            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 1);

        for (int32 Frame = 0; Frame < FPlatformEventPool::PruneIntervalFrames; Frame++)
        {
            IPlatformEvent* InUse = FPlatformEvent::Create(false);
            TEST_EXPECT(InUse != nullptr);
            FPlatformEvent::Recycle(InUse);

            Pool.Tick();
        }

        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 1);
    }

    TEST_SECTION("Flush empties both buckets");
    {
        Pool.Flush();

        IPlatformEvent* AutoEvent = FPlatformEvent::Create(false);
        IPlatformEvent* ManualEvent = FPlatformEvent::Create(true);
        TEST_EXPECT(AutoEvent != nullptr);
        TEST_EXPECT(ManualEvent != nullptr);

        FPlatformEvent::Recycle(AutoEvent);
        FPlatformEvent::Recycle(ManualEvent);

        TEST_EXPECT(Pool.GetNumFreeEvents() > 0);

        Pool.Flush();
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(), 0);
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(false), 0);
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(true), 0);
    }

    TEST_SECTION("Events on many threads are never handed out twice");
    {
        Pool.Flush();

        constexpr int32 NumIterations = 256;

        AtomicInt32 NumDuplicate(0);
        TSet<IPlatformEvent*> LiveEvents;
        FCriticalSection      LiveEventsCS;

        Tasks::ParallelFor(NumIterations, [&](int32 Index)
        {
            UNREFERENCED_VARIABLE(Index);

            for (int32 Round = 0; Round < 4; Round++)
            {
                IPlatformEvent* Event = FPlatformEvent::Create(false);
                TEST_EXPECT(Event != nullptr);

                {
                    SCOPED_LOCK(LiveEventsCS);
                    if (LiveEvents.Contains(Event))
                    {
                        NumDuplicate.Increment();
                    }
                    else
                    {
                        LiveEvents.Add(Event);
                    }
                }

                Event->Trigger();
                Event->Wait(FTimespan::Infinity());

                {
                    SCOPED_LOCK(LiveEventsCS);
                    LiveEvents.Remove(Event);
                }

                FPlatformEvent::Recycle(Event);
            }
        });

        TEST_EXPECT_EQ(NumDuplicate.Load(), 0);
        TEST_EXPECT(Pool.GetNumFreeEvents(false) <= FPlatformEventPool::MaxRetainedEvents);

        Pool.Flush();
        TEST_EXPECT_EQ(Pool.GetNumFreeEvents(), 0);
    }

    Pool.Flush();
    TEST_END();
}
