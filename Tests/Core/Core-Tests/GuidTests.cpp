#include "GuidTests.h"

#include <Core/Misc/Guid.h>
#include <Core/Json/Json.h>
#include <Core/Json/JsonArchive.h>
#include <Core/Json/JsonGuid.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/String.h>

#include "TestCommon/TestMacros.h"

struct FTestAssetReference
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field("Guid", Guid, FGuid());
    }

    FGuid Guid;
};

bool Guid_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A default GUID is invalid");
    {
        FGuid Guid;
        TEST_EXPECT(!Guid.IsValid());
        TEST_EXPECT(Guid.ToString().Equals("00000000000000000000000000000000"));
    }

    TEST_SECTION("Created GUIDs are valid, random version 4 and unique");
    {
        TMap<FGuid, int32> Seen;
        for (int32 Index = 0; Index < 1000; ++Index)
        {
            const FGuid  Guid = FGuid::Create();
            const String Text = Guid.ToString();
            TEST_EXPECT(Guid.IsValid());

            TEST_EXPECT(Text[12] == '4');
            TEST_EXPECT((Text[16] == '8') || (Text[16] == '9') || (Text[16] == 'a') || (Text[16] == 'b'));

            TEST_EXPECT(!Seen.Contains(Guid));
            Seen.Add(Guid, Index);
        }

        TEST_EXPECT(Seen.Size() == 1000);
    }

    TEST_SECTION("ToString writes 32 lowercase hex digits, A first");
    {
        const FGuid Guid(0x6F9619FF, 0x8B86D011, 0xB42D00CF, 0x4FC964FF);
        TEST_EXPECT(Guid.ToString().Equals("6f9619ff8b86d011b42d00cf4fc964ff"));
    }

    TEST_SECTION("Parse reads the packed and dashed forms in either case");
    {
        const FGuid Expected(0x6F9619FF, 0x8B86D011, 0xB42D00CF, 0x4FC964FF);

        FGuid Packed;
        TEST_EXPECT(FGuid::Parse("6f9619ff8b86d011b42d00cf4fc964ff", Packed));
        TEST_EXPECT(Packed == Expected);

        FGuid Dashed;
        TEST_EXPECT(FGuid::Parse("6F9619FF-8B86-D011-B42D-00CF4FC964FF", Dashed));
        TEST_EXPECT(Dashed == Expected);

        const FGuid Created = FGuid::Create();
        FGuid RoundTripped;
        TEST_EXPECT(FGuid::Parse(Created.ToString(), RoundTripped));
        TEST_EXPECT(RoundTripped == Created);
    }

    TEST_SECTION("Parse rejects anything that is not a GUID and leaves the output alone");
    {
        const FGuid Original(1, 2, 3, 4);
        FGuid Guid = Original;

        TEST_EXPECT(!FGuid::Parse("", Guid));
        TEST_EXPECT(!FGuid::Parse("6f9619ff8b86d011b42d00cf4fc964f", Guid));
        TEST_EXPECT(!FGuid::Parse("6f9619ff8b86d011b42d00cf4fc964fff", Guid));
        TEST_EXPECT(!FGuid::Parse("6f9619ff8b86d011b42d00cf4fc964fg", Guid));
        TEST_EXPECT(!FGuid::Parse("6F9619FF-8B86-D011-B42D-00CF4FC964F", Guid));
        TEST_EXPECT(!FGuid::Parse("6F9619FF8-B86-D011-B42D-00CF4FC964FF", Guid));
        TEST_EXPECT(!FGuid::Parse("{6F9619FF-8B86-D011-B42D-00CF4FC964FF}", Guid));
        TEST_EXPECT(Guid == Original);
    }

    TEST_SECTION("Comparison orders GUIDs like their strings");
    {
        const FGuid Low(0x00000001, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
        const FGuid High(0x00000002, 0x00000000, 0x00000000, 0x00000000);
        TEST_EXPECT(Low < High);
        TEST_EXPECT(!(High < Low));
        TEST_EXPECT(Low.ToString().Compare(High.ToString()) < 0);
        TEST_EXPECT(Low != High);
    }

    TEST_SECTION("A GUID round trips through a JSON archive as a string");
    {
        FTestAssetReference Source;
        Source.Guid = FGuid(0x6F9619FF, 0x8B86D011, 0xB42D00CF, 0x4FC964FF);

        FJsonValue Document;
        FJsonArchive Saver = FJsonArchive::Saver(Document);
        Source.Serialize(Saver);

        const FJsonValue* Written = Document.Find("Guid");
        TEST_EXPECT(Written && Written->IsString());
        TEST_EXPECT(Written && Written->GetStringOr("").Equals("6f9619ff8b86d011b42d00cf4fc964ff"));

        FTestAssetReference Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);
        TEST_EXPECT(!Loader.HasErrors());
        TEST_EXPECT(Loaded.Guid == Source.Guid);

        FTestAssetReference Empty;
        FJsonValue EmptyDocument;
        FJsonArchive EmptySaver = FJsonArchive::Saver(EmptyDocument);
        Empty.Serialize(EmptySaver);
        TEST_EXPECT(EmptyDocument.Find("Guid") == nullptr);
    }

    TEST_SECTION("Loading a malformed GUID reports an error");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Guid\": \"not-a-guid\"}"), Document));

        FTestAssetReference Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);
        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(!Loaded.Guid.IsValid());
    }

    TEST_END();
}
