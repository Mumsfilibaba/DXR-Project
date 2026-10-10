#include "ThreadLocalTests.h"

#include <Core/Containers/Array.h>
#include <Core/Platform/CriticalSection.h>
#include <Core/Platform/PlatformThread.h>
#include <Core/Platform/PlatformTLS.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Threading/Atomic/AtomicInt.h>
#include <Core/Threading/Runnable.h>
#include <Core/Threading/ScopedLock.h>
#include <Core/Threading/ThreadLocal.h>

#include "TestCommon/TestMacros.h"

static CONST_INIT AtomicInt32 GNumConstructed;
static CONST_INIT AtomicInt32 GNumDestroyed;

struct FTrackedValue
{
    FTrackedValue()
    {
        GNumConstructed.Increment();
    }

    ~FTrackedValue()
    {
        GNumDestroyed.Increment();
    }

    int32 Value = 0;
};

static CONST_INIT TThreadLocal<FTrackedValue> GTrackedValue;
static CONST_INIT FThreadLocalSlot GRawSlot;

struct FThreadLocalRunnable : public FRunnable
{
    explicit FThreadLocalRunnable(int32 InValue)
        : Value(InValue)
    {
    }

    virtual int32 Run() override
    {
        FTrackedValue& Tracked = *GTrackedValue;
        bSawFreshValue = (Tracked.Value == 0);
        Tracked.Value = Value;
        bKeptValue = (&GTrackedValue.Get() == &Tracked) && (GTrackedValue->Value == Value);

        bSawEmptyRawSlot = (GRawSlot.GetValue() == nullptr);
        GRawSlot.SetValue(reinterpret_cast<void*>(static_cast<uintptr_t>(Value)));
        bKeptRawSlot = (GRawSlot.GetValue() == reinterpret_cast<void*>(static_cast<uintptr_t>(Value)));
        return 0;
    }

    int32 Value;
    bool  bSawFreshValue   = false;
    bool  bKeptValue       = false;
    bool  bSawEmptyRawSlot = false;
    bool  bKeptRawSlot     = false;
};

struct FThreadInstance
{
    uint32         ThreadID;
    FTrackedValue* Instance;
};

bool ThreadLocal_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A slot reads nullptr and an instance is absent until first use");
    {
        static CONST_INIT FThreadLocalSlot UnusedSlot;
        static CONST_INIT TThreadLocal<FTrackedValue> UnusedValue;

        TEST_EXPECT(UnusedSlot.GetValue() == nullptr);
        TEST_EXPECT(UnusedSlot.GetIndex() != CORE_INVALID_TLS_INDEX);
        TEST_EXPECT(UnusedValue.TryGet() == nullptr);
    }

    TEST_SECTION("Each thread sees its own value");
    {
        GTrackedValue.Get().Value = 7;
        GRawSlot.SetValue(reinterpret_cast<void*>(uintptr_t(7)));

        constexpr int32 NumThreads = 4;
        FThreadLocalRunnable* Runnables[NumThreads];
        IPlatformThread*      Threads[NumThreads];

        const int32 NumDestroyedBefore = GNumDestroyed.Load();
        for (int32 Index = 0; Index < NumThreads; ++Index)
        {
            Runnables[Index] = new FThreadLocalRunnable(100 + Index);
            Threads[Index]   = FPlatformThread::Create(Runnables[Index], "ThreadLocalTest", false);
        }

        for (int32 Index = 0; Index < NumThreads; ++Index)
        {
            Threads[Index]->WaitForCompletion();

            TEST_EXPECT(Runnables[Index]->bSawFreshValue);
            TEST_EXPECT(Runnables[Index]->bKeptValue);
            TEST_EXPECT(Runnables[Index]->bSawEmptyRawSlot);
            TEST_EXPECT(Runnables[Index]->bKeptRawSlot);

            delete Threads[Index];
            delete Runnables[Index];
        }

        TEST_SECTION("The destructor runs when a thread exits");
        TEST_EXPECT_EQ(GNumDestroyed.Load() - NumDestroyedBefore, NumThreads);

        TEST_SECTION("Other threads leave the calling thread's value alone");
        TEST_EXPECT_EQ(GTrackedValue.Get().Value, 7);
        TEST_EXPECT(GRawSlot.GetValue() == reinterpret_cast<void*>(uintptr_t(7)));
    }

    TEST_SECTION("A worker keeps one instance across task graph work");
    {
        TArray<FThreadInstance> Seen;
        FCriticalSection        SeenLock;

        Tasks::ParallelFor(512, [&](int32)
        {
            FTrackedValue& Tracked = GTrackedValue.Get();
            Tracked.Value++;

            SCOPED_LOCK(SeenLock);
            Seen.Add(FThreadInstance{ FPlatformTLS::GetCurrentThreadID(), &Tracked });
        });

        bool bConsistent = true;
        for (int32 First = 0; First < Seen.Size(); ++First)
        {
            for (int32 Second = First + 1; Second < Seen.Size(); ++Second)
            {
                const bool bSameThread   = Seen[First].ThreadID == Seen[Second].ThreadID;
                const bool bSameInstance = Seen[First].Instance == Seen[Second].Instance;
                bConsistent = bConsistent && (bSameThread == bSameInstance);
            }
        }

        TEST_EXPECT_EQ(Seen.Size(), 512);
        TEST_EXPECT(bConsistent);
    }

    TEST_END();
}
