#include "Queue_Test.h"

#if RUN_TQUEUE_TEST
#include "TestUtils.h"

#include <Core/Containers/Queue.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Threading/Runnable.h>
#include <Core/Threading/Atomic/AtomicInt.h>
#include <Core/Platform/PlatformThread.h>

// Single Producer, Single Consumer
namespace SPSCTest
{
    static bool Test()
    {
        TQueue<String, EQueueType::SPSC> Queue;
        for (int64 Index = 1; Index <= 50; ++Index)
        {
            const String Item = TTypeToString<int64>::ToString(Index);
            Queue.Enqueue(::Move(Item));
        }

        TArray<String> Items;
        while (Items.Size() < 50)
        {
            String NewItem;
            if (Queue.Dequeue(NewItem))
            {
                Items.Add(::Move(NewItem));
            }
        }

        for (int32 Index = 0; Index < 50; ++Index)
        {
            const String ExpectedItem = TTypeToString<int64>::ToString(Index + 1);
            if (Items[Index] != ExpectedItem)
            {
                return false;
            }
        }

        return true;
    }
}

// Multiple Producers, Single Consumer
namespace MPSCTest
{
    static TQueue<String, EQueueType::MPSC>* GQueue = nullptr;

    // Counts producers that have not finished yet. The consumer may only stop once the last
    // one is done, so a single shared flag would let it quit while other producers are still
    // enqueueing. It has to be atomic for the consumer to observe the writes at all.
    static AtomicInt32 GActiveProducers(0);

    constexpr int64 NumItemsPerProducer = 500;
    constexpr int64 ProducerOffset      = 1000;
    constexpr int64 NumProducers        = 6;

    struct FProducerThread : public FRunnable
    {
        FProducerThread(int64 InThreadIndex)
            : ThreadIndex(InThreadIndex)
        {
        }

        int32 Run()
        {
            for (int64 Index = 0; Index < NumItemsPerProducer; ++Index)
            {
                const String NewItem = TTypeToString<int64>::ToString(ThreadIndex + Index);
                GQueue->Emplace(::Move(NewItem));
            }

            GActiveProducers.Decrement();
            return 0;
        }

        void Destroy()
        {
            delete this;
        }

        int64 ThreadIndex;
    };

    static TArray<String>* GItems = nullptr;

    struct FConsumerThread : public FRunnable
    {
        int32 Run()
        {
            while (GActiveProducers.Load() > 0 || !GQueue->IsEmpty())
            {
                String NewItem;
                if (GQueue->Dequeue(NewItem))
                {
                    GItems->Add(::Move(NewItem));
                }
            }

            return 0;
        }

        void Destroy()
        {
            delete this;
        }
    };

    static bool Test()
    {
        bool bResult = true;

        GActiveProducers.Store(static_cast<int32>(NumProducers));
        GQueue = new TQueue<String, EQueueType::MPSC>;
        GItems = new TArray<String>;

        TArray<FGenericPlatformThread*> Producers;
        for (int32 i = 0; i < NumProducers; ++i)
        {
            const int32 ProducerIndex = (i + 1) * ProducerOffset;
            Producers.Add(FPlatformThread::Create(new FProducerThread(ProducerIndex), "ProducerThread", false));
        }

        FGenericPlatformThread* Consumer = FPlatformThread::Create(new FConsumerThread, "ConsumerThread", false);

        for (FGenericPlatformThread* Producer : Producers)
        {
            Producer->WaitForCompletion();
        }

        Consumer->WaitForCompletion();

        bResult = (GItems->Size() == NumProducers * NumItemsPerProducer) && bResult;

        for (int32 ProducerIndex = 0; ProducerIndex < NumProducers; ++ProducerIndex)
        {
            for (int32 Index = 0; Index < NumItemsPerProducer; ++Index)
            {
                const String Expected = TTypeToString<int64>::ToString(((ProducerIndex + 1) * ProducerOffset) + Index);
                bResult = GItems->Contains(Expected) && bResult;
            }
        }

        GQueue->Clear();

        delete GQueue;
        GQueue = nullptr;

        delete GItems;
        GItems = nullptr;

        delete Consumer;

        for (FGenericPlatformThread* Producer : Producers)
        {
            delete Producer;
        }

        Producers.Clear();
        return bResult;
    }
}

// Single Producer, Multiple Consumers
namespace SPMCTest
{
    constexpr int64 NumItems     = 500;
    constexpr int64 NumConsumers = 6;

    static TQueue<String, EQueueType::SPMC>* GQueue = nullptr;

    // Atomic so the consumers are guaranteed to observe the producer's write
    static AtomicInt32 GIsRunning(1);

    struct FProducerThread : public FRunnable
    {
        int32 Run()
        {
            for (int64 Index = 0; Index < NumItems; ++Index)
            {
                const String NewItem = TTypeToString<int64>::ToString(Index);
                GQueue->Enqueue(::Move(NewItem));
            }

            GIsRunning.Store(0);
            return 0;
        }

        void Destroy()
        {
            delete this;
        }
    };

    struct FConsumerThread : public FRunnable
    {
        int32 Run()
        {
            while (GIsRunning.Load() > 0 || !GQueue->IsEmpty())
            {
                String NewItem;
                if (GQueue->Dequeue(NewItem))
                {
                    Items.Add(::Move(NewItem));
                }
            }

            return 0;
        }

        TArray<String> Items;
    };

