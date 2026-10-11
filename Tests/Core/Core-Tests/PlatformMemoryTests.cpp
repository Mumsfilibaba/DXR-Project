#include "PlatformMemoryTests.h"

#include <Core/Platform/PlatformMemory.h>

#include "TestCommon/TestMacros.h"

static bool IsPowerOfTwo(uint64 Value)
{
    return (Value != 0) && ((Value & (Value - 1)) == 0);
}

bool PlatformMemory_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The process's memory use is reported, and every peak is at least the current value");
    {
        FPlatformMemoryStats Stats;
        TEST_CHECK(FPlatformMemory::GetStats(Stats));

        TEST_EXPECT(Stats.FootprintBytes > 0);
        TEST_EXPECT(Stats.ResidentBytes > 0);
        TEST_EXPECT(Stats.AllocatorInUseBytes > 0);
        TEST_EXPECT(Stats.TotalPhysicalBytes > 0);
        TEST_EXPECT(Stats.AvailablePhysicalBytes > 0);
        TEST_EXPECT(Stats.AvailablePhysicalBytes <= Stats.TotalPhysicalBytes);
        TEST_EXPECT(Stats.PeakFootprintBytes >= Stats.FootprintBytes);
        TEST_EXPECT(Stats.PeakResidentBytes >= Stats.ResidentBytes);
    }

    TEST_SECTION("A page is a power of two of at least four kilobytes");
    const uint64 PageSize = FPlatformMemory::GetPageSize();
    TEST_EXPECT(IsPowerOfTwo(PageSize));
    TEST_EXPECT(PageSize >= 4096);

    TEST_SECTION("Pages come back aligned to a page, zeroed and writable");
    {
        const uint64 Size  = PageSize * 3;
        uint8*       Pages = static_cast<uint8*>(FPlatformMemory::AllocatePages(Size));
        TEST_CHECK(Pages != nullptr);
        TEST_EXPECT((reinterpret_cast<UPTR_INT>(Pages) & (PageSize - 1)) == 0);

        bool bAllZero = true;
        for (uint64 Index = 0; Index < Size; ++Index)
        {
            bAllZero = bAllZero && (Pages[Index] == 0);
        }

        TEST_EXPECT(bAllZero);

        for (uint64 Index = 0; Index < Size; ++Index)
        {
            Pages[Index] = static_cast<uint8>(Index * 13 + 5);
        }

        bool bKept = true;
        for (uint64 Index = 0; Index < Size; ++Index)
        {
            bKept = bKept && (Pages[Index] == static_cast<uint8>(Index * 13 + 5));
        }

        TEST_EXPECT(bKept);
        FPlatformMemory::FreePages(Pages, Size);
    }

    TEST_SECTION("A size short of a page still maps the whole page");
    {
        uint8* Page = static_cast<uint8*>(FPlatformMemory::AllocatePages(1));
        TEST_CHECK(Page != nullptr);

        Page[PageSize - 1] = 0x5A;
        TEST_EXPECT_EQ(Page[PageSize - 1], 0x5A);
        FPlatformMemory::FreePages(Page, 1);
    }

    TEST_SECTION("Touching freshly mapped pages raises the footprint the system reports");
    {
        constexpr uint64 TouchedBytes = 64ull * 1024ull * 1024ull;

        FPlatformMemoryStats Before;
        TEST_CHECK(FPlatformMemory::GetStats(Before));

        uint8* Pages = static_cast<uint8*>(FPlatformMemory::AllocatePages(TouchedBytes));
        TEST_CHECK(Pages != nullptr);

        for (uint64 Offset = 0; Offset < TouchedBytes; Offset += PageSize)
        {
            Pages[Offset] = 1;
        }

        FPlatformMemoryStats After;
        TEST_CHECK(FPlatformMemory::GetStats(After));
        TEST_EXPECT(After.FootprintBytes >= Before.FootprintBytes + TouchedBytes / 2);

        FPlatformMemory::FreePages(Pages, TouchedBytes);
    }

    TEST_END();
}
