#include "String_Test.h"

#if RUN_TSTRING_TEST
#include "TestUtils.h"

#include <Core/Containers/String.h>
#include <Core/Containers/StringConversion.h>

#include <string>

bool TString_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("IsEmpty / Length / Append / Add / Pop"));
    {
        FString Str;
        TEST_EXPECT(Str.IsEmpty());

        Str.Append(TEXT("Hello"));
        TEST_EXPECT(!Str.IsEmpty());
        TEST_EXPECT_EQ(Str.Length(), 5);

        Str.Append(' ');
        Str.Append(TEXT("World"));
        TEST_EXPECT_EQ(Str.Length(), 11);
        TEST_EXPECT(Str.Equals(TEXT("Hello World")));

        Str.Add('!');
        TEST_EXPECT(Str.Equals(TEXT("Hello World!")));

        Str.Pop();
        TEST_EXPECT(Str.Equals(TEXT("Hello World")));

        TEST_EXPECT_EQ(Str.Length(), 11);
        TEST_EXPECT_EQ(Str.Data()[Str.Length()], '\0');
    }

    TEST_SECTION(TEXT("First / Last / indexing"));
    {
        FString Str = TEXT("abcde");
        TEST_EXPECT_EQ(Str.First(), 'a');
        TEST_EXPECT_EQ(Str[2], 'c');
        TEST_EXPECT_EQ(Str.LastIndex(), 4);
        TEST_EXPECT_EQ(Str.Last(), 'e');

        const FString Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.LastIndex(), FString::InvalidIndex);

        FString Cleared = TEXT("abc");
        Cleared.Clear();
        TEST_EXPECT_EQ(Cleared.LastIndex(), FString::InvalidIndex);
    }

    TEST_SECTION(TEXT("ToLower / ToUpper (copy + inline)"));
    {
        FString Mixed = TEXT("MixedCase123");
        TEST_EXPECT(Mixed.ToLower().Equals(TEXT("mixedcase123")));
        TEST_EXPECT(Mixed.ToUpper().Equals(TEXT("MIXEDCASE123")));

        FString Lower = TEXT("abc");
        
        Lower.ToUpperInline();
        TEST_EXPECT(Lower.Equals(TEXT("ABC")));
        
        Lower.ToLowerInline();
        TEST_EXPECT(Lower.Equals(TEXT("abc")));
    }

    TEST_SECTION(TEXT("Trim variants"));
    {
        FString Padded = TEXT("   spaced   ");
        TEST_EXPECT(Padded.Trim().Equals(TEXT("spaced")));
        TEST_EXPECT(Padded.TrimStart().Equals(TEXT("spaced   ")));
        TEST_EXPECT(Padded.TrimEnd().Equals(TEXT("   spaced")));

        FString Inline = TEXT("  inline  ");
        Inline.TrimInline();
        TEST_EXPECT(Inline.Equals(TEXT("inline")));
    }

    TEST_SECTION(TEXT("Reverse"));
    {
        FString Str = TEXT("abcd");
        
        TEST_EXPECT(Str.Reverse().Equals(TEXT("dcba")));
        Str.ReverseInline();

        TEST_EXPECT(Str.Equals(TEXT("dcba")));
    }

    TEST_SECTION(TEXT("Compare / Equals with case sensitivity"));
    {
        FString Str = TEXT("Hello");
        TEST_EXPECT_EQ(Str.Compare(TEXT("Hello")), 0);
        TEST_EXPECT(Str.Compare(TEXT("hello")) != 0);
        TEST_EXPECT_EQ(Str.Compare(TEXT("hello"), EStringCaseType::NoCase), 0);
        TEST_EXPECT(Str.Equals(TEXT("Hello")));
        TEST_EXPECT(!Str.Equals(TEXT("hello")));
        TEST_EXPECT(Str.Equals(TEXT("hello"), EStringCaseType::NoCase));
    }

    TEST_SECTION(TEXT("Find / FindLast / FindChar / Contains"));
    {
        FString Str = TEXT("abcabc");
        TEST_EXPECT_EQ(Str.Find(TEXT("bc")), 1);
        TEST_EXPECT_EQ(Str.FindLast(TEXT("bc")), 4);
        TEST_EXPECT_EQ(Str.FindChar('c'), 2);
        TEST_EXPECT_EQ(Str.FindLastChar('a'), 3);
        TEST_EXPECT(Str.Contains(TEXT("cab")));
        TEST_EXPECT(!Str.Contains(TEXT("xyz")));
        TEST_EXPECT(Str.Contains('a'));
        TEST_EXPECT_EQ(Str.Find(TEXT("zzz")), FString::InvalidIndex);
    }

    TEST_SECTION(TEXT("StartsWith / EndsWith"));
    {
        FString Str = TEXT("FileName.txt");
        TEST_EXPECT(Str.StartsWith(TEXT("File")));
        TEST_EXPECT(!Str.StartsWith(TEXT("file")));
        TEST_EXPECT(Str.StartsWith(TEXT("file"), EStringCaseType::NoCase));
        TEST_EXPECT(Str.EndsWith(TEXT(".txt")));
        TEST_EXPECT(!Str.EndsWith(TEXT(".png")));

        TEST_EXPECT(Str.StartsWith(TEXT("FileName.txt")));
        TEST_EXPECT(Str.EndsWith(TEXT("FileName.txt")));
        TEST_EXPECT(FString(TEXT("/")).EndsWith(TEXT("/")));
        TEST_EXPECT(!Str.EndsWith(TEXT("AFileName.txt")));
        TEST_EXPECT(!Str.StartsWith(TEXT("FileName.txtA")));
    }

    TEST_SECTION(TEXT("Insert / Remove / Replace / ReplaceAll"));
    {
        FString Str = TEXT("HelloWorld");
        Str.Insert(' ', 5);
        TEST_EXPECT(Str.Equals(TEXT("Hello World")));

        Str.Remove(5, 1);
        TEST_EXPECT(Str.Equals(TEXT("HelloWorld")));

        FString Rep = TEXT("aaaa");
        Rep.Replace(TEXT("bb"), 1);
        TEST_EXPECT(Rep.Equals(TEXT("abba")));

        FString All = TEXT("a.b.c.d");
        All.ReplaceAll('.', '-');
        TEST_EXPECT(All.Equals(TEXT("a-b-c-d")));
    }

    TEST_SECTION(TEXT("SubString / Split / Swap"));
    {
        FString Str = TEXT("abcdef");
        TEST_EXPECT(Str.SubString(2, 3).Equals(TEXT("cde")));
        TEST_EXPECT(Str.SubString(0, Str.Length()).Equals(TEXT("abcdef")));
        TEST_EXPECT(Str.SubString(3, Str.Length() - 3).Equals(TEXT("def")));
        TEST_EXPECT(Str.SubString(Str.Length(), 0).IsEmpty());
        TEST_EXPECT(Str.SubString(2, 0).IsEmpty());

        FString Left;
        FString Right;
        FString Path = TEXT("key=value");
        Path.Split('=', Left, Right);

        TEST_EXPECT(Left.Equals(TEXT("key")));
        TEST_EXPECT(Right.Equals(TEXT("value")));

        FString First  = TEXT("first");
        FString Second = TEXT("second");
        First.Swap(Second);

        TEST_EXPECT(First.Equals(TEXT("second")));
        TEST_EXPECT(Second.Equals(TEXT("first")));
    }

    TEST_SECTION(TEXT("Clear / Reset / Reserve"));
    {
        FString Str = TEXT("Some content");
        Str.Reserve(64);
        TEST_EXPECT(Str.Capacity() >= 64);

        Str.Clear();
        TEST_EXPECT(Str.IsEmpty());
        TEST_EXPECT_EQ(Str.Length(), 0);

        Str.Reset(TEXT("Reset"), 5);
        TEST_EXPECT(Str.Equals(TEXT("Reset")));
    }

    TEST_SECTION(TEXT("operator+ / operator+="));
    {
        FString First = TEXT("foo");
        FString Second = TEXT("bar");
        FString Combined = First + Second;
        TEST_EXPECT(Combined.Equals(TEXT("foobar")));

        First += Second;
        TEST_EXPECT(First.Equals(TEXT("foobar")));

        First += '!';
        TEST_EXPECT(First.Equals(TEXT("foobar!")));
    }

    TEST_SECTION(TEXT("Construction / capacity invariants"));
    {
        FString Empty;
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.Capacity(), 1); // null-terminator invariant

        FString Hello = TEXT("Hello String");
        TEST_EXPECT_EQ(Hello.Length(), 12);
        TEST_EXPECT_EQ(Hello.Capacity(), 13);

        FString Sub = FString(TEXT("Hello World"), 5);
        TEST_EXPECT(Sub.Equals(TEXT("Hello")));
        TEST_EXPECT_EQ(Sub.Length(), 5);

        FStringView View(TEXT("Hello StringView"));
        FString FromView = FString(View);
        TEST_EXPECT(FromView.Equals(TEXT("Hello StringView")));

        FString Copy = Hello;
        TEST_EXPECT(Copy.Equals(TEXT("Hello String")));

        FString Moved = ::Move(Copy);
        TEST_EXPECT(Moved.Equals(TEXT("Hello String")));
        TEST_EXPECT(Copy.IsEmpty());
    }

    TEST_SECTION(TEXT("InlinePrintf / AppendPrintf"));
    {
        FString Str;
        Str.InlinePrintf(TEXT("Formatted String=%.4f"), 0.004f);
        TEST_EXPECT(Str.Equals(TEXT("Formatted String=0.0040")));

        Str.Append('_');
        Str.AppendPrintf(TEXT("Formatted String=%.4f"), 0.0077f);
        TEST_EXPECT(Str.Equals(TEXT("Formatted String=0.0040_Formatted String=0.0077")));
    }

    TEST_SECTION(TEXT("Reset to length"));
    {
        FString Str = TEXT("NewString");
        Str.Reset(12);

        TEST_EXPECT_EQ(Str.Size(), 12);
        TEST_EXPECT_EQ(Str.Capacity(), 13);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Str.Data(), TEXT("")) == 0);
    }

    TEST_SECTION(TEXT("FindCharWithPredicate / FindLastCharWithPredicate"));
    {
        FString Str = TEXT("0123MeSearch89Me89");
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

        TEST_EXPECT_EQ(Str.FindLast(TEXT("Me")), 14);
        TEST_EXPECT_EQ(Str.FindLastChar('M'), 14);
    }

    TEST_SECTION(TEXT("CopyToBuffer"));
    {
        FString Str = TEXT("compare");
        CHAR_T Buffer[6];
        Buffer[5] = 0;
        Str.CopyToBuffer(Buffer, 6, 2);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Buffer, TEXT("mpare")) == 0);
    }

    TEST_SECTION(TEXT("Insert(string) / Replace(string) / Replace(char)"));
    {
        FString Str = TEXT("COMPARE");

        Str.Insert(TEXT("lower"), 4);
        TEST_EXPECT(Str.Equals(TEXT("COMPlowerARE")));

        Str.Replace(TEXT("upper"), 4);
        TEST_EXPECT(Str.Equals(TEXT("COMPupperARE")));

        Str.Replace('X', 0);
        TEST_EXPECT(Str.Equals(TEXT("XOMPupperARE")));
    }

    TEST_SECTION(TEXT("operator+ with chars / comparison operators"));
    {
        FString Str  = TEXT("Test");
        FString Plus = Str + '5';
        TEST_EXPECT(Plus.Equals(TEXT("Test5")));

        FString PrePlus = '5' + Str;
        TEST_EXPECT(PrePlus.Equals(TEXT("5Test")));

        TEST_EXPECT((TEXT("Test") == Str));
        TEST_EXPECT((Str == TEXT("Test")));
        TEST_EXPECT(!(TEXT("Test") != Str));
        TEST_EXPECT((TEXT("Test") <= Str));
        TEST_EXPECT(!(TEXT("Test") < Str));
        TEST_EXPECT((TEXT("Test") >= Str));
        TEST_EXPECT(!(TEXT("Test") > Str));
    }

    TEST_SECTION(TEXT("StringCast round-trip"));
    {
        FString     Narrow = TEXT("Round Trip 123");
        FWideString Wide   = StringCast<WIDECHAR>(Narrow).ToString();
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Data(), L"Round Trip 123") == 0);

        FString Back = StringCast<CHAR_T>(Wide).ToString();
        TEST_EXPECT(Back.Equals(TEXT("Round Trip 123")));
    }

    TEST_SECTION(TEXT("Printf: %s and %c take the string's own character type, at any length"));
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

            const FString NarrowFormatted = FString::Printf(TEXT("%s"), *Narrow);
            TEST_EXPECT_EQ(NarrowFormatted.Length(), Length);
            TEST_EXPECT(NarrowFormatted.Equals(Narrow));

            const FWideString WideFormatted = FWideString::Printf(L"%s", *Wide);
            TEST_EXPECT_EQ(WideFormatted.Length(), Length);
            TEST_EXPECT(WideFormatted.Equals(Wide));

            FWideString WideAppended = L"<";
            WideAppended.AppendPrintf(L"%s>", *Wide);
            TEST_EXPECT_EQ(WideAppended.Length(), Length + 2);
        }

        FString Self = TEXT("abc");
        Self.InlinePrintf(TEXT("%s-%s"), *Self, *Self);
        TEST_EXPECT(Self.Equals(TEXT("abc-abc")));
    }

    TEST_SECTION(TEXT("FWideString core operations"));
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

    TEST_SECTION(TEXT("FAnsiString holds UTF-8 in plain chars"));
    {
        FAnsiString Str = "Åäö 日本語";
        TEST_EXPECT_EQ(Str.Length(), 16);
        TEST_EXPECT_EQ(Str.Find("日本"), 7);

        Str.AppendPrintf(" %d", 42);
        TEST_EXPECT(Str.Equals("Åäö 日本語 42"));
    }

    TEST_SECTION(TEXT("FU8String core operations"));
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

    TEST_SECTION(TEXT("FString reallocation stress (seeded size sweep vs std::string)"));
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom     Random(Seed);
            FString     Str;
            std::basic_string<CHAR_T> Oracle;

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
                LOG_ERROR("[STRESS FAIL] FString grow seed=%u size=%d", Seed, TargetSize);
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
