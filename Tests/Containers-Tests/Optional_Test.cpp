#include "Optional_Test.h"

#if RUN_TOPIONAL_TEST
#include "TestUtils.h"

#include <Core/Containers/Optional.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/ClassUtilities.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// STest

struct STest
{
    enum
    {
        SizeInBytes = 1024*1024
    };

    STest() noexcept
        : Pointer(nullptr)
        , Value(0)
    {
        Pointer = CMemory::Malloc(SizeInBytes);
    }

    STest(int32 InValue) noexcept
        : Pointer(nullptr)
        , Value(InValue)
    {
        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memset(Pointer, static_cast<uint8>(Value), SizeInBytes);
    }

    STest(const STest& Other) noexcept
        : Pointer(nullptr)
        , Value(Other.Value)
    {
        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memcpy(Pointer, Other.Pointer, SizeInBytes);
    }

    STest(STest&& Other) noexcept
        : Pointer(Other.Pointer)
        , Value(Other.Value)
    {
        Other.Pointer = nullptr;
        Other.Value   = 0;
    }

    ~STest() noexcept
    {
        CMemory::Free(Pointer);
        Pointer = nullptr;
        Value   = 0;
    }

    STest& operator=(const STest& Rhs) noexcept
    {
        if (Pointer)
        {
            CMemory::Free(Pointer);
        }

        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memcpy(Pointer, Rhs.Pointer, SizeInBytes);
        return *this;
    }

    STest& operator=(STest&& Rhs) noexcept
    {
        if (Pointer)
        {
            CMemory::Free(Pointer);
        }

        Pointer = Rhs.Pointer;
        Rhs.Pointer = nullptr;
        return *this;
    }

    bool operator==(const STest& Rhs) const noexcept
    {
        return CMemory::Memcmp(Pointer, Rhs.Pointer, SizeInBytes);
    }

    bool operator==(int32 Rhs) const noexcept
    {
        return Value == Rhs;
    }

    bool operator!=(int32 Rhs) const noexcept
    {
        return !(*this == Rhs);
    }

    void* Pointer = nullptr;
    int32 Value   = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMoveable

struct SMoveable
{
    SMoveable() = default;
    ~SMoveable() = default;

    SMoveable(SMoveable&&) = default;
    SMoveable& operator=(SMoveable&&) = default;

    SMoveable(const SMoveable&) = delete;
    SMoveable& operator=(const SMoveable&) = delete;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCopyable

struct SCopyable
{
    SCopyable() = default;
    ~SCopyable() = default;

    SCopyable(SCopyable&&) = delete;
    SCopyable& operator=(SCopyable&&) = delete;

    SCopyable(const SCopyable&) = default;
    SCopyable& operator=(const SCopyable&) = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tests

bool TOptional_Test()
{
    std::cout << '\n' << "----------TOptional----------" << '\n' << '\n';

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Constructors copy/move etc

    {
        TOptional<STest> Optional0;
        TOptional<STest> Optional1(InPlace, 65);

        CHECK(!Optional0);
        CHECK( Optional1);

        Optional1.Reset();

        CHECK(!Optional1);
    }

    {
        TOptional<SMoveable> Optional0;
        Optional0.Emplace();

        TOptional<SMoveable> Optional1(Move(Optional0));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Emplace

    {
        TOptional<STest> Optional0(InPlace, 70);
        CHECK(*Optional0 == 70);
        CHECK(Optional0.Emplace(245) == 245);
        CHECK(Optional0.Emplace(235) == 235);
        CHECK(Optional0.Emplace(225) == 225);
        CHECK(Optional0.Emplace(215) == 215);
        CHECK(Optional0.Emplace(205) == 205);

        TOptional<int32> Optional1;
        CHECK(Optional1.Emplace(10) == 10);
        CHECK(Optional1.Emplace(20) == 20);
        CHECK(Optional1.Emplace(30) == 30);
        CHECK(Optional1.Emplace(40) == 40);
        CHECK(Optional1.Emplace(50) == 50);
        CHECK(Optional1.Emplace(60) == 60);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap basic

    {
        TOptional<int64> Optional0;
        TOptional<int64> Optional1(InPlace, 100);
        CHECK(*Optional1 == 100);

        CHECK(!Optional0.HasValue());

        CHECK(Optional0.Emplace(255) == 255);

        CHECK(Optional0.HasValue());
        CHECK(Optional1.HasValue());

        Optional0.Swap(Optional1);

        CHECK(*Optional0 == 100);
        CHECK(*Optional1 == 255);

        Optional0.Reset();

        CHECK(Optional0.GetValueOrDefault(50) == 50);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap complex

    {
        TOptional<STest> Optional0;
        TOptional<STest> Optional1(InPlace, 100);
        
        CHECK(!Optional0.HasValue());
        CHECK( Optional1.HasValue());

        CHECK(*Optional1 == 100);

        CHECK(Optional0.Emplace(255) == 255);
        CHECK(Optional0.HasValue());

        Optional0.Swap(Optional1);

        CHECK(*Optional0 == 100);
        CHECK(*Optional1 == 255);
        CHECK(Optional0.HasValue());
        CHECK(Optional1.HasValue());

        Optional0.Swap(Optional1);

        CHECK(*Optional0 == 255);
        CHECK(*Optional1 == 100);
        CHECK(Optional0.HasValue());
        CHECK(Optional1.HasValue());

        Optional0.Swap(Optional1);

        CHECK(*Optional0 == 100);
        CHECK(*Optional1 == 255);
        CHECK(Optional0.HasValue());
        CHECK(Optional1.HasValue());
    }
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TryGetValue

    {
        TOptional<int32> Optional0;
        TOptional<int32> Optional1(InPlace);
        CHECK(!Optional0.HasValue());
        CHECK(Optional1.HasValue());

        CHECK(Optional0.TryGetValue() == nullptr);
        CHECK(Optional1.TryGetValue() != nullptr);
    }

    SUCCESS();
}

#endif