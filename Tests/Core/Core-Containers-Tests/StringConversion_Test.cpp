#include "StringConversion_Test.h"

#if RUN_TSTRINGCONVERSION_TEST
#include "TestUtils.h"

#include <Core/Containers/StringConversion.h>

template<typename ToCharType, typename FromCharType>
static bool ConvertsTo(const FromCharType* Source, int32 SourceLength, const ToCharType* Expected, int32 ExpectedLength)
{
    static constexpr int32 BufferLength = 64;

    ToCharType Buffer[BufferLength];
    const int32 NumWritten = FStringConversion::Convert(Buffer, BufferLength, Source, SourceLength);
    const int32 NumNeeded  = FStringConversion::Convert(static_cast<ToCharType*>(nullptr), 0, Source, SourceLength);
    if (NumWritten != ExpectedLength || NumNeeded != ExpectedLength)
    {
        return false;
    }

    for (int32 Index = 0; Index < ExpectedLength; ++Index)
    {
        if (Buffer[Index] != Expected[Index])
        {
            return false;
        }
    }

    return true;
}

template<typename ToCharType, typename FromCharType, int32 NumSource, int32 NumExpected>
static bool ConvertsTo(const FromCharType (&Source)[NumSource], const ToCharType (&Expected)[NumExpected])
{
    return ConvertsTo(Source, NumSource, Expected, NumExpected);
}

