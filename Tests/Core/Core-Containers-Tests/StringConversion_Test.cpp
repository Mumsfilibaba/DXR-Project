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

template<typename CharType>
static bool StepsThrough(const CharType* Text, int32 Length, const int32* ExpectedStarts, int32 NumStarts)
{
    int32 Index = 0;
    for (int32 Position = 0; Position < NumStarts; ++Position)
    {
        if (Index != ExpectedStarts[Position])
        {
            return false;
        }

        Index = FStringConversion::FindNextCodePoint(Text, Length, Index);
    }

    if (Index != Length || FStringConversion::FindNextCodePoint(Text, Length, Index) != Length)
    {
        return false;
    }

    for (int32 Position = NumStarts - 1; Position >= 0; --Position)
    {
        Index = FStringConversion::FindPreviousCodePoint(Text, Index);
        if (Index != ExpectedStarts[Position])
        {
            return false;
        }
    }

    return FStringConversion::FindPreviousCodePoint(Text, 0) == 0;
}

template<typename CharType, int32 Length, int32 NumStarts>
static bool StepsThrough(const CharType (&Text)[Length], const int32 (&ExpectedStarts)[NumStarts])
{
    return StepsThrough(Text, Length, ExpectedStarts, NumStarts);
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

    TEST_SECTION(TEXT("Decoding one code point gives its value and how many code units it took"));
    {
        int32 NumUnits = 0;
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf8 + 7, 14, NumUnits) == 0x65E5 && NumUnits == 3);
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf8 + 17, 4, NumUnits) == 0x1F600 && NumUnits == 4);
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf16 + 8, 2, NumUnits) == 0x1F600 && NumUnits == 2);
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf32 + 4, 5, NumUnits) == 0x65E5 && NumUnits == 1);

        // A sequence the text ends in the middle of is one invalid code point
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf8 + 7, 2, NumUnits) == FStringConversion::ReplacementCharacter && NumUnits == 2);
        TEST_EXPECT(FStringConversion::DecodeCodePoint(Utf16 + 8, 1, NumUnits) == FStringConversion::ReplacementCharacter && NumUnits == 1);
    }

    TEST_SECTION(TEXT("Encoding one code point writes all of its code units, and U+FFFD for one that is not valid"));
    {
        UTF8CHAR Utf8Units[FStringConversion::MaxCodeUnitsPerCodePoint] = {};
        TEST_EXPECT_EQ(FStringConversion::EncodeCodePoint(0x1F600, Utf8Units), 4);
        TEST_EXPECT(Utf8Units[0] == 0xF0 && Utf8Units[1] == 0x9F && Utf8Units[2] == 0x98 && Utf8Units[3] == 0x80);

        TEST_EXPECT_EQ(FStringConversion::EncodeCodePoint(0xD800, Utf8Units), 3);
        TEST_EXPECT(Utf8Units[0] == 0xEF && Utf8Units[1] == 0xBF && Utf8Units[2] == 0xBD);

        char16_t Utf16Units[FStringConversion::MaxCodeUnitsPerCodePoint] = {};
        TEST_EXPECT_EQ(FStringConversion::EncodeCodePoint(0x1F600, Utf16Units), 2);
        TEST_EXPECT(Utf16Units[0] == 0xD83D && Utf16Units[1] == 0xDE00);

        char32_t Utf32Units[FStringConversion::MaxCodeUnitsPerCodePoint] = {};
        TEST_EXPECT_EQ(FStringConversion::EncodeCodePoint(0x110000, Utf32Units), 1);
        TEST_EXPECT(Utf32Units[0] == 0xFFFD);

        CHAR_T Units[FStringConversion::MaxCodeUnitsPerCodePoint] = {};
        const int32 NumUnits = FStringConversion::EncodeCodePoint(0x65E5, Units);
        TEST_EXPECT(FString(Units, NumUnits).Equals(TEXT("日")));
    }

    TEST_SECTION(TEXT("Stepping forward and back by code point stops at the start of every character"));
    {
        const int32 Utf8Starts[]  = { 0, 2, 4, 6, 7, 10, 13, 16, 17 };
        const int32 Utf16Starts[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        const int32 Utf32Starts[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        TEST_EXPECT(StepsThrough(Utf8, Utf8Starts));
        TEST_EXPECT(StepsThrough(Utf16, Utf16Starts));
        TEST_EXPECT(StepsThrough(Utf32, Utf32Starts));

        const FString Text = TEXT("Åäö 日本語 😀");
        int32 NumCharacters = 0;
        for (int32 Index = 0; Index < Text.Length(); Index = FStringConversion::FindNextCodePoint(Text.Data(), Text.Length(), Index))
        {
            NumCharacters++;
        }

        TEST_EXPECT_EQ(NumCharacters, 9);
    }

    TEST_SECTION(TEXT("Stepping back over invalid text stops where stepping forward did"));
    {
        const UTF8CHAR LoneContinuation[] = { 'A', 0x80, 0x80, 0x80, 0x80, 'B' };
        const int32 LoneContinuationStarts[] = { 0, 1, 2, 3, 4, 5 };
        TEST_EXPECT(StepsThrough(LoneContinuation, LoneContinuationStarts));

        const UTF8CHAR Truncated[] = { 'A', 0xE6, 0x97, 'B', 0xE6, 0x97 };
        const int32 TruncatedStarts[] = { 0, 1, 3, 4 };
        TEST_EXPECT(StepsThrough(Truncated, TruncatedStarts));

        const char16_t LoneSurrogates[] = { 'A', 0xDC00, 0xD800, 'B', 0xD800 };
        const int32 LoneSurrogatesStarts[] = { 0, 1, 2, 3, 4 };
        TEST_EXPECT(StepsThrough(LoneSurrogates, LoneSurrogatesStarts));
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
