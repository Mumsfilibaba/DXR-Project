#include "Queue_Test.h"

#if RUN_TQUEUE_TEST
#include "TestUtils.h"

#include <Core/Containers/Queue.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Threading/ThreadInterface.h>
#include <Core/Platform/PlatformThreadMisc.h>

// Single Producer, Single Consumer
namespace SPSCTest
{
    bool Test()
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
            TEST_CHECK(Items[Index] == ExpectedItem);
        }

        SUCCESS();
    }
}

// Multiple Producers, Single Consumer
namespace MPSCTest
{
    TQueue<FString, EQueueType::MPSC>* GQueue = nullptr;
    
    bool GIsRunning = true;

    constexpr int64 NumItemsPerProducer = 500;
    constexpr int64 ProducerOffset      = 1000;
    constexpr int64 NumProducers        = 6;

    struct FProducerThread : public FThreadInterface
    {
        FProducerThread(int64 InThreadIndex)
            : ThreadIndex(InThreadIndex)
        {
        }

        int32 Run()
        {
            // Do some work, otherwise to fast
            for (int64 Index = 0; Index < NumItemsPerProducer; ++Index)
            {
                const FString NewItem = TTypeToString<int64>::ToString(ThreadIndex + Index);
                GQueue->Emplace(::Move(NewItem));
            }

            GIsRunning = false;
            return 0;
        }

        void Destroy()
        {
            delete this;
        }

        int64 ThreadIndex;
    };

    TArray<FString>* GItems = nullptr;
    struct FConsumerThread : public FThreadInterface
    {
        int32 Run()
        {
            while (GIsRunning || !GQueue->IsEmpty())
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

    bool Test()
    {
        // Create the Queue and Array on the heap to avoid wrong reporting of memory leaks
        GQueue = new TQueue<FString, EQueueType::MPSC>;
        GItems = new TArray<FString>;

        TArray<TSharedRef<FGenericThread>> Producers;
        for (int32 i = 0; i < NumProducers; ++i)
        {
            const int32 ProducerIndex = (i + 1) * ProducerOffset;
            Producers.Add(FPlatformThreadMisc::CreateThread(new FProducerThread(ProducerIndex), false));
        }

        TSharedRef<FGenericThread> Consumer = FPlatformThreadMisc::CreateThread(new FConsumerThread, false);

        // Wait for all the thread to finish
        for (TSharedRef<FGenericThread>& Producer : Producers)
        {
            Producer->WaitForCompletion();
        }
        Consumer->WaitForCompletion();

        TEST_CHECK(GItems->Size() == NumProducers * NumItemsPerProducer);

        // Check so the array contains the elements from all producers
        for (int32 ProducerIndex = 0; ProducerIndex < NumProducers; ++ProducerIndex) 
        {
            for (int32 Index = 0; Index < NumItemsPerProducer; ++Index)
            {
                const FString Expected = TTypeToString<int64>::ToString(((ProducerIndex + 1) * ProducerOffset) + Index);
                TEST_CHECK(GItems->Contains(Expected) == true);
            }
        }

        GQueue->Clear();

        // Delete the Queue and Array
        delete GQueue;
        GQueue = nullptr;

        delete GItems;
        GItems = nullptr;

        SUCCESS();
    }
}

// Single Producer, Multiple Consumers
namespace SPMCTest
{
    constexpr int64 NumItems     = 500;
    constexpr int64 NumConsumers = 6;

    TQueue<FString, EQueueType::SPMC>* GQueue = nullptr;

    bool GIsRunning = true;
    
    struct FProducerThread : public FThreadInterface
    {
        int32 Run()
        {
            // Do some work, otherwise to fast
            for (int64 Index = 0; Index < NumItems; ++Index)
            {
                const FString NewItem = TTypeToString<int64>::ToString(Index);
                GQueue->Enqueue(::Move(NewItem));
            }

            GIsRunning = false;
            return 0;
        }

        void Destroy()
        {
            delete this;
        }
    };

    struct FConsumerThread : public FThreadInterface
    {
        int32 Run()
        {
            while (GIsRunning || !GQueue->IsEmpty())
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

    bool Test()
    {
        // Create the Queue on the heap to avoid wrong reporting of memory leaks
        GQueue = new TQueue<FString, EQueueType::SPMC>;

        TSharedRef<FGenericThread> Producer = FPlatformThreadMisc::CreateThread(new FProducerThread, false);
        
        TArray<TUniquePtr<FConsumerThread>> ConsumerInterfaces;
        TArray<TSharedRef<FGenericThread>>  Consumers;
        for (int32 i = 0; i < NumConsumers; ++i)
        {
            TUniquePtr<FConsumerThread>& Interface = ConsumerInterfaces.Add(MakeUnique<FConsumerThread>());
            Consumers.Add(FPlatformThreadMisc::CreateThread(Interface.Get(), false));
        }

        // Wait for all the thread to finish
        Producer->WaitForCompletion();

        TArray<FString> TotalItems;
        for (TSharedRef<FGenericThread>& Consumer : Consumers)
        {
            Consumer->WaitForCompletion();
            TotalItems.Append(static_cast<FConsumerThread*>(Consumer->GetRunnable())->Items);
        }

        TEST_CHECK(TotalItems.IsEmpty() == false);

        // Check so the array contains the elements from Producer1
        for (int32 Index = 0; Index < NumItems; ++Index)
        {
            const FString Expected = TTypeToString<int64>::ToString(Index);
            TEST_CHECK(TotalItems.Contains(Expected) == true);
        }

        GQueue->Clear();

        // Delete the queue
        delete GQueue;
        GQueue = nullptr;

        SUCCESS();
    }
}

namespace UnusedPopulatedQueue
{
    void Test()
    {
        TQueue<FString, EQueueType::SPSC> Queue;
        for (int64 Index = 0; Index < 50; ++Index)
        {
            const FString Item = "Some long string that is longer than the small string optimization" + TTypeToString<int64>::ToString(Index);
            Queue.Enqueue(::Move(Item));
        }
    }
}

void TQueue_Test()
{
    SPSCTest::Test();
    MPSCTest::Test();
    SPMCTest::Test();

    UnusedPopulatedQueue::Test();
}
#endif