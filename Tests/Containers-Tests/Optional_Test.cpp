#include "Optional_Test.h"

#if RUN_TOPIONAL_TEST
#include "TestUtils.h"

#include <Core/Containers/Optional.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/ClassUtilities.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTest

struct FTest
{
    enum
    {
        SizeInBytes = 1024*1024
    };

    FTest()
    {
        Pointer = FMemory::Malloc(SizeInBytes);
    }

    FTest(int32 Value)
    {
        Pointer = FMemory::Malloc(SizeInBytes);
        FMemory::Memset(Pointer, static_cast<uint8>(Value), SizeInBytes);

        char* Temp =reinterpret_cast<char*>(Pointer);
        UNREFERENCED_VARIABLE(Temp);
    }
     
    FTest(const FTest& Other)
    {
        Pointer = FMemory::Malloc(SizeInBytes);
        FMemory::Memcpy(Pointer, Other.Pointer, SizeInBytes);
    }

    FTest(FTest&& Other)
        : Pointer(Other.Pointer)
        , Value(Other.Value)
    {
        Other.Pointer = nullptr;
        Other.Value   = 0;
    }

    ~FTest()
    {
        FMemory::Free(Pointer);
        Pointer = nullptr;
        Value   = 0;
    }

    FTest& operator=(const FTest& RHS)
    {
        if (Pointer)
        {
            FMemory::Free(Pointer);
        }

        Pointer = FMemory::Malloc(SizeInBytes);
        FMemory::Memcpy(Pointer, RHS.Pointer, SizeInBytes);
        return *this;
    }

    FTest& operator=(FTest&& RHS)
    {
        if (Pointer)
        {
            FMemory::Free(Pointer);
        }

        Pointer = RHS.Pointer;
        RHS.Pointer = nullptr;
        return *this;
    }

    bool operator==(const FTest& RHS) const
    {
        return FMemory::Memcmp(Pointer, RHS.Pointer, SizeInBytes);
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
// FMoveable

struct FMoveable
{
    FMoveable() = default;
    ~FMoveable() = default;

    FMoveable(FMoveable&&) = default;
    FMoveable& operator=(FMoveable&&) = default;

    FMoveable(const FMoveable&) = delete;
    FMoveable& operator=(const FMoveable&) = delete;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCopyable

struct FCopyable
{
    FCopyable() = default;
    ~FCopyable() = default;

    FCopyable(FCopyable&&) = delete;
    FCopyable& operator=(FCopyable&&) = delete;

    FCopyable(const FCopyable&) = default;
    FCopyable& operator=(const FCopyable&) = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tests

bool TOptional_Test()
{
    std::cout << '\n' << "----------TOptional----------" << '\n' << '\n';

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Constructors copy/move etc

    {
        TOptional<FTest> Optional0;
        TOptional<FTest> Optional1(InPlace, 65);

        CHECK(!Optional0);
        CHECK( Optional1);

        Optional1.Reset();

        CHECK(!Optional1);
    }

    {
        TOptional<FMoveable> Optional0;
        Optional0.Emplace();

        TOptional<FMoveable> Optional1(Move(Optional0));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Emplace

    {
        TOptional<FTest> Optional0(InPlace, 70);
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
        TOptional<FTest> Optional0;
        TOptional<FTest> Optional1(InPlace, 100);
        
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