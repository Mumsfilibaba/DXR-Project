#include "Optional_Test.h"

#if RUN_TOPIONAL_TEST
#include "TestUtils.h"

#include <Core/Containers/Optional.h>
#include <Core/Memory/Memory.h>

namespace
{
    // Non-trivial, heap-owning type to exercise Optional with real construction/copy/move/destruction.
    struct FHeavy
    {
        enum { SizeInBytes = 256 };

        FHeavy()
        {
            Pointer = Memory::Malloc(SizeInBytes);
        }

        FHeavy(int32 InValue)
            : Value(InValue)
        {
            Pointer = Memory::Malloc(SizeInBytes);
            Memory::Memset(Pointer, static_cast<uint8>(Value), SizeInBytes);
        }

        FHeavy(const FHeavy& Other)
            : Value(Other.Value)
        {
            Pointer = Memory::Malloc(SizeInBytes);
            Memory::Memcpy(Pointer, Other.Pointer, SizeInBytes);
        }

        FHeavy(FHeavy&& Other)
            : Pointer(Other.Pointer)
            , Value(Other.Value)
        {
            Other.Pointer = nullptr;
            Other.Value   = 0;
        }

        ~FHeavy()
        {
            Memory::Free(Pointer);
            Pointer = nullptr;
            Value   = 0;
        }

        FHeavy& operator=(const FHeavy& RHS)
        {
            Memory::Free(Pointer);
            Pointer = Memory::Malloc(SizeInBytes);
            Memory::Memcpy(Pointer, RHS.Pointer, SizeInBytes);
            Value = RHS.Value;
            return *this;
        }

        FHeavy& operator=(FHeavy&& RHS)
        {
            Memory::Free(Pointer);
            Pointer     = RHS.Pointer;
            Value       = RHS.Value;
            RHS.Pointer = nullptr;
            RHS.Value   = 0;
            return *this;
        }

        bool operator==(int32 RHS) const noexcept
        {
            return Value == RHS;
        }

        void* Pointer = nullptr;
        int32 Value   = 0;
    };

    // Move-only type to ensure Optional supports non-copyable payloads.
    struct FMoveOnly
    {
        FMoveOnly() = default;
        ~FMoveOnly() = default;
        FMoveOnly(FMoveOnly&&) = default;
        FMoveOnly& operator=(FMoveOnly&&) = default;
        FMoveOnly(const FMoveOnly&) = delete;
        FMoveOnly& operator=(const FMoveOnly&) = delete;
    };
}

bool TOptional_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Empty optional / HasValue / operator bool / TryGetValue");
    {
        TOptional<int32> Empty;
        TEST_EXPECT(!Empty.HasValue());
        TEST_EXPECT(!static_cast<bool>(Empty));
        TEST_EXPECT(Empty.TryGetValue() == nullptr);
        TEST_EXPECT_EQ(Empty.GetValueOrDefault(7), 7);
    }

    TEST_SECTION("InPlace construction / GetValue / operator* / operator->");
    {
        TOptional<int32> Value(EInPlace::InPlace, 42);
        TEST_EXPECT(Value.HasValue());
        TEST_EXPECT(static_cast<bool>(Value));
        TEST_EXPECT_EQ(Value.GetValue(), 42);
        TEST_EXPECT_EQ(*Value, 42);
        TEST_EXPECT(Value.TryGetValue() != nullptr);
        TEST_EXPECT_EQ(*Value.TryGetValue(), 42);
        TEST_EXPECT_EQ(Value.GetValueOrDefault(7), 42);
    }

    TEST_SECTION("Emplace / Reset");
    {
        TOptional<int32> Value;
        TEST_EXPECT_EQ(Value.Emplace(100), 100);
        TEST_EXPECT(Value.HasValue());
        TEST_EXPECT_EQ(*Value, 100);

        Value.Emplace(200);
        TEST_EXPECT_EQ(*Value, 200);

        Value.Reset();
        TEST_EXPECT(!Value.HasValue());
    }

    TEST_SECTION("Copy / move / nullptr assignment");
    {
        TOptional<int32> Value(EInPlace::InPlace, 5);
        TOptional<int32> Copy;
        Copy = Value;

        TEST_EXPECT(Copy.HasValue());
        TEST_EXPECT_EQ(*Copy, 5);

        TOptional<int32> Moved;
        Moved = ::Move(Copy);
        
        TEST_EXPECT(Moved.HasValue());
        TEST_EXPECT_EQ(*Moved, 5);

        Moved = nullptr;
        TEST_EXPECT(!Moved.HasValue());
    }

    TEST_SECTION("Swap (both set, one set, neither set)");
    {
        TOptional<int32> First(EInPlace::InPlace, 1);
        TOptional<int32> Second(EInPlace::InPlace, 2);
        First.Swap(Second);
        
        TEST_EXPECT_EQ(*First, 2);
        TEST_EXPECT_EQ(*Second, 1);

        TOptional<int32> Set(EInPlace::InPlace, 9);
        TOptional<int32> Unset;
        Set.Swap(Unset);

        TEST_EXPECT(!Set.HasValue());
        TEST_EXPECT(Unset.HasValue());
        TEST_EXPECT_EQ(*Unset, 9);
    }

    TEST_SECTION("operator== / operator!=");
    {
        TOptional<int32> First(EInPlace::InPlace, 5);
        TOptional<int32> Second(EInPlace::InPlace, 5);
        TOptional<int32> Third(EInPlace::InPlace, 6);
        TOptional<int32> Empty1;
        TOptional<int32> Empty2;

        TEST_EXPECT(First == Second);
        TEST_EXPECT(First != Third);
        TEST_EXPECT(Empty1 == Empty2);
        TEST_EXPECT(First != Empty1);
    }

    TEST_SECTION("Non-trivial heap type: InPlace / Emplace / Swap / GetValueOrDefault");
    {
        TOptional<FHeavy> First;
        TOptional<FHeavy> Second(EInPlace::InPlace, 100);
        TEST_EXPECT(!First.HasValue());
        TEST_EXPECT(Second.HasValue());
        TEST_EXPECT(*Second == 100);

        TEST_EXPECT(First.Emplace(255) == 255);
        TEST_EXPECT(First.HasValue());

        First.Swap(Second);
        TEST_EXPECT(*First == 100);
        TEST_EXPECT(*Second == 255);

        First.Reset();
        TEST_EXPECT(First.GetValueOrDefault(FHeavy(50)) == 50);
    }

    TEST_SECTION("Move-only payload: Emplace / move construction");
    {
        TOptional<FMoveOnly> First;
        First.Emplace();
        
        TEST_EXPECT(First.HasValue());

        TOptional<FMoveOnly> Second(::Move(First));
        TEST_EXPECT(Second.HasValue());
    }

    TEST_END();
}
#endif
