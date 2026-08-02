#include "CRC_Test.h"

#if RUN_CRC_TEST
#include "TestUtils.h"

#include <Core/Misc/CRC.h>
#include <Core/Templates/CString.h>

static uint32 CRCOf(const CHAR* Text)
{
    return CRC32::Generate(Text, static_cast<uint64>(TCString<CHAR>::Strlen(Text)));
}

bool CRC_Test()
{
    TEST_BEGIN();

    TEST_SECTION("CRC32 known-answer vectors (IEEE / zlib)");
    {
        TEST_EXPECT_EQ(CRC32::Generate("", 0), 0x00000000u);
        TEST_EXPECT_EQ(CRCOf("a"), 0xE8B7BE43u);
        TEST_EXPECT_EQ(CRCOf("abc"), 0x352441C2u);
        TEST_EXPECT_EQ(CRCOf("123456789"), 0xCBF43926u);
        TEST_EXPECT_EQ(CRCOf("The quick brown fox jumps over the lazy dog"), 0x414FA339u);
    }

    TEST_SECTION("CRC32 determinism / distinctness");
    {
        TEST_EXPECT_EQ(CRCOf("abc"), CRCOf("abc"));
        TEST_EXPECT(CRCOf("abc") != CRCOf("abd"));
    }

    TEST_END();
}
#endif
