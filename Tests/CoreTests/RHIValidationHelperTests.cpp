#include "RHIValidationHelperTests.h"

#include <RHI/RHIValidationHelpers.h>
#include <RHI/RHIBuffer.h>
#include <RHI/RHITypes.h>

#include "TestCommon/TestMacros.h"

bool RHIValidationHelpers_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Buffer range boundaries");
    TEST_EXPECT(RHIValidationHelpers::IsRangeValid(16, 0, 16));
    TEST_EXPECT(RHIValidationHelpers::IsRangeValid(16, 15, 1));
    TEST_EXPECT(!RHIValidationHelpers::IsRangeValid(16, 0, 0));
    TEST_EXPECT(!RHIValidationHelpers::IsRangeValid(16, 17, 1));
    TEST_EXPECT(!RHIValidationHelpers::IsRangeValid(16, 15, 2));

    TEST_SECTION("Overflow-safe buffer ranges");
    TEST_EXPECT(RHIValidationHelpers::IsRangeValid(UINT64_MAX, UINT64_MAX - 1, 1));
    TEST_EXPECT(!RHIValidationHelpers::IsRangeValid(UINT64_MAX, UINT64_MAX - 1, 2));
    TEST_EXPECT(!RHIValidationHelpers::IsRangeValid(UINT64_MAX, UINT64_MAX, UINT64_MAX));

    TEST_SECTION("Subresource ranges");
    TEST_EXPECT(RHIValidationHelpers::IsSubresourceRangeValid(8, 0, 8));
    TEST_EXPECT(RHIValidationHelpers::IsSubresourceRangeValid(8, 7, 1));
    TEST_EXPECT(!RHIValidationHelpers::IsSubresourceRangeValid(8, 0, 0));
    TEST_EXPECT(!RHIValidationHelpers::IsSubresourceRangeValid(8, 8, 1));
    TEST_EXPECT(!RHIValidationHelpers::IsSubresourceRangeValid(UINT32_MAX, UINT32_MAX, 1));

    TEST_SECTION("Range overlap");
    TEST_EXPECT(RHIValidationHelpers::DoRangesOverlap(0, 8, 4, 8));
    TEST_EXPECT(!RHIValidationHelpers::DoRangesOverlap(0, 8, 8, 8));
    TEST_EXPECT(!RHIValidationHelpers::DoRangesOverlap(0, 8, 16, 8));
    TEST_EXPECT(RHIValidationHelpers::DoRangesOverlap(UINT64_MAX - 8, 8, UINT64_MAX - 4, 4));

    TEST_SECTION("64-bit buffer copy descriptor");
    constexpr uint64 LargeValue = uint64(UINT32_MAX) + 4096;
    constexpr FRHIBufferCopyDesc CopyDesc(LargeValue, LargeValue + 1, LargeValue + 2);
    TEST_EXPECT_EQ(CopyDesc.SrcOffset, LargeValue);
    TEST_EXPECT_EQ(CopyDesc.DstOffset, LargeValue + 1);
    TEST_EXPECT_EQ(CopyDesc.Size, LargeValue + 2);

    TEST_SECTION("Buffer copy usage flags");
    FRHIBufferDesc BufferDesc;
    BufferDesc.Flags = EBufferFlags::Default | EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopyDest;
    TEST_EXPECT(BufferDesc.IsDefault());
    TEST_EXPECT(BufferDesc.IsShaderResourceBuffer());
    TEST_EXPECT(BufferDesc.IsCopyDest());
    TEST_EXPECT(!BufferDesc.IsCopySource());

    TEST_END();
}
