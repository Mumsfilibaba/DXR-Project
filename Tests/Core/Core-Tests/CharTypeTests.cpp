#include "CharTypeTests.h"

#include <Core/Core.h>
#include <Core/Platform/PlatformString.h>
#include <Core/Templates/CString.h>
#include <Core/Templates/TypeTraits/DecayTraits.h>
#include <Core/Templates/TypeTraits/EqualTraits.h>

#if PLATFORM_WINDOWS
    #include <Core/Windows/Windows.h>
#endif

#include "TestCommon/TestMacros.h"

static_assert(TIsSame<TDecay<decltype(TEXT("Text"))>::Type, const CHAR_T*>::Value);
static_assert(TIsSame<decltype(TEXT('A')), CHAR_T>::Value);
static_assert(TIsSame<TDecay<decltype(TEXT(__FILE__))>::Type, const CHAR_T*>::Value);

// CHAR stays a plain char in both builds, the same type the Windows SDK's CHAR names, so the two never clash
static_assert(TIsSame<CHAR, ANSICHAR>::Value);
static_assert(TIsSame<CHAR, char>::Value);

#if WIDE_CHAR_BUILD
static_assert(TIsSame<CHAR_T, wchar_t>::Value);
#else
static_assert(TIsSame<CHAR_T, char>::Value);
#endif

bool CharType_Test()
{
    TEST_BEGIN();

#if WIDE_CHAR_BUILD
    TEST_SECTION("A wide build stores code units of wchar_t");
    {
        const CHAR_T* Text = TEXT("ÁÅäö");
        TEST_EXPECT(Text[0] == 0xC1 && Text[1] == 0xC5 && Text[2] == 0xE4 && Text[3] == 0xF6 && Text[4] == 0);
    }
#else
    TEST_SECTION("A narrow build stores UTF-8, whatever the system code page is");
    {
        const CHAR_T*  Text     = TEXT("ÁÅäö 日本語");
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

    TEST_SECTION("Wide formats are rewritten so standard C reads %s and %c as wide, like MSVC");
    {
        WIDECHAR Converted[128];
        TEST_EXPECT(FPlatformString::ConvertWideFormatToStandard(Converted, 128, L"%s %c %ls %lc %d %%s %%%s %-10s %.*s %5.2f %1$s %hs %zu"));
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Converted, L"%ls %lc %ls %lc %d %%s %%%ls %-10ls %.*ls %5.2f %1$ls %hs %zu") == 0);

        TEST_EXPECT(FPlatformString::ConvertWideFormatToStandard(Converted, 128, L""));
        TEST_EXPECT(Converted[0] == 0);

        TEST_EXPECT(!FPlatformString::ConvertWideFormatToStandard(Converted, 4, L"%s%s"));
    }

    TEST_SECTION("LOG_* takes a plain literal format and makes it a TEXT() literal");
    {
        #define CHARTYPE_TEST_LOG(...) LOG_INFO("[CharType] " __VA_ARGS__)

        LOG_INFO("[CharType] A format without arguments");
        LOG_INFO("[CharType] %s=%d", TEXT("Value"), 5);
        CHARTYPE_TEST_LOG("A prefixed format, as the RHI log macros build them: %s", TEXT("ok"));

        #undef CHARTYPE_TEST_LOG
    }

    TEST_END();
}
