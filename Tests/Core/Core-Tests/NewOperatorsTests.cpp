#include "NewOperatorsTests.h"

#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Threading/ThreadLocal.h>

#include "TestCommon/TestMacros.h"

#include <new>

struct FAllocationCounts
{
    volatile int32  NumMallocs    = 0;
    volatile int32  NumReallocs   = 0;
    volatile int32  NumFrees      = 0;
    volatile uint32 LastAlignment = 0;
};

static CONST_INIT TThreadLocal<FAllocationCounts> GAllocationCounts;

static void* volatile GSink = nullptr;

struct FCountingMalloc : public FMalloc
{
    explicit FCountingMalloc(FMalloc* InBaseMalloc)
        : BaseMalloc(InBaseMalloc)
    {
    }

    virtual void* Malloc(uint64 InSize, uint32 InAlignment) override final
    {
        if (FAllocationCounts* Counts = GAllocationCounts.TryGet())
        {
            Counts->NumMallocs = Counts->NumMallocs + 1;
            Counts->LastAlignment = InAlignment;
        }

        return BaseMalloc->Malloc(InSize, InAlignment);
    }

    virtual void* Realloc(void* InBlock, uint64 InSize, uint32 InAlignment) override final
    {
        if (FAllocationCounts* Counts = GAllocationCounts.TryGet())
        {
            Counts->NumReallocs = Counts->NumReallocs + 1;
            Counts->LastAlignment = InAlignment;
        }

        return BaseMalloc->Realloc(InBlock, InSize, InAlignment);
    }

    virtual void Free(void* InBlock) override final
    {
        if (FAllocationCounts* Counts = GAllocationCounts.TryGet())
        {
            Counts->NumFrees = Counts->NumFrees + 1;
        }

        BaseMalloc->Free(InBlock);
    }

    FMalloc* BaseMalloc;
};

struct alignas(64) FOverAligned
{
    uint8 Bytes[64];
};

static bool IsAligned(const void* Pointer, uint64 Alignment)
{
    return (reinterpret_cast<UPTR_INT>(Pointer) & (Alignment - 1)) == 0;
}

struct FScopedCountingMalloc
{
    FScopedCountingMalloc()
        : Counts(GAllocationCounts.Get())
        , CountingMalloc(GMalloc)
        , PreviousMalloc(GMalloc)
    {
        Counts.NumMallocs    = 0;
        Counts.NumReallocs   = 0;
        Counts.NumFrees      = 0;
        Counts.LastAlignment = 0;
        GMalloc = &CountingMalloc;
    }

    ~FScopedCountingMalloc()
    {
        GMalloc = PreviousMalloc;
    }

    FAllocationCounts& Counts;
    FCountingMalloc    CountingMalloc;
    FMalloc*           PreviousMalloc;
};

bool NewOperators_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A new-expression allocates through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Value = new int32(42);
        GSink = Value;
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 1);
        TEST_EXPECT_EQ(*Value, 42);

        delete Value;
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 1);
    }

    TEST_SECTION("An array new-expression allocates through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Values = new int32[16];
        GSink = Values;
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 1);

        delete[] Values;
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 1);
    }

    TEST_SECTION("The nothrow and sized forms route through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Value = new (std::nothrow) int32(7);
        GSink = Value;
        TEST_EXPECT(Value != nullptr);
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 1);

        ::operator delete(Value, sizeof(int32));
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 1);

        void* Array = ::operator new[](64, std::nothrow);
        GSink = Array;
        TEST_EXPECT(Array != nullptr);
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 2);

        ::operator delete[](Array, std::nothrow);
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 2);
    }

    TEST_SECTION("A zero-sized new returns a unique non-null pointer");
    {
        void* First  = ::operator new(0);
        void* Second = ::operator new(0);
        TEST_EXPECT(First != nullptr);
        TEST_EXPECT(Second != nullptr);
        TEST_EXPECT(First != Second);
        ::operator delete(First);
        ::operator delete(Second);

        void* NothrowFirst  = ::operator new[](0, std::nothrow);
        void* NothrowSecond = ::operator new[](0, std::nothrow);
        TEST_EXPECT(NothrowFirst != nullptr);
        TEST_EXPECT(NothrowSecond != nullptr);
        TEST_EXPECT(NothrowFirst != NothrowSecond);
        ::operator delete[](NothrowFirst);
        ::operator delete[](NothrowSecond);
    }

    TEST_SECTION("An over-aligned new-expression passes its alignment to GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        FOverAligned* Value = new FOverAligned();
        GSink = Value;
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 1);
        TEST_EXPECT_EQ(CountingScope.Counts.LastAlignment, 64u);
        TEST_EXPECT(IsAligned(Value, 64));

        delete Value;
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 1);

        FOverAligned* Values = new FOverAligned[3];
        GSink = Values;
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 2);
        TEST_EXPECT(IsAligned(Values, 64));
        TEST_EXPECT(IsAligned(&Values[2], 64));

        delete[] Values;
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 2);
    }

    TEST_SECTION("The aligned nothrow and sized forms route through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        FOverAligned* Value = new (std::nothrow) FOverAligned();
        GSink = Value;
        TEST_EXPECT(Value != nullptr);
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 1);
        TEST_EXPECT(IsAligned(Value, 64));

        ::operator delete(Value, sizeof(FOverAligned), std::align_val_t(alignof(FOverAligned)));
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 1);

        void* PageAligned = ::operator new[](100, std::align_val_t(4096), std::nothrow);
        GSink = PageAligned;
        TEST_EXPECT(PageAligned != nullptr);
        TEST_EXPECT_EQ(CountingScope.Counts.NumMallocs, 2);
        TEST_EXPECT_EQ(CountingScope.Counts.LastAlignment, 4096u);
        TEST_EXPECT(IsAligned(PageAligned, 4096));

        ::operator delete[](PageAligned, std::align_val_t(4096), std::nothrow);
        TEST_EXPECT_EQ(CountingScope.Counts.NumFrees, 2);
    }

    GSink = nullptr;
    TEST_END();
}
