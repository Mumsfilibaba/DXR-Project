#include "NewOperatorsTests.h"

#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestMacros.h"

#include <new>

static thread_local volatile int32 GNumMallocs  = 0;
static thread_local volatile int32 GNumReallocs = 0;
static thread_local volatile int32 GNumFrees    = 0;

static void* volatile GSink = nullptr;

struct FCountingMalloc : public FMalloc
{
    explicit FCountingMalloc(FMalloc* InBaseMalloc)
        : BaseMalloc(InBaseMalloc)
    {
    }

    virtual void* Malloc(uint64 InSize) override final
    {
        GNumMallocs = GNumMallocs + 1;
        return BaseMalloc->Malloc(InSize);
    }

    virtual void* Realloc(void* InBlock, uint64 InSize) override final
    {
        GNumReallocs = GNumReallocs + 1;
        return BaseMalloc->Realloc(InBlock, InSize);
    }

    virtual void Free(void* InBlock) override final
    {
        GNumFrees = GNumFrees + 1;
        BaseMalloc->Free(InBlock);
    }

    FMalloc* BaseMalloc;
};

struct FScopedCountingMalloc
{
    FScopedCountingMalloc()
        : CountingMalloc(GMalloc)
        , PreviousMalloc(GMalloc)
    {
        GNumMallocs  = 0;
        GNumReallocs = 0;
        GNumFrees    = 0;
        GMalloc = &CountingMalloc;
    }

    ~FScopedCountingMalloc()
    {
        GMalloc = PreviousMalloc;
    }

    FCountingMalloc CountingMalloc;
    FMalloc*        PreviousMalloc;
};

bool NewOperators_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A new-expression allocates through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Value = new int32(42);
        GSink = Value;
        TEST_EXPECT_EQ(GNumMallocs, 1);
        TEST_EXPECT_EQ(*Value, 42);

        delete Value;
        TEST_EXPECT_EQ(GNumFrees, 1);
    }

    TEST_SECTION("An array new-expression allocates through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Values = new int32[16];
        GSink = Values;
        TEST_EXPECT_EQ(GNumMallocs, 1);

        delete[] Values;
        TEST_EXPECT_EQ(GNumFrees, 1);
    }

    TEST_SECTION("The nothrow and sized forms route through GMalloc");
    {
        FScopedCountingMalloc CountingScope;

        int32* Value = new (std::nothrow) int32(7);
        GSink = Value;
        TEST_EXPECT(Value != nullptr);
        TEST_EXPECT_EQ(GNumMallocs, 1);

        ::operator delete(Value, sizeof(int32));
        TEST_EXPECT_EQ(GNumFrees, 1);

        void* Array = ::operator new[](64, std::nothrow);
        GSink = Array;
        TEST_EXPECT(Array != nullptr);
        TEST_EXPECT_EQ(GNumMallocs, 2);

        ::operator delete[](Array, std::nothrow);
        TEST_EXPECT_EQ(GNumFrees, 2);
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

    GSink = nullptr;
    TEST_END();
}
