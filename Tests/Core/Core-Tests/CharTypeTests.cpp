#include "CharTypeTests.h"

#include <Core/Core.h>
#include <Core/Templates/TypeTraits/DecayTraits.h>
#include <Core/Templates/TypeTraits/EqualTraits.h>

#if PLATFORM_WINDOWS
    #include <Core/Windows/Windows.h>
#endif

#include "TestCommon/TestMacros.h"

static_assert(TIsSame<TDecay<decltype(TEXT("Text"))>::Type, const CHAR*>::Value);
static_assert(TIsSame<decltype(TEXT('A')), CHAR>::Value);
static_assert(TIsSame<TDecay<decltype(TEXT(__FILE__))>::Type, const CHAR*>::Value);
static_assert(TIsSame<CHAR, CHAR_T>::Value);

#if WIDE_CHAR_BUILD
static_assert(TIsSame<CHAR, wchar_t>::Value);
#else
static_assert(TIsSame<CHAR, char>::Value);
#endif

bool CharType_Test()
{
    TEST_BEGIN();

#if WIDE_CHAR_BUILD
    TEST_SECTION("A wide build stores code units of wchar_t");
    {
        const CHAR* Text = TEXT("ÁÅäö");
        TEST_EXPECT(Text[0] == 0xC1 && Text[1] == 0xC5 && Text[2] == 0xE4 && Text[3] == 0xF6 && Text[4] == 0);
    }
#else
    TEST_SECTION("A narrow build stores UTF-8, whatever the system code page is");
    {
        const CHAR*    Text     = TEXT("ÁÅäö 日本語");
        const char8_t* Expected = u8"ÁÅäö 日本語";

        int32 Index = 0;
        while (Expected[Index] != 0 && static_cast<uint8>(Text[Index]) == static_cast<uint8>(Expected[Index]))
        {
            ++Index;
        }

        TEST_EXPECT(Expected[Index] == 0 && Text[Index] == 0);
        TEST_EXPECT(Index == 18);
    }
#endif

    TEST_END();
}
