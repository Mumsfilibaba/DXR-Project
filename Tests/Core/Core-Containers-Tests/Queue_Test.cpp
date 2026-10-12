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
        TQueue<FString, EQueueType::SPSC> Queue;
        for (int64 Index = 1; Index <= 50; ++Index)
        {
            const FString Item = TTypeToString<int64>::ToString(Index);
            Queue.Enqueue(::Move(Item));
        }

        TArray<FString> Items;
        while (Items.Size() < 50)
        {
            FString NewItem;
            if (Queue.Dequeue(NewItem))
            {
                Items.Add(::Move(NewItem));
            }
        }

        for (int32 Index = 0; Index < 50; ++Index)
        {
            const FString ExpectedItem = TTypeToString<int64>::ToString(Index + 1);
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
    static TQueue<FString, EQueueType::MPSC>* GQueue = nullptr;
    static FAtomicInt32                       GActiveProducers(0);

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
                const FString NewItem = TTypeToString<int64>::ToString(ThreadIndex + Index);
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

    static TArray<FString>* GItems = nullptr;

    struct FConsumerThread : public FRunnable
    {
        int32 Run()
        {
            while (GActiveProducers.Load() > 0 || !GQueue->IsEmpty())
            {
                FString NewItem;
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
        GQueue = new TQueue<FString, EQueueType::MPSC>;
        GItems = new TArray<FString>;

        TArray<IPlatformThread*> Producers;
        for (int32 i = 0; i < NumProducers; ++i)
        {
            const int32 ProducerIndex = (i + 1) * ProducerOffset;
            Producers.Add(FPlatformThread::Create(new FProducerThread(ProducerIndex), TEXT("ProducerThread"), false));
        }

        IPlatformThread* Consumer = FPlatformThread::Create(new FConsumerThread, TEXT("ConsumerThread"), false);

        for (IPlatformThread* Producer : Producers)
        {
            Producer->WaitForCompletion();
        }

        Consumer->WaitForCompletion();

        bResult = (GItems->Size() == NumProducers * NumItemsPerProducer) && bResult;

        for (int32 ProducerIndex = 0; ProducerIndex < NumProducers; ++ProducerIndex)
        {
            for (int32 Index = 0; Index < NumItemsPerProducer; ++Index)
            {
                const FString Expected = TTypeToString<int64>::ToString(((ProducerIndex + 1) * ProducerOffset) + Index);
                bResult = GItems->Contains(Expected) && bResult;
            }
        }

        GQueue->Clear();

        delete GQueue;
        GQueue = nullptr;

        delete GItems;
        GItems = nullptr;

        delete Consumer;

        for (IPlatformThread* Producer : Producers)
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

    static TQueue<FString, EQueueType::SPMC>* GQueue = nullptr;

    // Atomic so the consumers are guaranteed to observe the producer's write
    static FAtomicInt32 GIsRunning(1);

    struct FProducerThread : public FRunnable
    {
        int32 Run()
        {
            for (int64 Index = 0; Index < NumItems; ++Index)
            {
                const FString NewItem = TTypeToString<int64>::ToString(Index);
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
                FString NewItem;
                if (GQueue->Dequeue(NewItem))
                {
                    Items.Add(::Move(NewItem));
                }
            }

            return 0;
        }

        TArray<FString> Items;
    };

    static bool Test()
    {
        bool bResult = true;

        GIsRunning.Store(1);
        GQueue = new TQueue<FString, EQueueType::SPMC>;

        IPlatformThread* Producer = FPlatformThread::Create(new FProducerThread, TEXT("ProducerThread"), false);

        TArray<TUniquePtr<FConsumerThread>> ConsumerInterfaces;
        TArray<IPlatformThread*> Consumers;
        for (int32 i = 0; i < NumConsumers; ++i)
        {
            TUniquePtr<FConsumerThread>& Interface = ConsumerInterfaces.Add(MakeUniquePtr<FConsumerThread>());
            Consumers.Add(FPlatformThread::Create(Interface.Get(), TEXT("ConsumerThread"), false));
        }

        Producer->WaitForCompletion();

        TArray<FString> TotalItems;
        for (IPlatformThread* Consumer : Consumers)
        {
            Consumer->WaitForCompletion();
            TotalItems.Append(static_cast<FConsumerThread*>(Consumer->GetRunnable())->Items);
        }

        bResult = (TotalItems.Size() == NumItems) && bResult;

        for (int32 Index = 0; Index < NumItems; ++Index)
        {
            const FString Expected = TTypeToString<int64>::ToString(Index);
            bResult = TotalItems.Contains(Expected) && bResult;
        }

        GQueue->Clear();

        delete GQueue;
        GQueue = nullptr;

        for (IPlatformThread* Consumer : Consumers)
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

    TEST_SECTION(TEXT("Empty queue / IsEmpty / Size / Peek"));
    {
        TQueue<int32> Queue;
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);
        TEST_EXPECT(Queue.Peek() == nullptr);

        int32 Out = -1;
        TEST_EXPECT(!Queue.Peek(Out));
        TEST_EXPECT(!Queue.Dequeue(Out));
    }

    TEST_SECTION(TEXT("Enqueue / Size / Peek (FIFO order)"));
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

    TEST_SECTION(TEXT("Dequeue preserves FIFO order"));
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

    TEST_SECTION(TEXT("Enqueue move overload"));
    {
        TQueue<FString> Queue;
        FString Item = TEXT("Hello");
        TEST_EXPECT(Queue.Enqueue(::Move(Item)));
        TEST_EXPECT_EQ(Queue.Size(), 1);

        FString Out;
        TEST_EXPECT(Queue.Dequeue(Out));
        TEST_EXPECT(Out.Equals(TEXT("Hello")));
    }

    TEST_SECTION(TEXT("Clear"));
    {
        TQueue<int32> Queue;
        Queue.Enqueue(1);
        Queue.Enqueue(2);
        Queue.Enqueue(3);
        Queue.Clear();

        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);
    }

    TEST_SECTION(TEXT("DequeueAll"));
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

    TEST_SECTION(TEXT("SPMC single consumer: FIFO / Clear / DequeueAll"));
    {
        TQueue<int32, EQueueType::SPMC> Queue;
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);

        int32 Value = 0;
        TEST_EXPECT(!Queue.Dequeue(Value));

        TEST_EXPECT(Queue.Enqueue(10));
        TEST_EXPECT(Queue.Enqueue(20));
        TEST_EXPECT(Queue.Enqueue(30));
        TEST_EXPECT_EQ(Queue.Size(), 3);

        TEST_EXPECT(Queue.Dequeue(Value));
        TEST_EXPECT_EQ(Value, 10);
        TEST_EXPECT(Queue.Dequeue(Value));
        TEST_EXPECT_EQ(Value, 20);
        TEST_EXPECT_EQ(Queue.Size(), 1);

        TEST_EXPECT(Queue.Dequeue());
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT(!Queue.Dequeue(Value));

        Queue.Enqueue(1);
        Queue.Enqueue(2);
        Queue.Clear();
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Queue.Size(), 0);

        Queue.Enqueue(5);
        Queue.Enqueue(6);
        Queue.Enqueue(7);

        TArray<int32> Out;
        Queue.DequeueAll(Out);

        TEST_EXPECT_EQ(Out.Size(), 3);
        TEST_EXPECT(Queue.IsEmpty());
        TEST_EXPECT_EQ(Out[0], 5);
        TEST_EXPECT_EQ(Out[1], 6);
        TEST_EXPECT_EQ(Out[2], 7);
    }

    TEST_SECTION(TEXT("Concurrency: SPSC / MPSC / SPMC"));
    {
        constexpr int32 NumConcurrencyIterations = 8;
        for (int32 Iteration = 0; Iteration < NumConcurrencyIterations; ++Iteration)
        {
            TEST_EXPECT(SPSCTest::Test());
            TEST_EXPECT(MPSCTest::Test());
            TEST_EXPECT(SPMCTest::Test());
        }
    }

    TEST_SECTION(TEXT("TQueue enqueue/dequeue stress (FInstanced, FIFO + no leaks)"));
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

    TEST_SECTION(TEXT("TQueue SPMC enqueue/dequeue stress (FInstanced, FIFO + no leaks)"));
    {
        FInstanced::Reset();

        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            TQueue<FInstanced, EQueueType::SPMC> Queue;
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
                LOG_ERROR("[STRESS FAIL] TQueue SPMC seed=%u size=%d", Seed, TargetSize);
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
