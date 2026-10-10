#include "String_Test.h"

#if RUN_TSTRING_TEST
#include "TestUtils.h"

#include <Core/Containers/String.h>
#include <Core/Containers/StringConversion.h>

#include <string>

bool TString_Test()
{
    TEST_BEGIN();

    TEST_SECTION("IsEmpty / Length / Append / Add / Pop");
    {
        FString Str;
        TEST_EXPECT(Str.IsEmpty());

        Str.Append("Hello");
        TEST_EXPECT(!Str.IsEmpty());
        TEST_EXPECT_EQ(Str.Length(), 5);

        Str.Append(' ');
        Str.Append("World");
        TEST_EXPECT_EQ(Str.Length(), 11);
        TEST_EXPECT(Str.Equals("Hello World"));

        Str.Add('!');
        TEST_EXPECT(Str.Equals("Hello World!"));

        Str.Pop();
        TEST_EXPECT(Str.Equals("Hello World"));

        TEST_EXPECT_EQ(Str.Length(), 11);
        TEST_EXPECT_EQ(Str.Data()[Str.Length()], '\0');
    }

    TEST_SECTION("First / Last / indexing");
    {
        FString Str = "abcde";
        TEST_EXPECT_EQ(Str.First(), 'a');
        TEST_EXPECT_EQ(Str[2], 'c');
        TEST_EXPECT_EQ(Str.LastIndex(), 4);
        TEST_EXPECT_EQ(Str.Last(), 'e');

        const FString Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.LastIndex(), FString::InvalidIndex);

        FString Cleared = "abc";
        Cleared.Clear();
        TEST_EXPECT_EQ(Cleared.LastIndex(), FString::InvalidIndex);
    }

    TEST_SECTION("ToLower / ToUpper (copy + inline)");
    {
        FString Mixed = "MixedCase123";
        TEST_EXPECT(Mixed.ToLower().Equals("mixedcase123"));
        TEST_EXPECT(Mixed.ToUpper().Equals("MIXEDCASE123"));

        FString Lower = "abc";
        
        Lower.ToUpperInline();
        TEST_EXPECT(Lower.Equals("ABC"));
        
        Lower.ToLowerInline();
        TEST_EXPECT(Lower.Equals("abc"));
    }

    TEST_SECTION("Trim variants");
    {
        FString Padded = "   spaced   ";
        TEST_EXPECT(Padded.Trim().Equals("spaced"));
        TEST_EXPECT(Padded.TrimStart().Equals("spaced   "));
        TEST_EXPECT(Padded.TrimEnd().Equals("   spaced"));

        FString Inline = "  inline  ";
        Inline.TrimInline();
        TEST_EXPECT(Inline.Equals("inline"));
    }

    TEST_SECTION("Reverse");
    {
        FString Str = "abcd";
        
        TEST_EXPECT(Str.Reverse().Equals("dcba"));
        Str.ReverseInline();

        TEST_EXPECT(Str.Equals("dcba"));
    }

    TEST_SECTION("Compare / Equals with case sensitivity");
    {
        FString Str = "Hello";
        TEST_EXPECT_EQ(Str.Compare("Hello"), 0);
        TEST_EXPECT(Str.Compare("hello") != 0);
        TEST_EXPECT_EQ(Str.Compare("hello", EStringCaseType::NoCase), 0);
        TEST_EXPECT(Str.Equals("Hello"));
        TEST_EXPECT(!Str.Equals("hello"));
        TEST_EXPECT(Str.Equals("hello", EStringCaseType::NoCase));
    }

    TEST_SECTION("Find / FindLast / FindChar / Contains");
    {
        FString Str = "abcabc";
        TEST_EXPECT_EQ(Str.Find("bc"), 1);
        TEST_EXPECT_EQ(Str.FindLast("bc"), 4);
        TEST_EXPECT_EQ(Str.FindChar('c'), 2);
        TEST_EXPECT_EQ(Str.FindLastChar('a'), 3);
        TEST_EXPECT(Str.Contains("cab"));
        TEST_EXPECT(!Str.Contains("xyz"));
        TEST_EXPECT(Str.Contains('a'));
        TEST_EXPECT_EQ(Str.Find("zzz"), FString::InvalidIndex);
    }

    TEST_SECTION("StartsWith / EndsWith");
    {
        FString Str = "FileName.txt";
        TEST_EXPECT(Str.StartsWith("File"));
        TEST_EXPECT(!Str.StartsWith("file"));
        TEST_EXPECT(Str.StartsWith("file", EStringCaseType::NoCase));
        TEST_EXPECT(Str.EndsWith(".txt"));
        TEST_EXPECT(!Str.EndsWith(".png"));

        TEST_EXPECT(Str.StartsWith("FileName.txt"));
        TEST_EXPECT(Str.EndsWith("FileName.txt"));
        TEST_EXPECT(FString("/").EndsWith("/"));
        TEST_EXPECT(!Str.EndsWith("AFileName.txt"));
        TEST_EXPECT(!Str.StartsWith("FileName.txtA"));
    }

    TEST_SECTION("Insert / Remove / Replace / ReplaceAll");
    {
        FString Str = "HelloWorld";
        Str.Insert(' ', 5);
        TEST_EXPECT(Str.Equals("Hello World"));

        Str.Remove(5, 1);
        TEST_EXPECT(Str.Equals("HelloWorld"));

        FString Rep = "aaaa";
        Rep.Replace("bb", 1);
        TEST_EXPECT(Rep.Equals("abba"));

        FString All = "a.b.c.d";
        All.ReplaceAll('.', '-');
        TEST_EXPECT(All.Equals("a-b-c-d"));
    }

    TEST_SECTION("SubString / Split / Swap");
    {
        FString Str = "abcdef";
        TEST_EXPECT(Str.SubString(2, 3).Equals("cde"));
        TEST_EXPECT(Str.SubString(0, Str.Length()).Equals("abcdef"));
        TEST_EXPECT(Str.SubString(3, Str.Length() - 3).Equals("def"));
        TEST_EXPECT(Str.SubString(Str.Length(), 0).IsEmpty());
        TEST_EXPECT(Str.SubString(2, 0).IsEmpty());

        FString Left;
        FString Right;
        FString Path = "key=value";
        Path.Split('=', Left, Right);

        TEST_EXPECT(Left.Equals("key"));
        TEST_EXPECT(Right.Equals("value"));

        FString First  = "first";
        FString Second = "second";
        First.Swap(Second);

        TEST_EXPECT(First.Equals("second"));
        TEST_EXPECT(Second.Equals("first"));
    }

    TEST_SECTION("Clear / Reset / Reserve");
    {
        FString Str = "Some content";
        Str.Reserve(64);
        TEST_EXPECT(Str.Capacity() >= 64);

        Str.Clear();
        TEST_EXPECT(Str.IsEmpty());
        TEST_EXPECT_EQ(Str.Length(), 0);

        Str.Reset("Reset", 5);
        TEST_EXPECT(Str.Equals("Reset"));
    }

    TEST_SECTION("operator+ / operator+=");
    {
        FString First = "foo";
        FString Second = "bar";
        FString Combined = First + Second;
        TEST_EXPECT(Combined.Equals("foobar"));

        First += Second;
        TEST_EXPECT(First.Equals("foobar"));

        First += '!';
        TEST_EXPECT(First.Equals("foobar!"));
    }

    TEST_SECTION("Construction / capacity invariants");
    {
        FString Empty;
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.Capacity(), 1); // null-terminator invariant

        FString Hello = "Hello String";
        TEST_EXPECT_EQ(Hello.Length(), 12);
        TEST_EXPECT_EQ(Hello.Capacity(), 13);

        FString Sub = FString("Hello World", 5);
        TEST_EXPECT(Sub.Equals("Hello"));
        TEST_EXPECT_EQ(Sub.Length(), 5);

        FStringView View("Hello StringView");
        FString FromView = FString(View);
        TEST_EXPECT(FromView.Equals("Hello StringView"));

        FString Copy = Hello;
        TEST_EXPECT(Copy.Equals("Hello String"));

        FString Moved = ::Move(Copy);
        TEST_EXPECT(Moved.Equals("Hello String"));
        TEST_EXPECT(Copy.IsEmpty());
    }

    TEST_SECTION("InlinePrintf / AppendPrintf");
    {
        FString Str;
        Str.InlinePrintf("Formatted String=%.4f", 0.004f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040"));

        Str.Append('_');
        Str.AppendPrintf("Formatted String=%.4f", 0.0077f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040_Formatted String=0.0077"));
    }

    TEST_SECTION("Reset to length");
    {
        FString Str = "NewString";
        Str.Reset(12);

        TEST_EXPECT_EQ(Str.Size(), 12);
        TEST_EXPECT_EQ(Str.Capacity(), 13);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Str.Data(), "") == 0);
    }

    TEST_SECTION("FindCharWithPredicate / FindLastCharWithPredicate");
    {
        FString Str = "0123MeSearch89Me89";
        TEST_EXPECT_EQ(Str.FindCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);
        
        TEST_EXPECT_EQ(Str.FindCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'M') || (Ch == 'c');
        }), 4);
        
        TEST_EXPECT_EQ(Str.FindLastCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'h') || (Ch == 'M') || (Ch == 'c');
        }), 14);

        TEST_EXPECT_EQ(Str.FindLast("Me"), 14);
        TEST_EXPECT_EQ(Str.FindLastChar('M'), 14);
    }

    TEST_SECTION("CopyToBuffer");
    {
        FString Str = "compare";
        CHAR_T Buffer[6];
        Buffer[5] = 0;
        Str.CopyToBuffer(Buffer, 6, 2);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Buffer, "mpare") == 0);
    }

    TEST_SECTION("Insert(string) / Replace(string) / Replace(char)");
    {
        FString Str = "COMPARE";

        Str.Insert("lower", 4);
        TEST_EXPECT(Str.Equals("COMPlowerARE"));

        Str.Replace("upper", 4);
        TEST_EXPECT(Str.Equals("COMPupperARE"));

        Str.Replace('X', 0);
        TEST_EXPECT(Str.Equals("XOMPupperARE"));
    }

    TEST_SECTION("operator+ with chars / comparison operators");
    {
        FString Str  = "Test";
        FString Plus = Str + '5';
        TEST_EXPECT(Plus.Equals("Test5"));

        FString PrePlus = '5' + Str;
        TEST_EXPECT(PrePlus.Equals("5Test"));

        TEST_EXPECT(("Test" == Str));
        TEST_EXPECT((Str == "Test"));
        TEST_EXPECT(!("Test" != Str));
        TEST_EXPECT(("Test" <= Str));
        TEST_EXPECT(!("Test" < Str));
        TEST_EXPECT(("Test" >= Str));
        TEST_EXPECT(!("Test" > Str));
    }

    TEST_SECTION("StringCast round-trip");
    {
        FString     Narrow = "Round Trip 123";
        FWideString Wide   = StringCast<WIDECHAR>(Narrow).ToString();
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Data(), L"Round Trip 123") == 0);

        FString Back = StringCast<CHAR_T>(Wide).ToString();
        TEST_EXPECT(Back.Equals("Round Trip 123"));
    }

    TEST_SECTION("Printf: %s and %c take the string's own character type, at any length");
    {
        TEST_EXPECT(FString::Printf(TEXT("%s=%d %c"), TEXT("Value"), 5, TEXT('x')).Equals(TEXT("Value=5 x")));
        TEST_EXPECT(FWideString::Printf(L"%s=%d %c", L"Wide", 5, L'x').Equals(L"Wide=5 x"));
        TEST_EXPECT(FWideString::Printf(L"[%s]", L"Åäö 日本語").Equals(L"[Åäö 日本語]"));

        const int32 Lengths[] = { STRING_FORMAT_BUFFER_SIZE - 1, STRING_FORMAT_BUFFER_SIZE, STRING_FORMAT_BUFFER_SIZE + 1, 5000 };
        for (int32 Length : Lengths)
        {
            FString     Narrow;
            FWideString Wide;
            for (int32 Index = 0; Index < Length; ++Index)
            {
                Narrow.Append(static_cast<CHAR_T>('a' + Index % 26));
                Wide.Append(static_cast<WIDECHAR>(L'a' + Index % 26));
            }

            const FString NarrowFormatted = FString::Printf("%s", *Narrow);
            TEST_EXPECT_EQ(NarrowFormatted.Length(), Length);
            TEST_EXPECT(NarrowFormatted.Equals(Narrow));

            const FWideString WideFormatted = FWideString::Printf(L"%s", *Wide);
            TEST_EXPECT_EQ(WideFormatted.Length(), Length);
            TEST_EXPECT(WideFormatted.Equals(Wide));

            FWideString WideAppended = L"<";
            WideAppended.AppendPrintf(L"%s>", *Wide);
            TEST_EXPECT_EQ(WideAppended.Length(), Length + 2);
        }

        FString Self = "abc";
        Self.InlinePrintf("%s-%s", *Self, *Self);
        TEST_EXPECT(Self.Equals("abc-abc"));
    }

    TEST_SECTION("WString core operations");
    {
        FWideString Str = L"Hello String";
        TEST_EXPECT_EQ(Str.Length(), 12);

        Str.Append(L'!');
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Str.Data(), L"Hello String!") == 0);

        FWideString Formatted;
        Formatted.InlinePrintf(L"Formatted String=%.4f", 0.004f);
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Formatted.Data(), L"Formatted String=0.0040") == 0);

        FWideString Search = L"0123MeSearch89Me89";
        TEST_EXPECT_EQ(Search.Find(L"Me"), 4);
        TEST_EXPECT_EQ(Search.FindLast(L"Me"), 14);
        TEST_EXPECT(Search.StartsWith(L"0123ME", EStringCaseType::NoCase));
        TEST_EXPECT(Search.EndsWith(L"ME89", EStringCaseType::NoCase));

        FWideString Lower = Search.ToLower();
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Lower.Data(), L"0123mesearch89me89") == 0);
    }

    TEST_SECTION("AnsiString holds UTF-8 in plain chars");
    {
        FAnsiString Str = "Åäö 日本語";
        TEST_EXPECT_EQ(Str.Length(), 16);
        TEST_EXPECT_EQ(Str.Find("日本"), 7);

        Str.AppendPrintf(" %d", 42);
        TEST_EXPECT(Str.Equals("Åäö 日本語 42"));
    }

    TEST_SECTION("U8String core operations");
    {
        FU8String Str = u8"Åäö 日本語";
        TEST_EXPECT_EQ(Str.Length(), 16);
        TEST_EXPECT(TCString<UTF8CHAR>::Strcmp(Str.Data(), u8"Åäö 日本語") == 0);

        Str.Append(u8'!');
        TEST_EXPECT(Str.Equals(u8"Åäö 日本語!"));
        TEST_EXPECT_EQ(Str.Find(u8"日本"), 7);
        TEST_EXPECT_EQ(Str.FindChar(u8' '), 6);
        TEST_EXPECT(Str.EndsWith(u8"語!"));

        FU8String Mixed = u8"Mixed Åäö Case";
        TEST_EXPECT(Mixed.StartsWith(u8"MIXED", EStringCaseType::NoCase));
        TEST_EXPECT(Mixed.ToUpper().Equals(u8"MIXED Åäö CASE"));

        FU8String Formatted;
        Formatted.InlinePrintf(u8"Value=%d", 42);
        TEST_EXPECT(Formatted.Equals(u8"Value=42"));

        const FU8StringView View(Str);
        TEST_EXPECT_EQ(View.Length(), 17);
        TEST_EXPECT(View.SubString(7, 9).Equals(u8"日本語"));

        TEST_EXPECT(FU8String(TCString<UTF8CHAR>::Empty()).IsEmpty());

        TEST_EXPECT_EQ(FPlatformString::Strlen(u8"日本"), 6);
        TEST_EXPECT_EQ(FPlatformString::Stricmp(u8"Åäö ABC", u8"Åäö abc"), 0);
        TEST_EXPECT_EQ(FPlatformString::Strtoi64(u8"-1234", nullptr, 10), -1234);
        TEST_EXPECT_EQ(FPlatformString::Atoi(u8"42"), 42);
        TEST_EXPECT(FPlatformString::ToUpper(u8'a') == u8'A');
    }

    TEST_SECTION("String reallocation stress (seeded size sweep vs std::string)");
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom     Random(Seed);
            FString     Str;
            std::string Oracle;

            // Grow the buffer with a random mix of append/insert across the boundary sizes.
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const CHAR_T Ch = static_cast<CHAR_T>('a' + static_cast<int32>(Random.RandInt(0, 25)));
                if (!Str.IsEmpty() && Random.RandBool())
                {
                    const int32 At = static_cast<int32>(Random.RandInt(0, Str.Length() - 1));
                    Str.Insert(Ch, At);
                    Oracle.insert(Oracle.begin() + At, Ch);
                }
                else
                {
                    Str.Append(Ch);
                    Oracle.push_back(Ch);
                }
            }

            bool bMatches = (Str.Length() == static_cast<int32>(Oracle.size())) && (TCString<CHAR_T>::Strcmp(Str.Data(), Oracle.c_str()) == 0);
            if (!bMatches)
            {
                LOG_ERROR("[STRESS FAIL] String grow seed=%u size=%d", Seed, TargetSize);
            }

            TEST_EXPECT(bMatches);

            // Shrink back to empty with random single-character removals.
            while (!Str.IsEmpty())
            {
                const int32 At = static_cast<int32>(Random.RandInt(0, Str.Length() - 1));
                Str.Remove(At, 1);
                Oracle.erase(Oracle.begin() + At);
            }

            TEST_EXPECT(Oracle.empty());
            TEST_EXPECT(Str.IsEmpty());
        }
    }

    TEST_END();
}
#endif
