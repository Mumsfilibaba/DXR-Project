#include "JsonWriterTests.h"

#include <Core/Json/Json.h>
#include <Core/Json/JsonSerializer.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/String.h>
#include <Core/Containers/StringView.h>
#include <Core/Math/Math.h>
#include <Core/Memory/Memory.h>
#include <Core/Templates/NumericLimits.h>

#include "TestCommon/TestMacros.h"

// The corpus that the round-trip and determinism checks run over
static const CHAR_T* GDocuments[] =
{
    TEXT("null"),
    TEXT("true"),
    TEXT("false"),
    TEXT("0"),
    TEXT("-1"),
    TEXT("3.25"),
    TEXT("\"\""),
    TEXT("\"text with spaces\""),
    TEXT("[]"),
    TEXT("{}"),
    TEXT("[1, 2, 3]"),
    TEXT("[[1, 2], [3, 4]]"),
    TEXT("{\"a\": 1, \"b\": [true, null], \"c\": {\"d\": \"e\"}}"),
    TEXT("{\"Version\": 1, \"Actors\": [{\"Type\": \"FPointLightActor\", \"Translation\": [15, 2.5, 0]}]}"),
    TEXT("[{\"nested\": {\"deeper\": {\"deepest\": [1, 2, 3]}}}]"),
};

static FString WriteValue(const FJsonValue& Value, EJsonWriteFlags Flags = EJsonWriteFlags::Pretty)
{
    return Json::ToString(Value, Flags);
}

static bool RoundTripsDouble(double Value)
{
    const FString Text = WriteValue(FJsonValue(Value), EJsonWriteFlags::Compact);

    FJsonValue Parsed;
    if (!Json::Parse(FStringView(Text), Parsed))
    {
        LOG_ERROR("[FAIL] '%s' did not parse back", Text.Data());
        return false;
    }

    const double Result = Parsed.GetDoubleOr(0.0);
    if (Result != Value)
    {
        LOG_ERROR("[FAIL] '%s' read back as a different value", Text.Data());
        return false;
    }

    return true;
}

static bool RoundTripsFloat(float Value)
{
    const FString Text = WriteValue(FJsonValue(Value), EJsonWriteFlags::Compact);

    FJsonValue Parsed;
    if (!Json::Parse(FStringView(Text), Parsed))
    {
        LOG_ERROR("[FAIL] '%s' did not parse back", Text.Data());
        return false;
    }

    const float Result = static_cast<float>(Parsed.GetDoubleOr(0.0));
    if (Result != Value)
    {
        LOG_ERROR("[FAIL] '%s' read back as a different float", Text.Data());
        return false;
    }

    return true;
}

static bool WritesAs(const FJsonValue& Value, const CHAR_T* Expected, EJsonWriteFlags Flags = EJsonWriteFlags::Compact)
{
    const FString Text = WriteValue(Value, Flags);
    if (!Text.Equals(Expected))
    {
        LOG_ERROR("[FAIL] expected '%s' but wrote '%s'", Expected, Text.Data());
        return false;
    }

    return true;
}

