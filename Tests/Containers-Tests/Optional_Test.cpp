#include "Optional_Test.h"

#if RUN_TOPIONAL_TEST

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

    STest()
    {
        Pointer = CMemory::Malloc(SizeInBytes);
    }

    STest(int32 Value)
    {
        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memset(Pointer, static_cast<uint8>(Value), SizeInBytes);

        char* Temp =reinterpret_cast<char*>(Pointer);
        UNREFERENCED_VARIABLE(Temp);
    }

    STest(const STest& Other)
    {
        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memcpy(Pointer, Other.Pointer, SizeInBytes);
    }

    STest(STest&& Other)
        : Pointer(Other.Pointer)
    {
        Other.Pointer = nullptr;
    }

    ~STest()
    {
        CMemory::Free(Pointer);
        Pointer = nullptr;
    }

    STest& operator=(const STest& RHS)
    {
        if (Pointer)
        {
            CMemory::Free(Pointer);
        }

        Pointer = CMemory::Malloc(SizeInBytes);
        CMemory::Memcpy(Pointer, RHS.Pointer, SizeInBytes);
        return *this;
    }

    STest& operator=(STest&& RHS)
    {
        if (Pointer)
        {
            CMemory::Free(Pointer);
        }

        Pointer = RHS.Pointer;
        RHS.Pointer = nullptr;
        return *this;
    }

    bool operator==(const STest& RHS) const
    {
        return CMemory::Memcmp(Pointer, RHS.Pointer, SizeInBytes);
    }

    void* Pointer = nullptr;
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

void TOptional_Test()
{
    std::cout << '\n' << "----------TOptional----------" << '\n' << '\n';

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Constructors copy/move etc

    TOptional<STest> Optional0;
    if (!Optional0)
    {
        std::cout << "Optional0 does NOT have a value" << '\n';
    }

    TOptional<STest> Optional1(65);
    if (Optional1)
    {
        std::cout << "Optional1 does has a value" << '\n';
    }

    Optional1.Reset();

    if (!Optional1)
    {
        std::cout << "Optional1 does has NOT a value" << '\n';
    }

    TOptional<SMoveable> Optional2;
    Optional2.Emplace();

    TOptional<SMoveable> Optional3(Move(Optional2));

    TOptional<STest> Optional4(70);
    if (Optional4)
    {
        std::cout << "Optional4 does has a value" << '\n';
    }

    Optional4.Emplace(255);
    Optional4.Emplace(245);
    Optional4.Emplace(235);
    Optional4.Emplace(225);
    Optional4.Emplace(215);
    Optional4.Emplace(205);

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap basic

    TOptional<int64> Optional5;

    std::cout << "Optional5.HasValue()=" << std::boolalpha << Optional5.HasValue() << '\n';

    Optional5.Emplace(255);

    TOptional<int64> Optional6(100);

    std::cout << "Optional6.HasValue()=" << std::boolalpha << Optional6.HasValue() << '\n';

    Optional5.Swap(Optional6);

    std::cout << "Optional5.HasValue()=" << std::boolalpha << Optional5.HasValue() << '\n';
    std::cout << "Optional6.HasValue()=" << std::boolalpha << Optional6.HasValue() << '\n';

    Optional5.Reset();

    std::cout << "Optional5.GetValueOrDefault()=" << Optional5.GetValueOrDefault(50) << '\n';

    // Swap complex

    TOptional<STest> Optional7;

    std::cout << "Optional7.HasValue()=" << std::boolalpha << Optional7.HasValue() << '\n';

    Optional7.Emplace(255);

    std::cout << "Optional7.HasValue()=" << std::boolalpha << Optional7.HasValue() << '\n';

    TOptional<STest> Optional8(100);

    std::cout << "Optional8.HasValue()=" << std::boolalpha << Optional8.HasValue() << '\n';

    Optional7.Swap(Optional8);

    std::cout << "Optional7.HasValue()=" << std::boolalpha << Optional7.HasValue() << '\n';
    std::cout << "Optional8.HasValue()=" << std::boolalpha << Optional8.HasValue() << '\n';

    Optional7.Swap(Optional8);

    std::cout << "Optional7.HasValue()=" << std::boolalpha << Optional7.HasValue() << '\n';
    std::cout << "Optional8.HasValue()=" << std::boolalpha << Optional8.HasValue() << '\n';

    Optional7.Swap(Optional8);

    std::cout << "Optional7.HasValue()=" << std::boolalpha << Optional7.HasValue() << '\n';
    std::cout << "Optional8.HasValue()=" << std::boolalpha << Optional8.HasValue() << '\n';

    return;
}

#endif