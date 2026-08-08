#include "HDRMetadataTests.h"

#include <RHI/RHITypes.h>

#include "TestCommon/TestMacros.h"

bool HDRMetadata_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Chromaticity encodes in 0.00002 increments");
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(0.708f),  uint16(35400)); // Rec.2020 red x
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(0.3127f), uint16(15635)); // D65 white x
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(0.3290f), uint16(16450)); // D65 white y

    TEST_SECTION("Luminance encoding");
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeMinLuminance(0.001f), uint32(10)); // 0.001 / 0.0001
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeNits16(1000.0f),      uint16(1000));

    TEST_SECTION("Out-of-range inputs clamp rather than wrap");
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(2.0f),  uint16(65535));
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(-1.0f), uint16(0));
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeNits16(100000.0f),   uint16(65535));
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeMinLuminance(-1.0f), uint32(0));

    TEST_SECTION("CreateHDR10Default uses Rec.2020 primaries and D65");
    const FRHIHDRMetadata Default = FRHIHDRMetadata::CreateHDR10Default();
    TEST_EXPECT(Default.bIsValid);
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(Default.RedPrimary.X), uint16(35400));
    TEST_EXPECT_EQ(FRHIHDRMetadata::EncodeChromaticity(Default.WhitePoint.X), uint16(15635));

    TEST_END();
}