bool JsonWriter_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Parse, write and parse again produces an equal document"));
    {
        for (const CHAR_T* Document : GDocuments)
        {
            FJsonValue First;
            if (!Json::Parse(FStringView(Document), First))
            {
                LOG_ERROR("[FAIL] the corpus entry '%s' did not parse", Document);
                _bTestPassed = false;
                continue;
            }

            FJsonValue Second;
            const FString Text = WriteValue(First);
            TEST_EXPECT(Json::Parse(FStringView(Text), Second));
            TEST_EXPECT(First.Equals(Second));
        }
    }

    TEST_SECTION(TEXT("Writing the same document twice is byte-identical"));
    {
        for (const CHAR_T* Document : GDocuments)
        {
            FJsonValue Value;
            if (!Json::Parse(FStringView(Document), Value))
            {
                continue;
            }

            TEST_EXPECT(WriteValue(Value).Equals(WriteValue(Value)));
        }
    }

    TEST_SECTION(TEXT("Output never contains a carriage return"));
    {
        FJsonValue Value;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"a\":[1,2],\"b\":{\"c\":3}}")), Value));

        const FString Text = WriteValue(Value);
        TEST_EXPECT(!Text.Contains('\r'));
        TEST_EXPECT(Text.Contains('\n'));
    }

    TEST_SECTION(TEXT("Floats survive the round trip bit-exactly"));
    {
        TEST_EXPECT(RoundTripsFloat(0.1f));
        TEST_EXPECT(RoundTripsFloat(-0.1f));
        TEST_EXPECT(RoundTripsFloat(0.0f));
        TEST_EXPECT(RoundTripsFloat(-0.0f));
        TEST_EXPECT(RoundTripsFloat(1.0f));
        TEST_EXPECT(RoundTripsFloat(0.3333333f));
        TEST_EXPECT(RoundTripsFloat(1e-30f));
        TEST_EXPECT(RoundTripsFloat(1e30f));
        TEST_EXPECT(RoundTripsFloat(TNumericLimits<float>::Max()));
        TEST_EXPECT(RoundTripsFloat(-TNumericLimits<float>::Max()));
        TEST_EXPECT(RoundTripsFloat(1.17549435e-38f));
        TEST_EXPECT(RoundTripsFloat(1.0e-45f));
        TEST_EXPECT(RoundTripsFloat(16777217.0f));
    }

    TEST_SECTION(TEXT("Doubles survive the round trip bit-exactly"));
    {
        TEST_EXPECT(RoundTripsDouble(0.1));
        TEST_EXPECT(RoundTripsDouble(1.0 / 3.0));
        TEST_EXPECT(RoundTripsDouble(2.2250738585072014e-308));
        TEST_EXPECT(RoundTripsDouble(1.7976931348623157e308));
        TEST_EXPECT(RoundTripsDouble(-2.5e-3));
        TEST_EXPECT(RoundTripsDouble(9007199254740993.0));
    }

    TEST_SECTION(TEXT("A float prints at float precision rather than its double expansion"));
    {
        TEST_EXPECT(WritesAs(FJsonValue(0.1f), TEXT("0.1")));
        TEST_EXPECT(WritesAs(FJsonValue(0.5f), TEXT("0.5")));
        TEST_EXPECT(WritesAs(FJsonValue(2.5f), TEXT("2.5")));
        TEST_EXPECT(WritesAs(FJsonValue(-3.0f), TEXT("-3")));
        TEST_EXPECT(WritesAs(FJsonValue(1.0f), TEXT("1")));
    }

    TEST_SECTION(TEXT("Whole numbers stay readable instead of turning into exponents"));
    {
        TEST_EXPECT(WritesAs(FJsonValue(5000.0), TEXT("5000")));
        TEST_EXPECT(WritesAs(FJsonValue(100000.0), TEXT("100000")));
        TEST_EXPECT(WritesAs(FJsonValue(0.0001), TEXT("0.0001")));
        TEST_EXPECT(WritesAs(FJsonValue(-0.0), TEXT("-0")));
    }

    TEST_SECTION(TEXT("An int64 past 2^53 stays exact and stays out of exponent form"));
    {
        TEST_EXPECT(WritesAs(FJsonValue(static_cast<int64>(9007199254740993LL)), TEXT("9007199254740993")));
        TEST_EXPECT(WritesAs(FJsonValue(TNumericLimits<int64>::Max()), TEXT("9223372036854775807")));
        TEST_EXPECT(WritesAs(FJsonValue(TNumericLimits<int64>::Min()), TEXT("-9223372036854775808")));
        TEST_EXPECT(WritesAs(FJsonValue(static_cast<int64>(1000000000000LL)), TEXT("1000000000000")));

        FJsonValue Parsed;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("9007199254740993")), Parsed));
        TEST_EXPECT(Parsed.GetInt64Or(0) == 9007199254740993LL);
    }

    TEST_SECTION(TEXT("Values JSON cannot spell are written as null"));
    {
        // Built from their bit patterns rather than written as expressions, because the engine
        // compiles with fast floating point and an arithmetic NaN is exactly what it may fold away
        const uint64 NotANumberBits = 0x7FF8000000000000ULL;
        const uint64 InfinityBits   = 0x7FF0000000000000ULL;

        double NotANumber = 0.0;
        double Infinity   = 0.0;
        Memory::Memcpy(&NotANumber, &NotANumberBits, sizeof(double));
        Memory::Memcpy(&Infinity, &InfinityBits, sizeof(double));

        TEST_EXPECT(Math::IsNaN(NotANumber));
        TEST_EXPECT(Math::IsInfinity(Infinity));

        TEST_EXPECT(WritesAs(FJsonValue(NotANumber), TEXT("null")));
        TEST_EXPECT(WritesAs(FJsonValue(Infinity), TEXT("null")));
        TEST_EXPECT(WritesAs(FJsonValue(-Infinity), TEXT("null")));
        TEST_EXPECT(WritesAs(FJsonValue(static_cast<float>(NotANumber)), TEXT("null")));
        TEST_EXPECT(WritesAs(FJsonValue(static_cast<float>(Infinity)), TEXT("null")));
    }

    TEST_SECTION(TEXT("Escaping produces exactly the expected literal text"));
    {
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("plain")), TEXT("\"plain\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\"b")), TEXT("\"a\\\"b\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\\b")), TEXT("\"a\\\\b\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\nb")), TEXT("\"a\\nb\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\tb")), TEXT("\"a\\tb\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\rb")), TEXT("\"a\\rb\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\bb")), TEXT("\"a\\bb\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\fb")), TEXT("\"a\\fb\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a\x01\x1F" "b")), TEXT("\"a\\u0001\\u001fb\"")));

        // A forward slash needs no escape, and UTF-8 stays readable rather than becoming \u escapes
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("a/b")), TEXT("\"a/b\"")));
        TEST_EXPECT(WritesAs(FJsonValue(TEXT("Sm\xC3\xB6rg\xC3\xA5s")), TEXT("\"Sm\xC3\xB6rg\xC3\xA5s\"")));
    }

    TEST_SECTION(TEXT("Escapes survive a round trip"));
    {
        const CHAR_T* Awkward = TEXT("quote \" backslash \\ newline \n tab \t slash / unicode \xE2\x82\xAC");

        FJsonValue Parsed;
        TEST_EXPECT(Json::Parse(FStringView(WriteValue(FJsonValue(Awkward))), Parsed));

        FString Result;
        TEST_EXPECT(Parsed.TryGetString(Result) && Result.Equals(Awkward));
    }

    TEST_SECTION(TEXT("Scalar arrays stay on one line and other arrays do not"));
    {
        FJsonValue Vector = FJsonValue::MakeArray();
        Vector.Add(FJsonValue(0.0f));
        Vector.Add(FJsonValue(2.5f));
        Vector.Add(FJsonValue(-3.0f));

        FJsonValue Root = FJsonValue::MakeObject();
        Root.AddMember(TEXT("Translation"), Move(Vector));

        TEST_EXPECT(WritesAs(Root, TEXT("{\n    \"Translation\": [0, 2.5, -3]\n}\n"), EJsonWriteFlags::Pretty));

        FJsonValue Strings = FJsonValue::MakeArray();
        Strings.Add(FJsonValue(TEXT("a")));
        Strings.Add(FJsonValue(TEXT("b")));

        FJsonValue WithStrings = FJsonValue::MakeObject();
        WithStrings.AddMember(TEXT("Names"), Move(Strings));

        TEST_EXPECT(WritesAs(WithStrings, TEXT("{\n    \"Names\": [\n        \"a\",\n        \"b\"\n    ]\n}\n"), EJsonWriteFlags::Pretty));
    }

    TEST_SECTION(TEXT("Compact output has no whitespace at all"));
    {
        FJsonValue Value;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{ \"a\" : [ 1 , 2 ] , \"b\" : { \"c\" : 3 } }")), Value));
        TEST_EXPECT(WritesAs(Value, TEXT("{\"a\":[1,2],\"b\":{\"c\":3}}")));
    }

    TEST_SECTION(TEXT("Empty containers are written on one line"));
    {
        FJsonValue Root = FJsonValue::MakeObject();
        Root.AddMember(TEXT("Array"), FJsonValue::MakeArray());
        Root.AddMember(TEXT("Object"), FJsonValue::MakeObject());

        TEST_EXPECT(WritesAs(Root, TEXT("{\n    \"Array\": [],\n    \"Object\": {}\n}\n"), EJsonWriteFlags::Pretty));
    }

    TEST_SECTION(TEXT("A map writes its keys in alphabetical order every time"));
    {
        typedef TMap<FString, int32> FLookup;

        FLookup Map;
        Map.Add(FString(TEXT("Zebra")), 1);
        Map.Add(FString(TEXT("apple")), 2);
        Map.Add(FString(TEXT("Mango")), 3);
        Map.Add(FString(TEXT("Banana")), 4);

        FJsonValue Written;
        TJsonSerializer<FLookup>::Save(Written, Map);

        TEST_EXPECT(Written.NumMembers() == 4);
        TEST_EXPECT(Written.GetMemberName(0).Equals(TEXT("Banana")));
        TEST_EXPECT(Written.GetMemberName(1).Equals(TEXT("Mango")));
        TEST_EXPECT(Written.GetMemberName(2).Equals(TEXT("Zebra")));
        TEST_EXPECT(Written.GetMemberName(3).Equals(TEXT("apple")));

        FJsonValue Again;
        TJsonSerializer<FLookup>::Save(Again, Map);
        TEST_EXPECT(WriteValue(Written).Equals(WriteValue(Again)));
    }

    TEST_SECTION(TEXT("Object members keep their order through a round trip"));
    {
        FJsonValue Root = FJsonValue::MakeObject();
        Root.AddMember(TEXT("Zebra"), FJsonValue(1));
        Root.AddMember(TEXT("Apple"), FJsonValue(2));
        Root.AddMember(TEXT("Mango"), FJsonValue(3));

        FJsonValue Parsed;
        TEST_EXPECT(Json::Parse(FStringView(WriteValue(Root)), Parsed));
        TEST_EXPECT(Parsed.GetMemberName(0).Equals(TEXT("Zebra")));
        TEST_EXPECT(Parsed.GetMemberName(1).Equals(TEXT("Apple")));
        TEST_EXPECT(Parsed.GetMemberName(2).Equals(TEXT("Mango")));
    }

    TEST_END();
}
