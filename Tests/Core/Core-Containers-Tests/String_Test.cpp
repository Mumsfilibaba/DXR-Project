#include "String_Test.h"

#if RUN_TSTRING_TEST
#include "TestUtils.h"

#include <Core/Containers/String.h>

#include <string>

bool TString_Test()
{
    TEST_BEGIN();

    TEST_SECTION("IsEmpty / Length / Append / Add / Pop");
    {
        String Str;
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
        String Str = "abcde";
        TEST_EXPECT_EQ(Str.First(), 'a');
        TEST_EXPECT_EQ(Str[2], 'c');
        TEST_EXPECT_EQ(Str.LastIndex(), 4);
        TEST_EXPECT_EQ(Str.Last(), 'e');

        const String Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.LastIndex(), String::InvalidIndex);

        String Cleared = "abc";
        Cleared.Clear();
        TEST_EXPECT_EQ(Cleared.LastIndex(), String::InvalidIndex);
    }

    TEST_SECTION("ToLower / ToUpper (copy + inline)");
    {
        String Mixed = "MixedCase123";
        TEST_EXPECT(Mixed.ToLower().Equals("mixedcase123"));
        TEST_EXPECT(Mixed.ToUpper().Equals("MIXEDCASE123"));

        String Lower = "abc";
        
        Lower.ToUpperInline();
        TEST_EXPECT(Lower.Equals("ABC"));
        
        Lower.ToLowerInline();
        TEST_EXPECT(Lower.Equals("abc"));
    }

    TEST_SECTION("Trim variants");
    {
        String Padded = "   spaced   ";
        TEST_EXPECT(Padded.Trim().Equals("spaced"));
        TEST_EXPECT(Padded.TrimStart().Equals("spaced   "));
        TEST_EXPECT(Padded.TrimEnd().Equals("   spaced"));

        String Inline = "  inline  ";
        Inline.TrimInline();
        TEST_EXPECT(Inline.Equals("inline"));
    }

    TEST_SECTION("Reverse");
    {
        String Str = "abcd";
        
        TEST_EXPECT(Str.Reverse().Equals("dcba"));
        Str.ReverseInline();

        TEST_EXPECT(Str.Equals("dcba"));
    }

    TEST_SECTION("Compare / Equals with case sensitivity");
    {
        String Str = "Hello";
        TEST_EXPECT_EQ(Str.Compare("Hello"), 0);
        TEST_EXPECT(Str.Compare("hello") != 0);
        TEST_EXPECT_EQ(Str.Compare("hello", EStringCaseType::NoCase), 0);
        TEST_EXPECT(Str.Equals("Hello"));
        TEST_EXPECT(!Str.Equals("hello"));
        TEST_EXPECT(Str.Equals("hello", EStringCaseType::NoCase));
    }

    TEST_SECTION("Find / FindLast / FindChar / Contains");
    {
        String Str = "abcabc";
        TEST_EXPECT_EQ(Str.Find("bc"), 1);
        TEST_EXPECT_EQ(Str.FindLast("bc"), 4);
        TEST_EXPECT_EQ(Str.FindChar('c'), 2);
        TEST_EXPECT_EQ(Str.FindLastChar('a'), 3);
        TEST_EXPECT(Str.Contains("cab"));
        TEST_EXPECT(!Str.Contains("xyz"));
        TEST_EXPECT(Str.Contains('a'));
        TEST_EXPECT_EQ(Str.Find("zzz"), String::InvalidIndex);
    }

    TEST_SECTION("StartsWith / EndsWith");
    {
        String Str = "FileName.txt";
        TEST_EXPECT(Str.StartsWith("File"));
        TEST_EXPECT(!Str.StartsWith("file"));
        TEST_EXPECT(Str.StartsWith("file", EStringCaseType::NoCase));
        TEST_EXPECT(Str.EndsWith(".txt"));
        TEST_EXPECT(!Str.EndsWith(".png"));
    }

    TEST_SECTION("Insert / Remove / Replace / ReplaceAll");
    {
        String Str = "HelloWorld";
        Str.Insert(' ', 5);
        TEST_EXPECT(Str.Equals("Hello World"));

        Str.Remove(5, 1);
        TEST_EXPECT(Str.Equals("HelloWorld"));

        String Rep = "aaaa";
        Rep.Replace("bb", 1);
        TEST_EXPECT(Rep.Equals("abba"));

        String All = "a.b.c.d";
        All.ReplaceAll('.', '-');
        TEST_EXPECT(All.Equals("a-b-c-d"));
    }

    TEST_SECTION("SubString / Split / Swap");
    {
        String Str = "abcdef";
        TEST_EXPECT(Str.SubString(2, 3).Equals("cde"));
        TEST_EXPECT(Str.SubString(0, Str.Length()).Equals("abcdef"));
        TEST_EXPECT(Str.SubString(3, Str.Length() - 3).Equals("def"));
        TEST_EXPECT(Str.SubString(Str.Length(), 0).IsEmpty());
        TEST_EXPECT(Str.SubString(2, 0).IsEmpty());

        String Left;
        String Right;
        String Path = "key=value";
        Path.Split('=', Left, Right);

        TEST_EXPECT(Left.Equals("key"));
        TEST_EXPECT(Right.Equals("value"));

        String First  = "first";
        String Second = "second";
        First.Swap(Second);

        TEST_EXPECT(First.Equals("second"));
        TEST_EXPECT(Second.Equals("first"));
    }

    TEST_SECTION("Clear / Reset / Reserve");
    {
        String Str = "Some content";
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
        String First = "foo";
        String Second = "bar";
        String Combined = First + Second;
        TEST_EXPECT(Combined.Equals("foobar"));

        First += Second;
        TEST_EXPECT(First.Equals("foobar"));

        First += '!';
        TEST_EXPECT(First.Equals("foobar!"));
    }

    TEST_SECTION("Construction / capacity invariants");
    {
        String Empty;
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.Capacity(), 1); // null-terminator invariant

        String Hello = "Hello String";
        TEST_EXPECT_EQ(Hello.Length(), 12);
        TEST_EXPECT_EQ(Hello.Capacity(), 13);

        String Sub = String("Hello World", 5);
        TEST_EXPECT(Sub.Equals("Hello"));
        TEST_EXPECT_EQ(Sub.Length(), 5);

        StringView View("Hello StringView");
        String FromView = String(View);
        TEST_EXPECT(FromView.Equals("Hello StringView"));

        String Copy = Hello;
        TEST_EXPECT(Copy.Equals("Hello String"));

        String Moved = ::Move(Copy);
        TEST_EXPECT(Moved.Equals("Hello String"));
        TEST_EXPECT(Copy.IsEmpty());
    }

    TEST_SECTION("Format / AppendFormat");
    {
        String Str;
        Str.Format("Formatted String=%.4f", 0.004f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040"));

        Str.Append('_');
        Str.AppendFormat("Formatted String=%.4f", 0.0077f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040_Formatted String=0.0077"));
    }

    TEST_SECTION("Reset to length");
    {
        String Str = "NewString";
        Str.Reset(12);

        TEST_EXPECT_EQ(Str.Size(), 12);
        TEST_EXPECT_EQ(Str.Capacity(), 13);
        TEST_EXPECT(TCString<CHAR>::Strcmp(Str.Data(), "") == 0);
    }

    TEST_SECTION("FindCharWithPredicate / FindLastCharWithPredicate");
    {
        String Str = "0123MeSearch89Me89";
        TEST_EXPECT_EQ(Str.FindCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);
        
        TEST_EXPECT_EQ(Str.FindCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'M') || (Ch == 'c');
        }), 4);
        
        TEST_EXPECT_EQ(Str.FindLastCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'h') || (Ch == 'M') || (Ch == 'c');
        }), 14);

        TEST_EXPECT_EQ(Str.FindLast("Me"), 14);
        TEST_EXPECT_EQ(Str.FindLastChar('M'), 14);
    }

    TEST_SECTION("CopyToBuffer");
    {
        String Str = "compare";
        CHAR Buffer[6];
        Buffer[5] = 0;
        Str.CopyToBuffer(Buffer, 6, 2);
        TEST_EXPECT(TCString<CHAR>::Strcmp(Buffer, "mpare") == 0);
    }

    TEST_SECTION("Insert(string) / Replace(string) / Replace(char)");
    {
        String Str = "COMPARE";

        Str.Insert("lower", 4);
        TEST_EXPECT(Str.Equals("COMPlowerARE"));

        Str.Replace("upper", 4);
        TEST_EXPECT(Str.Equals("COMPupperARE"));

        Str.Replace('X', 0);
        TEST_EXPECT(Str.Equals("XOMPupperARE"));
    }

    TEST_SECTION("operator+ with chars / comparison operators");
    {
        String Str  = "Test";
        String Plus = Str + '5';
        TEST_EXPECT(Plus.Equals("Test5"));

        String PrePlus = '5' + Str;
        TEST_EXPECT(PrePlus.Equals("5Test"));

        TEST_EXPECT(("Test" == Str));
        TEST_EXPECT((Str == "Test"));
        TEST_EXPECT(!("Test" != Str));
        TEST_EXPECT(("Test" <= Str));
        TEST_EXPECT(!("Test" < Str));
        TEST_EXPECT(("Test" >= Str));
        TEST_EXPECT(!("Test" > Str));
    }

    TEST_SECTION("CharToWide / WideToChar round-trip");
    {
        String  Narrow = "Round Trip 123";
        WString Wide   = CharToWide(Narrow);
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Data(), L"Round Trip 123") == 0);

        String Back = WideToChar(Wide);
        TEST_EXPECT(Back.Equals("Round Trip 123"));
    }

    TEST_SECTION("WString core operations");
    {
        WString Str = L"Hello String";
        TEST_EXPECT_EQ(Str.Length(), 12);

        Str.Append(L'!');
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Str.Data(), L"Hello String!") == 0);

        WString Formatted;
        Formatted.Format(L"Formatted String=%.4f", 0.004f);
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Formatted.Data(), L"Formatted String=0.0040") == 0);

        WString Search = L"0123MeSearch89Me89";
        TEST_EXPECT_EQ(Search.Find(L"Me"), 4);
        TEST_EXPECT_EQ(Search.FindLast(L"Me"), 14);
        TEST_EXPECT(Search.StartsWith(L"0123ME", EStringCaseType::NoCase));
        TEST_EXPECT(Search.EndsWith(L"ME89", EStringCaseType::NoCase));

        WString Lower = Search.ToLower();
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Lower.Data(), L"0123mesearch89me89") == 0);
    }

    TEST_SECTION("String reallocation stress (seeded size sweep vs std::string)");
    {
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            FRandom     Random(Seed);
            String      Str;
            std::string Oracle;

            // Grow the buffer with a random mix of append/insert across the boundary sizes.
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                const CHAR Ch = static_cast<CHAR>('a' + static_cast<int32>(Random.RandInt(0, 25)));
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

            bool bMatches = (Str.Length() == static_cast<int32>(Oracle.size())) && (TCString<CHAR>::Strcmp(Str.Data(), Oracle.c_str()) == 0);
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
