#include "PlatformMiscTests.h"

#include <Core/Containers/String.h>
#include <Core/Platform/PlatformMisc.h>

#include "TestCommon/TestMacros.h"

static constexpr CHAR GTestEnvName[] = "DXR_PLATFORM_MISC_ENV_TEST";

static void RestoreEnvironmentVariable(const String& PreviousValue, bool bHadPrevious)
{
    if (bHadPrevious)
    {
        FPlatformMisc::SetEnvironmentVariable(GTestEnvName, *PreviousValue);
    }
    else
    {
        FPlatformMisc::SetEnvironmentVariable(GTestEnvName, nullptr);
    }
}

bool PlatformMisc_Test()
{
    TEST_BEGIN();

    String PreviousValue;
    const bool bHadPrevious = FPlatformMisc::GetEnvironmentVariable(GTestEnvName, PreviousValue);

    TEST_SECTION("Missing variable");
    {
        FPlatformMisc::SetEnvironmentVariable(GTestEnvName, nullptr);

        String Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION("Set and get");
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, "MetalRHI"));

        String Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT_EQ(Value, "MetalRHI");
    }

    TEST_SECTION("Overwrite");
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, "VulkanRHI"));

        String Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT_EQ(Value, "VulkanRHI");
    }

    TEST_SECTION("Empty value is distinct from missing");
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, ""));

        String Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION("Null value removes the variable");
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, "Keep"));
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, nullptr));

        String Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION("Empty name is rejected");
    {
        String Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable("", Value));
        TEST_EXPECT(!FPlatformMisc::SetEnvironmentVariable("", "Value"));
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(nullptr, Value));
        TEST_EXPECT(!FPlatformMisc::SetEnvironmentVariable(nullptr, "Value"));
    }

    RestoreEnvironmentVariable(PreviousValue, bHadPrevious);

    TEST_END();
}