    static bool Test()
    {
        bool bResult = true;

        GIsRunning.Store(1);
        GQueue = new TQueue<String, EQueueType::SPMC>;

        FGenericPlatformThread* Producer = FPlatformThread::Create(new FProducerThread, "ProducerThread", false);

        TArray<TUniquePtr<FConsumerThread>> ConsumerInterfaces;
        TArray<FGenericPlatformThread*> Consumers;
        for (int32 i = 0; i < NumConsumers; ++i)
        {
            TUniquePtr<FConsumerThread>& Interface = ConsumerInterfaces.Add(MakeUniquePtr<FConsumerThread>());
            Consumers.Add(FPlatformThread::Create(Interface.Get(), "ConsumerThread", false));
        }

        Producer->WaitForCompletion();

        TArray<String> TotalItems;
        for (FGenericPlatformThread* Consumer : Consumers)
        {
            Consumer->WaitForCompletion();
            TotalItems.Append(static_cast<FConsumerThread*>(Consumer->GetRunnable())->Items);
        }

        bResult = (TotalItems.IsEmpty() == false) && bResult;

        for (int32 Index = 0; Index < NumItems; ++Index)
        {
            const String Expected = TTypeToString<int64>::ToString(Index);
            bResult = TotalItems.Contains(Expected) && bResult;
        }

        GQueue->Clear();

        delete GQueue;
        GQueue = nullptr;

        for (FGenericPlatformThread* Consumer : Consumers)
        {
            delete Consumer;
        }

        delete Producer;
        return bResult;
    }
}

bool TQueue_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Empty queue / IsEmpty / Size / Peek");
    {
        TQueue<int32> Queue;
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);
        TEST_EXPECT(Queue.Peek() == nullptr);

        int32 Out = -1;
        TEST_EXPECT(!Queue.Peek(Out));
        TEST_EXPECT(!Queue.Dequeue(Out));
    }

    TEST_SECTION("Enqueue / Size / Peek (FIFO order)");
    {
        TQueue<int32> Queue;
        TEST_EXPECT(Queue.Enqueue(1));
        TEST_EXPECT(Queue.Enqueue(2));
        TEST_EXPECT(Queue.Enqueue(3));
        TEST_EXPECT(!Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 3);

        int32 Front = -1;
        TEST_EXPECT(Queue.Peek(Front));
        TEST_EXPECT_EQ(Front, 1);
        TEST_EXPECT(Queue.Peek() != nullptr);
        TEST_EXPECT_EQ(*Queue.Peek(), 1);
        TEST_EXPECT_EQ(Queue.Size(), 3);
    }

    TEST_SECTION("Dequeue preserves FIFO order");
    {
        TQueue<int32> Queue;
        Queue.Enqueue(10);
        Queue.Enqueue(20);
        Queue.Enqueue(30);

        int32 Value = 0;
        TEST_EXPECT(Queue.Dequeue(Value));
        TEST_EXPECT_EQ(Value, 10);
        TEST_EXPECT(Queue.Dequeue(Value));
        TEST_EXPECT_EQ(Value, 20);
        TEST_EXPECT_EQ(Queue.Size(), 1);

        TEST_EXPECT(Queue.Dequeue());
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT(!Queue.Dequeue(Value));
    }

    TEST_SECTION("Enqueue move overload");
    {
        TQueue<String> Queue;
        String Item = "Hello";
        TEST_EXPECT(Queue.Enqueue(::Move(Item)));
        TEST_EXPECT_EQ(Queue.Size(), 1);

        String Out;
        TEST_EXPECT(Queue.Dequeue(Out));
        TEST_EXPECT(Out.Equals("Hello"));
    }

    TEST_SECTION("Clear");
    {
        TQueue<int32> Queue;
        Queue.Enqueue(1);
        Queue.Enqueue(2);
        Queue.Enqueue(3);
        Queue.Clear();

        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);
    }

    TEST_SECTION("DequeueAll");
    {
        TQueue<int32> Queue;
        Queue.Enqueue(5);
        Queue.Enqueue(6);
        Queue.Enqueue(7);

        TArray<int32> Out;
        Queue.DequeueAll(Out);
        
        TEST_EXPECT_EQ(Out.Size(), 3);
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT(Out.Contains(5));
        TEST_EXPECT(Out.Contains(6));
        TEST_EXPECT(Out.Contains(7));
    }

    TEST_SECTION("Concurrency: SPSC / MPSC / SPMC");
    {
        TEST_EXPECT(SPSCTest::Test());
        TEST_EXPECT(MPSCTest::Test());
        TEST_EXPECT(SPMCTest::Test());
    }

    TEST_SECTION("TQueue enqueue/dequeue stress (FInstanced, FIFO + no leaks)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            TQueue<FInstanced> Queue;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                Queue.Enqueue(FInstanced(Step));
            }

            TEST_EXPECT_EQ(Queue.Size(), TargetSize);
            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            bool bOrdered = true;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                FInstanced Out;
                const bool bPopped = Queue.Dequeue(Out);
                bOrdered = bOrdered && bPopped && (Out.GetId() == Step) && Out.IsPayloadValid();
            }

            if (!bOrdered)
            {
                LOG_ERROR("[STRESS FAIL] TQueue seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bOrdered);
            TEST_EXPECT(Queue.IsEmpty());
            TEST_EXPECT(FInstanced::LiveCount() == 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
