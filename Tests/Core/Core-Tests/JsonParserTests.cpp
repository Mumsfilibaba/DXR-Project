#include "JsonParserTests.h"

#include <Core/Json/Json.h>
#include <Core/Containers/String.h>
#include <Core/Containers/StringView.h>
#include <Core/Templates/NumericLimits.h>

#include "TestCommon/TestMacros.h"

static bool ParseOk(const CHAR_T* Text, FJsonValue& OutValue, EJsonParseFlags Flags = EJsonParseFlags::None)
{
    FJsonError Error;
    if (Json::Parse(FStringView(Text), OutValue, &Error, Flags))
    {
        return true;
    }

    LOG_ERROR("[FAIL] unexpected parse failure on '%s' : %s", Text, Error.ToString().Data());
    return false;
}

static bool ParseFails(const CHAR_T* Text, EJsonParseFlags Flags = EJsonParseFlags::None)
{
    FJsonValue Value;
    return !Json::Parse(FStringView(Text), Value, nullptr, Flags);
}

static bool FailsAt(const CHAR_T* Text, int32 ExpectedLine, int32 ExpectedColumn, EJsonParseFlags Flags = EJsonParseFlags::None)
{
    FJsonValue Value;
    FJsonError Error;
    if (Json::Parse(FStringView(Text), Value, &Error, Flags))
    {
        LOG_ERROR("[FAIL] expected a parse failure on '%s'", Text);
        return false;
    }

    if ((Error.Line != ExpectedLine) || (Error.Column != ExpectedColumn))
    {
        LOG_ERROR("[FAIL] expected failure at line %d column %d but got line %d column %d (%s)",
            ExpectedLine, ExpectedColumn, Error.Line, Error.Column, Error.Message.Data());
        return false;
    }

    return true;
}

static bool ParsesToString(const CHAR_T* Text, const CHAR_T* Expected)
{
    FJsonValue Value;
    if (!ParseOk(Text, Value))
    {
        return false;
    }

    FString Parsed;
    return Value.TryGetString(Parsed) && Parsed.Equals(Expected);
}

// Builds "[[[ ... ]]]" nested Depth levels deep
static FString MakeNestedArrays(int32 Depth)
{
    FString Text;
    Text.Reserve(Depth * 2);

    for (int32 Index = 0; Index < Depth; ++Index)
    {
        Text.Append('[');
    }

    for (int32 Index = 0; Index < Depth; ++Index)
    {
        Text.Append(']');
    }

    return Text;
}

