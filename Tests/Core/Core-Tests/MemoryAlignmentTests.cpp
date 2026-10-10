#include "MemoryAlignmentTests.h"

#include <Core/Memory/Memory.h>

#include "TestCommon/TestMacros.h"

static bool IsAligned(const void* Pointer, uint64 Alignment)
{
    return (reinterpret_cast<UPTR_INT>(Pointer) & (Alignment - 1)) == 0;
}

static void FillPattern(void* Block, uint64 Size)
{
    uint8* Bytes = reinterpret_cast<uint8*>(Block);
    for (uint64 Index = 0; Index < Size; ++Index)
    {
        Bytes[Index] = static_cast<uint8>(Index * 7 + 3);
    }
}

static bool HasPattern(const void* Block, uint64 Size)
{
    const uint8* Bytes = reinterpret_cast<const uint8*>(Block);
    for (uint64 Index = 0; Index < Size; ++Index)
    {
        if (Bytes[Index] != static_cast<uint8>(Index * 7 + 3))
        {
            return false;
        }
    }

    return true;
}

bool MemoryAlignment_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Malloc honours every power-of-two alignment up to a page");
    {
        for (uint32 Alignment = 1; Alignment <= 4096; Alignment *= 2)
        {
            void* Block = Memory::Malloc(24, Alignment);
            TEST_EXPECT(Block != nullptr);
            TEST_EXPECT(IsAligned(Block, Alignment > STANDARD_ALIGNMENT ? Alignment : STANDARD_ALIGNMENT));

            FillPattern(Block, 24);
            TEST_EXPECT(HasPattern(Block, 24));
            Memory::Free(Block);
        }
    }

    TEST_SECTION("A zero-sized Malloc returns nullptr at any alignment");
    {
        TEST_EXPECT(Memory::Malloc(0) == nullptr);
        TEST_EXPECT(Memory::Malloc(0, 256) == nullptr);
    }

    TEST_SECTION("Realloc keeps the alignment and the contents while growing and shrinking");
    {
        void* Block = Memory::Malloc(64, 256);
        TEST_CHECK(Block != nullptr);
        FillPattern(Block, 64);

        Block = Memory::Realloc(Block, 64 * 1024, 256);
        TEST_CHECK(Block != nullptr);
        TEST_EXPECT(IsAligned(Block, 256));
        TEST_EXPECT(HasPattern(Block, 64));

        Block = Memory::Realloc(Block, 32, 256);
        TEST_CHECK(Block != nullptr);
        TEST_EXPECT(IsAligned(Block, 256));
        TEST_EXPECT(HasPattern(Block, 32));

        Memory::Free(Block);
    }

    TEST_SECTION("Realloc from nullptr allocates an aligned block");
    {
        void* Block = Memory::Realloc(nullptr, 100, 128);
        TEST_EXPECT(Block != nullptr);
        TEST_EXPECT(IsAligned(Block, 128));
        Memory::Free(Block);
    }

    TEST_SECTION("Default-aligned blocks still grow in place through Realloc");
    {
        void* Block = Memory::Malloc(16);
        TEST_CHECK(Block != nullptr);
        FillPattern(Block, 16);

        Block = Memory::Realloc(Block, 4096);
        TEST_CHECK(Block != nullptr);
        TEST_EXPECT(IsAligned(Block, STANDARD_ALIGNMENT));
        TEST_EXPECT(HasPattern(Block, 16));

        Memory::Free(Block);
    }

    TEST_SECTION("MallocZeroed zeroes an over-aligned block");
    {
        uint8* Block = reinterpret_cast<uint8*>(Memory::MallocZeroed(256, 64));
        TEST_CHECK(Block != nullptr);
        TEST_EXPECT(IsAligned(Block, 64));

        bool bAllZero = true;
        for (uint32 Index = 0; Index < 256; ++Index)
        {
            bAllZero = bAllZero && (Block[Index] == 0);
        }

        TEST_EXPECT(bAllZero);
        Memory::Free(Block);
    }

    TEST_END();
}
