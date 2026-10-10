#include "StringView_Test.h"

#if RUN_STRINGVIEW_SUITE
#include "TestUtils.h"

#include <Core/Containers/StringView.h>

bool StringView_Suite()
{
    TEST_BEGIN();

    TEST_SECTION("Construction / indexing / Length / IsEmpty");
    {
        FStringView Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.Length(), 0);
        TEST_EXPECT_EQ(Empty.LastIndex(), FStringView::InvalidIndex);

        FStringView View("Hello, World");
        TEST_EXPECT_EQ(View.Length(), 12);
        TEST_EXPECT(!View.IsEmpty());
        TEST_EXPECT_EQ(View[0], 'H');
        TEST_EXPECT_EQ(View[11], 'd');
        TEST_EXPECT_EQ(View.LastIndex(), 11);

        const CHAR_T* LongString = "This is a long string";
        FStringView Slice = FStringView(LongString, 4, 5);
        TEST_EXPECT(Slice.Equals("is a"));

        FStringView Prefix = FStringView("COMPAREPostfix", 7);
        TEST_EXPECT(Prefix.Equals("COMPARE"));
    }

    TEST_SECTION("Find / FindChar / Contains / SubString");
    {
        FStringView View("Hello, World");
        TEST_EXPECT_EQ(View.FindChar('W'), 7);
        TEST_EXPECT_EQ(View.FindChar('z'), FStringView::InvalidIndex);
        TEST_EXPECT_EQ(View.Find("World"), 7);
        TEST_EXPECT_EQ(View.Find("xyz"), FStringView::InvalidIndex);

        FStringView Sub = View.SubString(7, 5);
        TEST_EXPECT_EQ(Sub.Length(), 5);
        TEST_EXPECT(Sub.Equals("World"));
    }

    TEST_SECTION("StartsWith / EndsWith / Equals");
    {
        FStringView View("Hello, World");
        TEST_EXPECT(View.StartsWith("Hello"));
        TEST_EXPECT(View.EndsWith("World"));
        TEST_EXPECT(!View.StartsWith("World"));
        TEST_EXPECT(View.Equals("Hello, World"));
        TEST_EXPECT(!View.Equals("Goodbye"));
    }

    TEST_SECTION("Trim / Compare (case-sensitive + NoCase) / CopyToBuffer");
    {
        FStringView Padded = "    Trimmable String    ";
        TEST_EXPECT(Padded.Trim().Equals("Trimmable String"));

        FStringView Upper = FStringView("COMPAREPostfix", 7);
        FStringView Lower = FStringView("comparePostfix", 7);
        TEST_EXPECT(Upper.Compare(Lower) != 0);
        TEST_EXPECT_EQ(Upper.Compare(Lower, EStringCaseType::NoCase), 0);

        FStringView View("This is a long string");
        CHAR_T Buffer[6] = { };
        View.CopyToBuffer(Buffer, 5, 3);
        TEST_EXPECT(TCString<CHAR_T>::Strcmp(Buffer, "s is ") == 0);
    }

    TEST_SECTION("FindCharWithPredicate / FindLast / comparison operators");
    {
        FStringView Search = "0123MeSearch89Me89";
        
        TEST_EXPECT_EQ(Search.FindCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);
        
        TEST_EXPECT_EQ(Search.FindLastCharWithPredicate([](CHAR_T Ch)
        {
            return (Ch == 'h') || (Ch == 'M') || (Ch == 'c');
        }), 14);

        TEST_EXPECT_EQ(Search.FindLast("Me"), 14);

        FStringView Test = "Test";
        TEST_EXPECT(("Test" == Test));
        TEST_EXPECT((Test == "Test"));
        TEST_EXPECT(!("Test" != Test));
        TEST_EXPECT(("Test" <= Test));
        TEST_EXPECT(!("Test" < Test));
        TEST_EXPECT(("Test" >= Test));
        TEST_EXPECT(!("Test" > Test));
    }

    TEST_SECTION("WStringView");
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
