#include "PlatformTimeTests.h"

#include <Core/CoreDefines.h>
#include <Core/Platform/PlatformTime.h>

#include "TestCommon/TestMacros.h"

bool PlatformTime_Test()
{
    TEST_BEGIN();

#if PLATFORM_MACOS
    // Ratios fed in directly, so both cases are checkable from either machine
    TEST_SECTION("Timebase ticks per second");
    {
        // Intel: mach_absolute_time already counts nanoseconds
        TEST_EXPECT_EQ(FPlatformTime::TicksPerSecondFromTimebase(1, 1), 1000000000ull);

        // Apple Silicon: 125/3 describes the 24 MHz counter the M-series uses
        TEST_EXPECT_EQ(FPlatformTime::TicksPerSecondFromTimebase(125, 3), 24000000ull);
    }

    TEST_SECTION("Frequency matches the running machine's timebase");
    {
        mach_timebase_info_data_t TimeBaseInfo = {};
        mach_timebase_info(&TimeBaseInfo);

        TEST_EXPECT_EQ(FPlatformTime::QueryPerformanceFrequency(), FPlatformTime::TicksPerSecondFromTimebase(TimeBaseInfo.numer, TimeBaseInfo.denom));
    }
#endif

    TEST_SECTION("Counter advances");
    {
        const uint64 First = FPlatformTime::QueryPerformanceCounter();

        uint64 Second = First;
        while (Second == First)
        {
            Second = FPlatformTime::QueryPerformanceCounter();
        }

        TEST_EXPECT(Second > First);
        TEST_EXPECT(FPlatformTime::QueryPerformanceFrequency() > 0);
    }

    TEST_END();
}
