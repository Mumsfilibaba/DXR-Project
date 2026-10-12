#include "StringView_Test.h"

#if RUN_STRINGVIEW_SUITE
#include "TestUtils.h"

#include <Core/Containers/StringView.h>

bool StringView_Suite()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Construction / indexing / Length / IsEmpty"));
    {
        FStringView Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.LastIndex(), FStringView::InvalidIndex);

        FStringView View(TEXT("Hello, World"));
        TEST_EXPECT_EQ(View.Length(), 12);
        TEST_EXPECT(!View.IsEmpty());
        TEST_EXPECT_EQ(View[0], 'H');
        TEST_EXPECT_EQ(View[11], 'd');
        TEST_EXPECT_EQ(View.LastIndex(), 11);

        const CHAR_T* LongString = TEXT("This is a long string");
        FStringView Slice = FStringView(LongString, 4, 5);
        TEST_EXPECT(Slice.Equals(TEXT("is a")));

        FStringView Prefix = FStringView(TEXT("COMPAREPostfix"), 7);
        TEST_EXPECT(Prefix.Equals(TEXT("COMPARE")));
    }

    TEST_SECTION(TEXT("Find / FindChar / Contains / SubString"));
    {
        FStringView View(TEXT("Hello, World"));
        TEST_EXPECT_EQ(View.FindChar('W'), 7);
        TEST_EXPECT_EQ(View.FindChar('z'), FStringView::InvalidIndex);
        TEST_EXPECT_EQ(View.Find(TEXT("World")), 7);
        TEST_EXPECT_EQ(View.Find(TEXT("xyz")), FStringView::InvalidIndex);

        FStringView Sub = View.SubString(7, 5);
        TEST_EXPECT_EQ(Sub.Length(), 5);
        TEST_EXPECT(Sub.Equals(TEXT("World")));
    }

    TEST_SECTION(TEXT("StartsWith / EndsWith / Equals"));
    {
        FStringView View(TEXT("Hello, World"));
        TEST_EXPECT(View.StartsWith(TEXT("Hello")));
        TEST_EXPECT(View.EndsWith(TEXT("World")));
        TEST_EXPECT(!View.StartsWith(TEXT("World")));
        TEST_EXPECT(View.Equals(TEXT("Hello, World")));
        TEST_EXPECT(!View.Equals(TEXT("Goodbye")));
    }

    TEST_SECTION(TEXT("Trim / Compare (case-sensitive + NoCase) / CopyToBuffer"));
    {
        FStringView Padded = TEXT("    Trimmable String    ");
        TEST_EXPECT(Padded.Trim().Equals(TEXT("Trimmable String")));

        FStringView Upper = FStringView(TEXT("COMPAREPostfix"), 7);
        FStringView Lower = FStringView(TEXT("comparePostfix"), 7);
        TEST_EXPECT(Upper.Compare(Lower) != 0);
        TEST_EXPECT_EQ(Upper.Compare(Lower, EStringCaseType::NoCase), 0);

        FStringView View(TEXT("This is a long string"));
        CHAR_T Buffer[6] = { };
        View.CopyToBuffer(Buffer, 5, 3);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Buffer, TEXT("s is ")) == 0);
    }

    TEST_SECTION(TEXT("FindCharWithPredicate / FindLast / comparison operators"));
    {
        FStringView Search = TEXT("0123MeSearch89Me89");
        
        TEST_EXPECT_EQ(Search.FindCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);
        
        TEST_EXPECT_EQ(Search.FindLastCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'h') || (Ch == 'M') || (Ch == 'c');
        }), 14);

        TEST_EXPECT_EQ(Search.FindLast(TEXT("Me")), 14);

        FStringView Test = TEXT("Test");
        TEST_EXPECT((TEXT("Test") == Test));
        TEST_EXPECT((Test == TEXT("Test")));
        TEST_EXPECT(!(TEXT("Test") != Test));
        TEST_EXPECT((TEXT("Test") <= Test));
        TEST_EXPECT(!(TEXT("Test") < Test));
        TEST_EXPECT((TEXT("Test") >= Test));
        TEST_EXPECT(!(TEXT("Test") > Test));
    }

    TEST_SECTION(TEXT("FWideStringView"));
    {
        const WIDECHAR* LongString = L"This is a long string";
        FWideStringView Empty;
        TEST_EXPECT(Empty.IsEmpty());

        FWideStringView View = LongString;
        TEST_EXPECT(View.Equals(L"This is a long string"));

        FWideStringView Slice = FWideStringView(LongString, 4, 5);
        TEST_EXPECT(Slice.Equals(L"is a"));

        FWideStringView Padded = L"    Trimmable String    ";
        TEST_EXPECT(Padded.Trim().Equals(L"Trimmable String"));

        TEST_EXPECT_EQ(View.Find(L"long"), 10);
        TEST_EXPECT(View.StartsWith(L"This"));
        TEST_EXPECT(View.EndsWith(L"string"));
    }

    TEST_END();
}
#endif
