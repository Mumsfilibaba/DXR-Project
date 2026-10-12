#include "StaticString_Test.h"

#if RUN_STATICSTRING_SUITE
#include "TestUtils.h"

#include <Core/Containers/StaticString.h>
#include <Core/Containers/StringConversion.h>
#include <Core/Containers/StringView.h>
#include <Core/Templates/CString.h>

bool StaticString_Suite()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Construction (cstr / substring / view / copy / move)"));
    {
        FStaticString<64> Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.LastIndex(), FStaticString<64>::InvalidIndex);

        FStaticString<64> Hello = TEXT("Hello");
        TEST_EXPECT_EQ(Hello.Length(), 5);
        TEST_EXPECT_EQ(Hello[0], 'H');
        TEST_EXPECT_EQ(Hello.LastIndex(), 4);

        FStaticString<64> Sub = FStaticString<64>(TEXT("Hello World"), 5);
        TEST_EXPECT(Sub.Equals(TEXT("Hello")));

        FStringView View(TEXT("Hello StringView"));
        FStaticString<64> FromView = FStaticString<64>(View);
        TEST_EXPECT(FromView.Equals(TEXT("Hello StringView")));

        FStaticString<64> Copy = Hello;
        TEST_EXPECT(Copy.Equals(TEXT("Hello")));

        FStaticString<64> Moved = ::Move(Copy);
        TEST_EXPECT(Moved.Equals(TEXT("Hello")));
    }

    TEST_SECTION(TEXT("Append (cstr / char)"));
    {
        FStaticString<64> Str = TEXT("Hello");
        Str.Append(TEXT(", World"));
        
        TEST_EXPECT_EQ(Str.Length(), 12);
        TEST_EXPECT(Str.Equals(TEXT("Hello, World")));

        Str.Append('!');
        TEST_EXPECT_EQ(Str.Length(), 13);
        TEST_EXPECT(Str.Equals(TEXT("Hello, World!")));
    }

    TEST_SECTION(TEXT("InlinePrintf / AppendPrintf / ToLower / ToUpper"));
    {
        FStaticString<64> Str;
        
        Str.InlinePrintf(TEXT("Formatted String=%.4f"), 0.004f);
        TEST_EXPECT(Str.Equals(TEXT("Formatted String=0.0040")));

        Str.AppendPrintf(TEXT("=%.4f"), 0.0077f);
        TEST_EXPECT(Str.Equals(TEXT("Formatted String=0.0040=0.0077")));

        FStaticString<64> Mixed = TEXT("MixedCase");
        TEST_EXPECT(Mixed.ToLower().Equals(TEXT("mixedcase")));
        TEST_EXPECT(Mixed.ToUpper().Equals(TEXT("MIXEDCASE")));
    }

    TEST_SECTION(TEXT("Printf that does not fit is cut off, narrow and wide"));
    {
        TStaticString<ANSICHAR, 8> Narrow;
        Narrow.InlinePrintf("%s", "0123456789");
        TEST_EXPECT_EQ(Narrow.Length(), 7);
        TEST_EXPECT(Narrow.Equals("0123456"));

        Narrow.AppendPrintf("%d", 42);
        TEST_EXPECT(Narrow.Equals("0123456"));

        Narrow.InlinePrintf("%s", "0123456");
        TEST_EXPECT(Narrow.Equals("0123456"));

        TStaticString<WIDECHAR, 8> Wide;
        Wide.InlinePrintf(L"%s", L"0123456789");
        TEST_EXPECT(Wide.Length() >= 0 && Wide.Length() <= 7);
        TEST_EXPECT(Wide.Data()[Wide.Length()] == 0);

        Wide.InlinePrintf(L"%s%c", L"Åäö", L'!');
        TEST_EXPECT(Wide.Equals(L"Åäö!"));

        Wide.AppendPrintf(L"%d", 12);
        TEST_EXPECT(Wide.Equals(L"Åäö!12"));
    }

    TEST_SECTION(TEXT("Reset / Trim / Reverse"));
    {
        FStaticString<64> Str = TEXT("  Trimmable String  ");
        TEST_EXPECT(Str.Trim().Equals(TEXT("Trimmable String")));

        Str.Reset();
        TEST_EXPECT(Str.IsEmpty());

        Str.Append(TEXT("123456789"));
        TEST_EXPECT(Str.Reverse().Equals(TEXT("987654321")));
    }

    TEST_SECTION(TEXT("Find / Contains / StartsWith / EndsWith / predicates"));
    {
        FStaticString<64> Search = TEXT("0123MeSearch89Me89");
        TEST_EXPECT_EQ(Search.Find(TEXT("Me")), 4);
        TEST_EXPECT_EQ(Search.FindChar('M'), 4);
        TEST_EXPECT(Search.Contains(TEXT("Me")));
        TEST_EXPECT(Search.StartsWith(TEXT("0123ME"), EStringCaseType::NoCase));
        TEST_EXPECT(Search.EndsWith(TEXT("ME89"), EStringCaseType::NoCase));
        
        TEST_EXPECT_EQ(Search.FindCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);

        TEST_EXPECT_EQ(Search.FindLast(TEXT("Me")), 14);
        TEST_EXPECT_EQ(Search.FindLastChar('M'), 14);
    }

    TEST_SECTION(TEXT("Compare / Resize / CopyToBuffer"));
    {
        FStaticString<64> Upper = TEXT("COMPARE");
        FStaticString<64> Lower = TEXT("compare");
        TEST_EXPECT(Upper.Compare(Lower) != 0);
        TEST_EXPECT_EQ(Upper.Compare(Lower, EStringCaseType::NoCase), 0);

        Lower.Resize(7);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Lower.Data(), TEXT("compare")) == 0);

        CHAR_T Buffer[6];
        Buffer[5] = 0;
        Lower.CopyToBuffer(Buffer, 6, 2);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Buffer, TEXT("mpare")) == 0);
    }

    TEST_SECTION(TEXT("Insert / Replace / operator+ / comparison operators"));
    {
        FStaticString<64> Str = TEXT("COMPARE");
        
        Str.Insert(TEXT("lower"), 4);
        TEST_EXPECT(Str.Equals(TEXT("COMPlowerARE")));
        
        Str.Replace(TEXT("upper"), 4);
        TEST_EXPECT(Str.Equals(TEXT("COMPupperARE")));

        Str.Replace('X', 0);
        TEST_EXPECT(Str.Equals(TEXT("XOMPupperARE")));

        FStaticString<64> Test = TEXT("Test");
        FStaticString<64> Plus = Test + '5';
        TEST_EXPECT(Plus.Equals(TEXT("Test5")));

        FStaticString<64> PrePlus = '5' + Test;
        TEST_EXPECT(PrePlus.Equals(TEXT("5Test")));

        TEST_EXPECT((TEXT("Test") == Test));
        TEST_EXPECT(!(TEXT("Test") != Test));
        TEST_EXPECT((TEXT("Test") <= Test));
        TEST_EXPECT(!(TEXT("Test") < Test));
        TEST_EXPECT((TEXT("Test") >= Test));
        TEST_EXPECT(!(TEXT("Test") > Test));
    }

    TEST_SECTION(TEXT("FWideStaticString core + conversions"));
    {
        FWideStaticString<64> Wide = L"Hello String";
        TEST_EXPECT_EQ(Wide.Length(), 12);
        Wide.Append(L'!');

        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(Wide.Data(), L"Hello String!") == 0);

        FStaticString<64>     Narrow = TEXT("Round Trip");
        FWideStaticString<64> ToWide = StringCast<WIDECHAR>(Narrow).Get();
        TEST_EXPECT(TCString<WIDECHAR>::Strcmp(ToWide.Data(), L"Round Trip") == 0);

        FStaticString<64> Back = StringCast<CHAR_T>(ToWide).Get();
        TEST_EXPECT(Back.Equals(TEXT("Round Trip")));
    }

    TEST_END();
}
#endif
