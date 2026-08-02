#include "PriorityQueue_Test.h"

#if RUN_TPRIORITYQUEUE_TEST
#include "TestUtils.h"

#include <Core/Containers/PriorityQueue.h>

bool TPriorityQueue_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Enqueue / Size / Peek / Dequeue priority order");
    {
        TPriorityQueue<int32> Queue;
        TEST_EXPECT_EQ(Queue.Size(), 0);

        Queue.Enqueue(10, EQueuePriority::Normal);
        Queue.Enqueue(20, EQueuePriority::Highest);
        Queue.Enqueue(30, EQueuePriority::Low);

        TEST_EXPECT_EQ(Queue.Size(), 3);

        EQueuePriority PeekPriority = EQueuePriority::Count;
        int32* Front = Queue.Peek(&PeekPriority);
        TEST_EXPECT(Front != nullptr && *Front == 20);
        TEST_EXPECT(PeekPriority == EQueuePriority::Highest);

        int32          Out         = 0;
        EQueuePriority OutPriority = EQueuePriority::Count;
        TEST_EXPECT(Queue.Dequeue(&Out, &OutPriority));
        TEST_EXPECT(Out == 20 && OutPriority == EQueuePriority::Highest);

        TEST_EXPECT(Queue.Dequeue(&Out));
        TEST_EXPECT(Out == 10); // Normal outranks Low

        TEST_EXPECT(Queue.Dequeue(&Out));
        TEST_EXPECT(Out == 30);

        TEST_EXPECT(!Queue.Dequeue(&Out));
        TEST_EXPECT_EQ(Queue.Size(), 0);
    }

    TEST_SECTION("FIFO within a single priority");
    {
        TPriorityQueue<int32> Queue;
        Queue.Enqueue(1, EQueuePriority::Normal);
        Queue.Enqueue(2, EQueuePriority::Normal);
        Queue.Enqueue(3, EQueuePriority::Normal);

        int32 Out = 0;
        TEST_EXPECT(Queue.Dequeue(&Out) && Out == 1);
        TEST_EXPECT(Queue.Dequeue(&Out) && Out == 2);
        TEST_EXPECT(Queue.Dequeue(&Out) && Out == 3);
    }

    TEST_SECTION("Remove / Reset");
    {
        TPriorityQueue<int32> Queue;
        Queue.Enqueue(1, EQueuePriority::Normal);
        Queue.Enqueue(2, EQueuePriority::Normal);
        Queue.Enqueue(3, EQueuePriority::High);

        TEST_EXPECT(Queue.Remove(2));
        TEST_EXPECT_EQ(Queue.Size(), 2);
        TEST_EXPECT(!Queue.Remove(99));

        Queue.Reset();
        TEST_EXPECT_EQ(Queue.Size(), 0);
    }

    TEST_SECTION("TPriorityQueue stress (FInstanced, random priorities, ordered drain, no leaks)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom Random(Seed);

            TPriorityQueue<FInstanced> Queue;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const EQueuePriority Priority = static_cast<EQueuePriority>(Random.RandInt(0, 4));
                Queue.Enqueue(FInstanced(Step), Priority);
            }

            TEST_EXPECT_EQ(Queue.Size(), TargetSize);
            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            // A pure drain must return elements in non-decreasing priority-index order (Highest first).
            int32          LastPriority = -1;
            int32          DequeueCount = 0;
            bool           bOrdered     = true;
            EQueuePriority OutPriority  = EQueuePriority::Count;
            
            FInstanced Out;
            while (Queue.Dequeue(&Out, &OutPriority))
            {
                const int32 PriorityIndex = static_cast<int32>(OutPriority);
                bOrdered = bOrdered && (PriorityIndex >= LastPriority) && Out.IsPayloadValid();
                LastPriority = PriorityIndex;
                ++DequeueCount;
            }

            if (!bOrdered)
            {
                LOG_ERROR("[STRESS FAIL] TPriorityQueue seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bOrdered);
            TEST_EXPECT_EQ(DequeueCount, TargetSize);
            TEST_EXPECT_EQ(Queue.Size(), 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
