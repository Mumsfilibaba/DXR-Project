#include "PlatformMiscTests.h"

#include <Core/Containers/String.h>
#include <Core/Platform/PlatformMisc.h>

#include "TestCommon/TestMacros.h"

static constexpr CHAR_T GTestEnvName[] = TEXT("DXR_PLATFORM_MISC_ENV_TEST");

static void RestoreEnvironmentVariable(const FString& PreviousValue, bool bHadPrevious)
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

    FString PreviousValue;
    const bool bHadPrevious = FPlatformMisc::GetEnvironmentVariable(GTestEnvName, PreviousValue);

    TEST_SECTION(TEXT("Missing variable"));
    {
        FPlatformMisc::SetEnvironmentVariable(GTestEnvName, nullptr);

        FString Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION(TEXT("Set and get"));
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, TEXT("MetalRHI")));

        FString Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT_EQ(Value, TEXT("MetalRHI"));
    }

    TEST_SECTION(TEXT("Overwrite"));
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, TEXT("VulkanRHI")));

        FString Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT_EQ(Value, TEXT("VulkanRHI"));
    }

    TEST_SECTION(TEXT("Empty value is distinct from missing"));
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, TEXT("")));

        FString Value;
        TEST_EXPECT(FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION(TEXT("Null value removes the variable"));
    {
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, TEXT("Keep")));
        TEST_EXPECT(FPlatformMisc::SetEnvironmentVariable(GTestEnvName, nullptr));

        FString Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(GTestEnvName, Value));
        TEST_EXPECT(Value.IsEmpty());
    }

    TEST_SECTION(TEXT("Empty name is rejected"));
    {
        FString Value;
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(TEXT(""), Value));
        TEST_EXPECT(!FPlatformMisc::SetEnvironmentVariable(TEXT(""), TEXT("Value")));
        TEST_EXPECT(!FPlatformMisc::GetEnvironmentVariable(nullptr, Value));
        TEST_EXPECT(!FPlatformMisc::SetEnvironmentVariable(nullptr, TEXT("Value")));
    }

    RestoreEnvironmentVariable(PreviousValue, bHadPrevious);

    TEST_END();
}
