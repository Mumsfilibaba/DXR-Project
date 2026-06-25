#include "StringView_Test.h"

#if RUN_STRINGVIEW_SUITE
#include "TestUtils.h"

#include <Core/Containers/StringView.h>

bool StringView_Suite()
{
    TEST_BEGIN();

    TEST_SECTION("Construction / indexing / Length / IsEmpty");
    {
        StringView Empty;
        TEST_EXPECT(Empty.IsEmpty());
        TEST_EXPECT_EQ(Empty.Length(), 0);

        StringView View("Hello, World");
        TEST_EXPECT_EQ(View.Length(), 12);
        TEST_EXPECT(!View.IsEmpty());
        TEST_EXPECT_EQ(View[0], 'H');
        TEST_EXPECT_EQ(View[11], 'd');

        const CHAR* LongString = "This is a long string";
        StringView Slice = StringView(LongString, 4, 5);
        TEST_EXPECT(Slice.Equals("is a"));

        StringView Prefix = StringView("COMPAREPostfix", 7);
        TEST_EXPECT(Prefix.Equals("COMPARE"));
    }

    TEST_SECTION("Find / FindChar / Contains / SubString");
    {
        StringView View("Hello, World");
        TEST_EXPECT_EQ(View.FindChar('W'), 7);
        TEST_EXPECT_EQ(View.FindChar('z'), StringView::InvalidIndex);
        TEST_EXPECT_EQ(View.Find("World"), 7);
        TEST_EXPECT_EQ(View.Find("xyz"), StringView::InvalidIndex);

        StringView Sub = View.SubString(7, 5);
        TEST_EXPECT_EQ(Sub.Length(), 5);
        TEST_EXPECT(Sub.Equals("World"));
    }

    TEST_SECTION("StartsWith / EndsWith / Equals");
    {
        StringView View("Hello, World");
        TEST_EXPECT(View.StartsWith("Hello"));
        TEST_EXPECT(View.EndsWith("World"));
        TEST_EXPECT(!View.StartsWith("World"));
        TEST_EXPECT(View.Equals("Hello, World"));
        TEST_EXPECT(!View.Equals("Goodbye"));
    }

    TEST_SECTION("Trim / Compare (case-sensitive + NoCase) / CopyToBuffer");
    {
        StringView Padded = "    Trimmable String    ";
        TEST_EXPECT(Padded.Trim().Equals("Trimmable String"));

        StringView Upper = StringView("COMPAREPostfix", 7);
        StringView Lower = StringView("comparePostfix", 7);
        TEST_EXPECT(Upper.Compare(Lower) != 0);
        TEST_EXPECT_EQ(Upper.Compare(Lower, EStringCaseType::NoCase), 0);

        StringView View("This is a long string");
        CHAR Buffer[6] = { };
        View.CopyToBuffer(Buffer, 5, 3);
        TEST_EXPECT(TCString<CHAR>::Strcmp(Buffer, "s is ") == 0);
    }

    TEST_SECTION("FindCharWithPredicate / FindLast / comparison operators");
    {
        StringView Search = "0123MeSearch89Me89";
        
        TEST_EXPECT_EQ(Search.FindCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'e') || (Ch == 'c');
        }), 5);
        
        TEST_EXPECT_EQ(Search.FindLastCharWithPredicate([](CHAR Ch)
        {
            return (Ch == 'h') || (Ch == 'M') || (Ch == 'c');
        }), 14);

        TEST_EXPECT_EQ(Search.FindLast("Me"), 14);

        StringView Test = "Test";
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
        WStringView Empty;
        TEST_EXPECT(Empty.IsEmpty());

        WStringView View = LongString;
        TEST_EXPECT(View.Equals(L"This is a long string"));

        WStringView Slice = WStringView(LongString, 4, 5);
        TEST_EXPECT(Slice.Equals(L"is a"));

        WStringView Padded = L"    Trimmable String    ";
        TEST_EXPECT(Padded.Trim().Equals(L"Trimmable String"));

        TEST_EXPECT_EQ(View.Find(L"long"), 10);
        TEST_EXPECT(View.StartsWith(L"This"));
        TEST_EXPECT(View.EndsWith(L"string"));
    }

    TEST_END();
}
#endif