bool TStringConversion_Test()
{
    TEST_BEGIN();

    const UTF8CHAR Utf8[]  = { 0xC3, 0x85, 0xC3, 0xA4, 0xC3, 0xB6, ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ' ', 0xF0, 0x9F, 0x98, 0x80 };
    const char16_t Utf16[] = { 0xC5, 0xE4, 0xF6, ' ', 0x65E5, 0x672C, 0x8A9E, ' ', 0xD83D, 0xDE00 };
    const char32_t Utf32[] = { 0xC5, 0xE4, 0xF6, ' ', 0x65E5, 0x672C, 0x8A9E, ' ', 0x1F600 };

    TEST_SECTION(TEXT("Every pair of encodings converts both ways"));
    {
        TEST_EXPECT(ConvertsTo(Utf8, Utf16));
        TEST_EXPECT(ConvertsTo(Utf8, Utf32));
        TEST_EXPECT(ConvertsTo(Utf16, Utf8));
        TEST_EXPECT(ConvertsTo(Utf16, Utf32));
        TEST_EXPECT(ConvertsTo(Utf32, Utf8));
        TEST_EXPECT(ConvertsTo(Utf32, Utf16));
    }

    TEST_SECTION(TEXT("Invalid UTF-8 becomes U+FFFD, one per maximal invalid subsequence"));
    {
        const UTF8CHAR LoneContinuation[] = { 'A', 0x80, 'B' };
        const char32_t LoneContinuationExpected[] = { 'A', 0xFFFD, 'B' };
        TEST_EXPECT(ConvertsTo(LoneContinuation, LoneContinuationExpected));

        const UTF8CHAR Truncated[] = { 0xE6, 0x97, 'A' };
        const char32_t TruncatedExpected[] = { 0xFFFD, 'A' };
        TEST_EXPECT(ConvertsTo(Truncated, TruncatedExpected));

        const UTF8CHAR TruncatedAtEnd[] = { 'A', 0xF0, 0x9F, 0x98 };
        const char32_t TruncatedAtEndExpected[] = { 'A', 0xFFFD };
        TEST_EXPECT(ConvertsTo(TruncatedAtEnd, TruncatedAtEndExpected));

        const UTF8CHAR Overlong[] = { 0xC0, 0xAF, 0xE0, 0x80, 0xAF };
        const char32_t OverlongExpected[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD };
        TEST_EXPECT(ConvertsTo(Overlong, OverlongExpected));

        const UTF8CHAR EncodedSurrogate[] = { 0xED, 0xA0, 0x80 };
        const char32_t EncodedSurrogateExpected[] = { 0xFFFD, 0xFFFD, 0xFFFD };
        TEST_EXPECT(ConvertsTo(EncodedSurrogate, EncodedSurrogateExpected));

        const UTF8CHAR AboveMax[] = { 0xF4, 0x90, 0x80, 0x80, 0xFF };
        const char32_t AboveMaxExpected[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD };
        TEST_EXPECT(ConvertsTo(AboveMax, AboveMaxExpected));

        // U+FFFD itself is EF BF BD in UTF-8
        const UTF8CHAR Invalid[]         = { 0x80 };
        const char16_t Replacement[]     = { 0xFFFD };
        const UTF8CHAR ReplacementUtf8[] = { 0xEF, 0xBF, 0xBD };
        TEST_EXPECT(ConvertsTo(Invalid, Replacement));
        TEST_EXPECT(ConvertsTo(Replacement, ReplacementUtf8));
    }

    TEST_SECTION(TEXT("Invalid UTF-16 and UTF-32 become U+FFFD"));
    {
        const char16_t LoneSurrogates[] = { 0xD800, 'A', 0xDC00, 'B', 0xD83D };
        const char32_t LoneSurrogatesExpected[] = { 0xFFFD, 'A', 0xFFFD, 'B', 0xFFFD };
        TEST_EXPECT(ConvertsTo(LoneSurrogates, LoneSurrogatesExpected));

        const char32_t OutOfRange[] = { 0x110000, 0xD800, 'A' };
        const char16_t OutOfRangeExpected[] = { 0xFFFD, 0xFFFD, 'A' };
        TEST_EXPECT(ConvertsTo(OutOfRange, OutOfRangeExpected));
    }

    TEST_SECTION(TEXT("A code point that does not fit is not written in part"));
    {
        UTF8CHAR Buffer[8] = {};
        TEST_EXPECT_EQ(FStringConversion::Convert(Buffer, 4, u"日本", 2), 3);
        TEST_EXPECT(Buffer[0] == 0xE6 && Buffer[1] == 0x97 && Buffer[2] == 0xA5 && Buffer[3] == 0);
    }

    TEST_SECTION(TEXT("StringCast between CHAR_T and WIDECHAR"));
    {
        const FString Text = TEXT("Åäö 日本語 😀");

        const TStringConversion<WIDECHAR, CHAR_T> Wide = StringCast<WIDECHAR>(Text);
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Get(), L"Åäö 日本語 😀") == 0);
        TEST_EXPECT_EQ(Wide.Length(), TCString<WIDECHAR>::Strlen(L"Åäö 日本語 😀"));

        const FString Back = StringCast<CHAR_T>(Wide.Get()).ToString();
        TEST_EXPECT(Back.Equals(Text));

        const TStringConversion<UTF8CHAR, WIDECHAR> FromWide = StringCast<UTF8CHAR>(L"Åäö 日本語 😀");
        TEST_EXPECT(TCString<UTF8CHAR>::Strcmp(FromWide.Get(), u8"Åäö 日本語 😀") == 0);
        TEST_EXPECT_EQ(FromWide.Length(), 21);
    }

    TEST_SECTION(TEXT("StringCast points at null-terminated text of the same encoding instead of copying"));
    {
        const FAnsiString Ansi = "Åäö";
        TEST_EXPECT(reinterpret_cast<const void*>(StringCast<UTF8CHAR>(Ansi).Get()) == reinterpret_cast<const void*>(*Ansi));
        TEST_EXPECT(reinterpret_cast<const void*>(StringCast<ANSICHAR>(Ansi.Data()).Get()) == reinterpret_cast<const void*>(Ansi.Data()));

        const FWideString Wide = L"Wide";
        TEST_EXPECT(StringCast<WIDECHAR>(Wide).Get() == *Wide);

        // A view is not null-terminated where it ends, so it is copied
        const FAnsiStringView View = FAnsiStringView(Ansi).SubString(0, 2);
        const TStringConversion<UTF8CHAR, ANSICHAR> FromView = StringCast<UTF8CHAR>(View);
        TEST_EXPECT(reinterpret_cast<const void*>(FromView.Get()) != reinterpret_cast<const void*>(View.Data()));
        TEST_EXPECT(TCString<UTF8CHAR>::Strcmp(FromView.Get(), u8"Å") == 0);

        const TStringConversion<ANSICHAR, ANSICHAR> FromLength = StringCast<ANSICHAR>(Ansi.Data(), 2);
        TEST_EXPECT(FromLength.Length() == 2 && FromLength.Get()[2] == 0);
    }

    TEST_SECTION(TEXT("StringCast of empty, null and static strings"));
    {
        const TStringConversion<WIDECHAR, ANSICHAR> Empty = StringCast<WIDECHAR>("");
        TEST_EXPECT(Empty.Length() == 0 && Empty.Get()[0] == 0);

        const TStringConversion<WIDECHAR, ANSICHAR> Null = StringCast<WIDECHAR>(static_cast<const ANSICHAR*>(nullptr));
        TEST_EXPECT(Null.Length() == 0 && Null.Get() != nullptr && Null.Get()[0] == 0);

        const TStringConversion<ANSICHAR, WIDECHAR> EmptyWide = StringCast<ANSICHAR>(FWideString());
        TEST_EXPECT(EmptyWide.Length() == 0 && EmptyWide.Get()[0] == 0);

        const TStaticString<ANSICHAR, 32> Static = "日本語";
        const FWideString FromStatic = StringCast<WIDECHAR>(Static).ToString();
        TEST_EXPECT(FromStatic.Equals(L"日本語"));
    }

    TEST_END();
}
#endif
