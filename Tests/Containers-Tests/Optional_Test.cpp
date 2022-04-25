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

    {
        TOptional<STest> Optional0;
        TOptional<STest> Optional1(InPlace, 65);

        if (!Optional0)
        {
            std::cout << "Optional0 does NOT have a value" << '\n';
        }

        if (Optional1)
        {
            std::cout << "Optional1 does has a value" << '\n';
        }

        Optional1.Reset();

        if (!Optional1)
        {
            std::cout << "Optional1 does has NOT a value" << '\n';
        }
    }

    {
        TOptional<SMoveable> Optional0;
        Optional0.Emplace();

        TOptional<SMoveable> Optional�1(Move(Optional0));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Emplace

    {
        TOptional<STest> Optional0(InPlace, 70);
        Optional0.Emplace(245);
        Optional0.Emplace(235);
        Optional0.Emplace(225);
        Optional0.Emplace(215);
        Optional0.Emplace(205);

        TOptional<int32> Optional1;
        std::cout << "Optional1.Emplace(10)" << Optional1.Emplace(10) << '\n';
        std::cout << "Optional1.Emplace(20)" << Optional1.Emplace(20) << '\n';
        std::cout << "Optional1.Emplace(30)" << Optional1.Emplace(30) << '\n';
        std::cout << "Optional1.Emplace(40)" << Optional1.Emplace(40) << '\n';
        std::cout << "Optional1.Emplace(50)" << Optional1.Emplace(50) << '\n';
        std::cout << "Optional1.Emplace(60)" << Optional1.Emplace(60) << '\n';
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap basic

    {
        TOptional<int64> Optional0;
        TOptional<int64> Optional1(InPlace, 100);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';

        Optional0.Emplace(255);

        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        Optional0.Swap(Optional1);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';
        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        Optional0.Reset();

        std::cout << "Optional0.GetValueOrDefault()=" << Optional0.GetValueOrDefault(50) << '\n';
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Swap complex

    {
        TOptional<STest> Optional0;
        TOptional<STest> Optional1(InPlace, 100);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';

        Optional0.Emplace(255);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';


        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        Optional0.Swap(Optional1);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';
        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        Optional0.Swap(Optional1);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';
        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        Optional0.Swap(Optional1);

        std::cout << "Optional0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';
        std::cout << "Optional1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';
    }
    
    {
        TOptional<int32> Optional0;
        TOptional<int32> Optional1(InPlace);
        std::cout << "OptionalInt0.HasValue()=" << std::boolalpha << Optional0.HasValue() << '\n';
        std::cout << "OptionalInt1.HasValue()=" << std::boolalpha << Optional1.HasValue() << '\n';

        std::cout << "OptionalInt0.TryGetValue()=" << Optional0.TryGetValue() << '\n';
        std::cout << "OptionalInt1.TryGetValue()=" << Optional1.TryGetValue() << '\n';
    }



    return;
}

#endif