#include "StaticString_Test.h"

#if RUN_STATICSTRING_SUITE
#include "TestUtils.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/StringView.h>
#include <Core/Templates/CString.h>

bool StaticString_Suite()
{
    TEST_BEGIN();

    TEST_SECTION("Construction (cstr / substring / view / copy / move)");
    {
        StaticString<64> Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.LastIndex(), StaticString<64>::InvalidIndex);

        StaticString<64> Hello = "Hello";
        TEST_EXPECT_EQ(Hello.Length(), 5);
        TEST_EXPECT_EQ(Hello[0], 'H');
        TEST_EXPECT_EQ(Hello.LastIndex(), 4);

        StaticString<64> Sub = StaticString<64>("Hello World", 5);
        TEST_EXPECT(Sub.Equals("Hello"));

        StringView View("Hello StringView");
        StaticString<64> FromView = StaticString<64>(View);
        TEST_EXPECT(FromView.Equals("Hello StringView"));

        StaticString<64> Copy = Hello;
        TEST_EXPECT(Copy.Equals("Hello"));

        StaticString<64> Moved = ::Move(Copy);
        TEST_EXPECT(Moved.Equals("Hello"));
    }

    TEST_SECTION("Append (cstr / char)");
    {
        StaticString<64> Str = "Hello";
        Str.Append(", World");
        
        TEST_EXPECT_EQ(Str.Length(), 12);
        TEST_EXPECT(Str.Equals("Hello, World"));

        Str.Append('!');
        TEST_EXPECT_EQ(Str.Length(), 13);
        TEST_EXPECT(Str.Equals("Hello, World!"));
    }

    TEST_SECTION("Format / AppendFormat / ToLower / ToUpper");
    {
        StaticString<64> Str;
        
        Str.Format("Formatted String=%.4f", 0.004f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040"));

        Str.AppendFormat("=%.4f", 0.0077f);
        TEST_EXPECT(Str.Equals("Formatted String=0.0040=0.0077"));

        StaticString<64> Mixed = "MixedCase";
        TEST_EXPECT(Mixed.ToLower().Equals("mixedcase"));
        TEST_EXPECT(Mixed.ToUpper().Equals("MIXEDCASE"));
    }

    TEST_SECTION("Reset / Trim / Reverse");
    {
        StaticString<64> Str = "  Trimmable String  ";
        TEST_EXPECT(Str.Trim().Equals("Trimmable String"));

        Str.Reset();
        TEST_EXPECT(Str.IsEmpty());

        Str.Append("123456789");
        TEST_EXPECT(Str.Reverse().Equals("987654321"));
    }

    TEST_SECTION("Find / Contains / StartsWith / EndsWith / predicates");
    {
        StaticString<64> Search = "0123MeSearch89Me89";
        TEST_EXPECT_EQ(Search.Find("Me"), 4);
        TEST_EXPECT_EQ(Search.FindChar('M'), 4);
        TEST_EXPECT(Search.Contains("Me"));
        TEST_EXPECT(Search.StartsWith("0123ME", EStringCaseType::NoCase));
        TEST_EXPECT(Search.EndsWith("ME89", EStringCaseType::NoCase));
        
        TEST_EXPECT_EQ(Search.FindCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);

        TEST_EXPECT_EQ(Search.FindLast("Me"), 14);
        TEST_EXPECT_EQ(Search.FindLastChar('M'), 14);
    }

    TEST_SECTION("Compare / Resize / CopyToBuffer");
    {
        StaticString<64> Upper = "COMPARE";
        StaticString<64> Lower = "compare";
        TEST_EXPECT(Upper.Compare(Lower) != 0);
        TEST_EXPECT_EQ(Upper.Compare(Lower, EStringCaseType::NoCase), 0);

        Lower.Resize(7);
        TEST_EXPECT(TCString<CHAR>::Strcmp(Lower.Data(), "compare") == 0);

        CHAR Buffer[6];
        Buffer[5] = 0;
        Lower.CopyToBuffer(Buffer, 6, 2);
        TEST_EXPECT(TCString<CHAR>::Strcmp(Buffer, "mpare") == 0);
    }

    TEST_SECTION("Insert / Replace / operator+ / comparison operators");
    {
        StaticString<64> Str = "COMPARE";
        
        Str.Insert("lower", 4);
        TEST_EXPECT(Str.Equals("COMPlowerARE"));
        
        Str.Replace("upper", 4);
        TEST_EXPECT(Str.Equals("COMPupperARE"));

        Str.Replace('X', 0);
        TEST_EXPECT(Str.Equals("XOMPupperARE"));

        StaticString<64> Test = "Test";
        StaticString<64> Plus = Test + '5';
        TEST_EXPECT(Plus.Equals("Test5"));

        StaticString<64> PrePlus = '5' + Test;
        TEST_EXPECT(PrePlus.Equals("5Test"));

        TEST_EXPECT(("Test" == Test));
        TEST_EXPECT(!("Test" != Test));
        TEST_EXPECT(("Test" <= Test));
        TEST_EXPECT(!("Test" < Test));
        TEST_EXPECT(("Test" >= Test));
        TEST_EXPECT(!("Test" > Test));
    }

    TEST_SECTION("WStaticString core + conversions");
    {
        WStaticString<64> Wide = L"Hello String";
        TEST_EXPECT_EQ(Wide.Length(), 12);
        Wide.Append(L'!');

        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Data(), L"Hello String!") == 0);

        StaticString<64>  Narrow = "Round Trip";
        WStaticString<64> ToWide = CharToWide(Narrow);
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(ToWide.Data(), L"Round Trip") == 0);

        StaticString<64> Back = WideToChar(ToWide);
        TEST_EXPECT(Back.Equals("Round Trip"));
    }

    TEST_END();
}
#endif