bool JsonParser_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Every value type at the document root"));
    {
        FJsonValue Value;

        TEST_EXPECT(ParseOk(TEXT("null"), Value) && Value.IsNull());
        TEST_EXPECT(ParseOk(TEXT("true"), Value) && Value.IsBool() && Value.GetBoolOr(false));
        TEST_EXPECT(ParseOk(TEXT("false"), Value) && Value.IsBool() && !Value.GetBoolOr(true));
        TEST_EXPECT(ParseOk(TEXT("42"), Value) && Value.IsNumber() && Value.IsIntegral());
        TEST_EXPECT(ParseOk(TEXT("4.5"), Value) && Value.IsNumber() && !Value.IsIntegral());
        TEST_EXPECT(ParseOk(TEXT("\"text\""), Value) && Value.IsString());
        TEST_EXPECT(ParseOk(TEXT("[]"), Value) && Value.IsArray() && (Value.Num() == 0));
        TEST_EXPECT(ParseOk(TEXT("{}"), Value) && Value.IsObject() && (Value.NumMembers() == 0));
    }

    TEST_SECTION(TEXT("Objects keep their members in the order they were written"));
    {
        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("{ \"Zebra\": 1, \"Apple\": 2, \"Mango\": 3 }"), Value));
        TEST_EXPECT(Value.NumMembers() == 3);
        TEST_EXPECT(Value.GetMemberName(0).Equals(TEXT("Zebra")));
        TEST_EXPECT(Value.GetMemberName(1).Equals(TEXT("Apple")));
        TEST_EXPECT(Value.GetMemberName(2).Equals(TEXT("Mango")));
    }

    TEST_SECTION(TEXT("Nesting several levels deep"));
    {
        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("{\"a\":[{\"b\":[1,{\"c\":[[2]]}]}]}"), Value));

        const FJsonValue* A = Value.Find(TEXT("a"));
        TEST_EXPECT(A && A->IsArray() && (A->Num() == 1));

        if (A && A->IsArray() && (A->Num() == 1))
        {
            const FJsonValue* B = (*A)[0].Find(TEXT("b"));
            TEST_EXPECT(B && B->IsArray() && (B->Num() == 2));

            if (B && B->IsArray() && (B->Num() == 2))
            {
                const FJsonValue* C = (*B)[1].Find(TEXT("c"));
                TEST_EXPECT(C && C->IsArray() && (C->Num() == 1));
                TEST_EXPECT(C && (*C)[0].IsArray() && ((*C)[0][0].GetInt64Or(0) == 2));
            }
        }
    }

    TEST_SECTION(TEXT("Integers stay integers and reals stay reals"));
    {
        FJsonValue Value;

        TEST_EXPECT(ParseOk(TEXT("0"), Value) && Value.IsIntegral() && (Value.GetInt64Or(-1) == 0));
        TEST_EXPECT(ParseOk(TEXT("-0"), Value) && Value.IsIntegral() && (Value.GetInt64Or(-1) == 0));
        TEST_EXPECT(ParseOk(TEXT("-17"), Value) && Value.IsIntegral() && (Value.GetInt64Or(0) == -17));
        TEST_EXPECT(ParseOk(TEXT("1e10"), Value) && !Value.IsIntegral() && (Value.GetDoubleOr(0.0) == 1e10));
        TEST_EXPECT(ParseOk(TEXT("1E+10"), Value) && !Value.IsIntegral() && (Value.GetDoubleOr(0.0) == 1e10));
        TEST_EXPECT(ParseOk(TEXT("-2.5e-3"), Value) && !Value.IsIntegral() && (Value.GetDoubleOr(0.0) == -2.5e-3));
        TEST_EXPECT(ParseOk(TEXT("2.0"), Value) && !Value.IsIntegral());
    }

    TEST_SECTION(TEXT("The int64 extremes survive exactly"));
    {
        FJsonValue Value;

        TEST_EXPECT(ParseOk(TEXT("9223372036854775807"), Value));
        TEST_EXPECT(Value.IsIntegral() && (Value.GetInt64Or(0) == TNumericLimits<int64>::Max()));

        TEST_EXPECT(ParseOk(TEXT("-9223372036854775808"), Value));
        TEST_EXPECT(Value.IsIntegral() && (Value.GetInt64Or(0) == TNumericLimits<int64>::Min()));

        // 2^53 + 1 is the first integer a double cannot represent, and it has to stay exact
        TEST_EXPECT(ParseOk(TEXT("9007199254740993"), Value));
        TEST_EXPECT(Value.IsIntegral() && (Value.GetInt64Or(0) == 9007199254740993LL));

        // Past int64 there is nowhere exact to put it, so it becomes a real rather than wrapping
        TEST_EXPECT(ParseOk(TEXT("99999999999999999999"), Value));
        TEST_EXPECT(Value.IsNumber() && !Value.IsIntegral());
    }

    TEST_SECTION(TEXT("Number grammar rejections from the RFC"));
    {
        TEST_EXPECT(ParseFails(TEXT("+1")));
        TEST_EXPECT(ParseFails(TEXT("01")));
        TEST_EXPECT(ParseFails(TEXT("-01")));
        TEST_EXPECT(ParseFails(TEXT("007")));
        TEST_EXPECT(ParseFails(TEXT(".5")));
        TEST_EXPECT(ParseFails(TEXT("5.")));
        TEST_EXPECT(ParseFails(TEXT("0x10")));
        TEST_EXPECT(ParseFails(TEXT("1e")));
        TEST_EXPECT(ParseFails(TEXT("1e+")));
        TEST_EXPECT(ParseFails(TEXT("-")));
        TEST_EXPECT(ParseFails(TEXT("1.2.3")));
        TEST_EXPECT(ParseFails(TEXT("Infinity")));
        TEST_EXPECT(ParseFails(TEXT("NaN")));
    }

    TEST_SECTION(TEXT("Every two-character escape"));
    {
        TEST_EXPECT(ParsesToString(TEXT("\"a\\\"b\""), TEXT("a\"b")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\\\b\""), TEXT("a\\b")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\/b\""), TEXT("a/b")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\bb\""), TEXT("a\bb")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\fb\""), TEXT("a\fb")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\nb\""), TEXT("a\nb")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\rb\""), TEXT("a\rb")));
        TEST_EXPECT(ParsesToString(TEXT("\"a\\tb\""), TEXT("a\tb")));
        TEST_EXPECT(ParseFails(TEXT("\"a\\qb\"")));
    }

    TEST_SECTION(TEXT("Unicode escapes decode to UTF-8"));
    {
        TEST_EXPECT(ParsesToString(TEXT("\"\\u0041\""), TEXT("A")));

        // U+00E9 becomes two bytes, U+20AC three
        TEST_EXPECT(ParsesToString(TEXT("\"\\u00e9\""), TEXT("\xC3\xA9")));
        TEST_EXPECT(ParsesToString(TEXT("\"\\u20AC\""), TEXT("\xE2\x82\xAC")));

        // U+1F600, written as a surrogate pair, becomes four
        TEST_EXPECT(ParsesToString(TEXT("\"\\uD83D\\uDE00\""), TEXT("\xF0\x9F\x98\x80")));

        TEST_EXPECT(ParseFails(TEXT("\"\\uD83D\"")));
        TEST_EXPECT(ParseFails(TEXT("\"\\uDE00\"")));
        TEST_EXPECT(ParseFails(TEXT("\"\\uD83D\\u0041\"")));
        TEST_EXPECT(ParseFails(TEXT("\"\\u00G1\"")));
        TEST_EXPECT(ParseFails(TEXT("\"\\u12\"")));
    }

    TEST_SECTION(TEXT("Raw control characters are rejected inside strings"));
    {
        TEST_EXPECT(ParseFails(TEXT("\"line\nbreak\"")));
        TEST_EXPECT(ParseFails(TEXT("\"tab\there\"")));
    }

    TEST_SECTION(TEXT("Non-ASCII UTF-8 passes through untouched"));
    {
        TEST_EXPECT(ParsesToString(TEXT("\"Sm\xC3\xB6rg\xC3\xA5sbord\""), TEXT("Sm\xC3\xB6rg\xC3\xA5sbord")));
    }

    TEST_SECTION(TEXT("Malformed input reports the exact position"));
    {
        TEST_EXPECT(FailsAt(TEXT(""), 1, 1));
        TEST_EXPECT(FailsAt(TEXT("   "), 1, 4));
        TEST_EXPECT(FailsAt(TEXT("{\"a\" 1}"), 1, 6));
        TEST_EXPECT(FailsAt(TEXT("{\"a\":1 \"b\":2}"), 1, 8));
        TEST_EXPECT(FailsAt(TEXT("[1,2"), 1, 5));
        TEST_EXPECT(FailsAt(TEXT("{\"a\":1"), 1, 7));
        TEST_EXPECT(FailsAt(TEXT("\"unterminated"), 1, 14));
        TEST_EXPECT(FailsAt(TEXT("[1,]"), 1, 4));
        TEST_EXPECT(FailsAt(TEXT("{\"a\":1,}"), 1, 8));
        TEST_EXPECT(FailsAt(TEXT("[1,,2]"), 1, 4));
        TEST_EXPECT(FailsAt(TEXT("{1:2}"), 1, 2));
        TEST_EXPECT(FailsAt(TEXT("{} junk"), 1, 4));
    }

    TEST_SECTION(TEXT("Positions count lines through a multi-line document"));
    {
        TEST_EXPECT(FailsAt(TEXT("{\n    \"a\": 1,\n    \"b\" 2\n}"), 3, 9));
        TEST_EXPECT(FailsAt(TEXT("[\n  1,\n  2\n  3\n]"), 4, 3));
    }

    TEST_SECTION(TEXT("Nesting past the depth limit fails instead of overflowing the stack"));
    {
        FJsonValue Value;

        const FString JustUnder = MakeNestedArrays(JSON_DEFAULT_MAX_DEPTH);
        TEST_EXPECT(Json::Parse(FStringView(JustUnder), Value));

        const FString WayOver = MakeNestedArrays(JSON_DEFAULT_MAX_DEPTH + 5000);
        TEST_EXPECT(!Json::Parse(FStringView(WayOver), Value));
    }

    TEST_SECTION(TEXT("Comments are rejected by default and accepted with the flag"));
    {
        const CHAR_T* WithLineComment  = TEXT("{\n    // a note\n    \"a\": 1\n}");
        const CHAR_T* WithBlockComment = TEXT("{ /* a note */ \"a\": 1 }");

        TEST_EXPECT(ParseFails(WithLineComment));
        TEST_EXPECT(ParseFails(WithBlockComment));

        FJsonValue Value;
        TEST_EXPECT(ParseOk(WithLineComment, Value, EJsonParseFlags::AllowComments));
        TEST_EXPECT(Value.NumMembers() == 1);

        TEST_EXPECT(ParseOk(WithBlockComment, Value, EJsonParseFlags::AllowComments));
        TEST_EXPECT(Value.NumMembers() == 1);

        TEST_EXPECT(ParseFails(TEXT("{ /* unterminated \"a\": 1 }"), EJsonParseFlags::AllowComments));
    }

    TEST_SECTION(TEXT("Trailing commas are rejected by default and accepted with the flag"));
    {
        TEST_EXPECT(ParseFails(TEXT("[1,2,]")));
        TEST_EXPECT(ParseFails(TEXT("{\"a\":1,}")));

        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("[1,2,]"), Value, EJsonParseFlags::AllowTrailingCommas) && (Value.Num() == 2));
        TEST_EXPECT(ParseOk(TEXT("{\"a\":1,}"), Value, EJsonParseFlags::AllowTrailingCommas) && (Value.NumMembers() == 1));

        // Two commas in a row is still a mistake, not a trailing comma
        TEST_EXPECT(ParseFails(TEXT("[1,,]"), EJsonParseFlags::AllowTrailingCommas));
    }

    TEST_SECTION(TEXT("Both leniencies together"));
    {
        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("{\n  // hand edited\n  \"a\": [1, 2,],\n}"), Value, EJsonParseFlags::Relaxed));
        TEST_EXPECT(Value.NumMembers() == 1);
    }

    TEST_SECTION(TEXT("A UTF-8 BOM is stripped and does not shift the reported column"));
    {
        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("\xEF\xBB\xBF{\"a\":1}"), Value) && (Value.NumMembers() == 1));
        TEST_EXPECT(FailsAt(TEXT("\xEF\xBB\xBF{\"a\" 1}"), 1, 6));
    }

    TEST_SECTION(TEXT("CRLF input parses the same as LF"));
    {
        FJsonValue Unix;
        FJsonValue Windows;
        TEST_EXPECT(ParseOk(TEXT("{\n    \"a\": 1,\n    \"b\": [2, 3]\n}"), Unix));
        TEST_EXPECT(ParseOk(TEXT("{\r\n    \"a\": 1,\r\n    \"b\": [2, 3]\r\n}"), Windows));
        TEST_EXPECT(Unix.Equals(Windows));
    }

    TEST_SECTION(TEXT("A repeated member keeps the last value, as every reader does"));
    {
        FJsonValue Value;
        TEST_EXPECT(ParseOk(TEXT("{\"a\":1,\"a\":2}"), Value));
        TEST_EXPECT(Value.NumMembers() == 1);

        const FJsonValue* Found = Value.Find(TEXT("a"));
        TEST_EXPECT(Found && (Found->GetInt64Or(0) == 2));
    }

    TEST_SECTION(TEXT("A view without a null terminator stops where it is told to"));
    {
        const CHAR_T* Backing = TEXT("[1,2,3]garbage");

        FJsonValue Value;
        TEST_EXPECT(Json::Parse(FStringView(Backing, 7), Value));
        TEST_EXPECT(Value.IsArray() && (Value.Num() == 3));
    }

    TEST_END();
}
