#include "JsonArchiveTests.h"

#include <Core/Json/Json.h>
#include <Core/Json/JsonMath.h>
#include <Core/Json/JsonSerializer.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/Optional.h>
#include <Core/Containers/StaticArray.h>
#include <Core/Containers/String.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

enum class ETestShape : uint8
{
    Box     = 0,
    Sphere  = 1,
    Capsule = 2,
};

JSON_ENUM_NAMES_BEGIN(ETestShape)
    JSON_ENUM_NAME(ETestShape, Box)
    JSON_ENUM_NAME(ETestShape, Sphere)
    JSON_ENUM_NAME(ETestShape, Capsule)
JSON_ENUM_NAMES_END()

enum class ETestPlainEnum : int32
{
    First  = 10,
    Second = 20,
};

typedef TStaticArray<int32, 4> FTestFixedArray;
typedef TMap<String, int32>    FTestLookup;

struct FTestWithOptional
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field("Maybe", Maybe);
    }

    TOptional<int32> Maybe;
};

struct FTestLight
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field("Intensity", Intensity, 1.0f);
        Archive.Field("bCastShadows", bCastShadows, false);
        Archive.Field("Color", Color, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f));
    }

    bool operator==(const FTestLight& Other) const
    {
        return (Intensity == Other.Intensity)
            && (bCastShadows == Other.bCastShadows)
            && (Color.R == Other.Color.R)
            && (Color.G == Other.Color.G)
            && (Color.B == Other.Color.B)
            && (Color.A == Other.Color.A);
    }

    float       Intensity    = 1.0f;
    bool        bCastShadows = false;
    FFloatColor Color        = FFloatColor(1.0f, 1.0f, 1.0f, 1.0f);
};

struct FTestActor
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field("Name", Name, String(""));
        Archive.Field("Count", Count, 0);
        Archive.Field("Shape", Shape, ETestShape::Box);
        Archive.Field("Translation", Translation, Vector3(0.0f, 0.0f, 0.0f));
        Archive.Field("Values", Values);
        Archive.Field("Light", Light, FTestLight());
    }

    bool operator==(const FTestActor& Other) const
    {
        if (!Name.Equals(Other.Name) || (Count != Other.Count) || (Shape != Other.Shape))
        {
            return false;
        }

        if ((Translation.X != Other.Translation.X) || (Translation.Y != Other.Translation.Y) || (Translation.Z != Other.Translation.Z))
        {
            return false;
        }

        if (Values.Size() != Other.Values.Size())
        {
            return false;
        }

        for (int32 Index = 0; Index < Values.Size(); ++Index)
        {
            if (Values[Index] != Other.Values[Index])
            {
                return false;
            }
        }

        return Light == Other.Light;
    }

    String        Name;
    int32         Count = 0;
    ETestShape    Shape = ETestShape::Box;
    Vector3       Translation{ 0.0f, 0.0f, 0.0f };
    TArray<int32> Values;
    FTestLight    Light;
};

static FTestActor MakeActor()
{
    FTestActor Actor;
    Actor.Name        = String("StreetLight_01");
    Actor.Count       = 7;
    Actor.Shape       = ETestShape::Capsule;
    Actor.Translation = Vector3(15.0f, 2.5f, -3.0f);
    Actor.Values      = { 1, 2, 3 };

    Actor.Light.Intensity    = 5000.0f;
    Actor.Light.bCastShadows = true;
    Actor.Light.Color        = FFloatColor(0.5f, 0.25f, 0.125f, 1.0f);
    return Actor;
}

template<typename T>
static bool SaveThenLoad(const T& Source, T& OutLoaded)
{
    FJsonValue Document;
    {
        FJsonArchive Saver = FJsonArchive::Saver(Document);
        TJsonSerializer<T>::Save(Document, Source);
    }

    FJsonArchive Loader = FJsonArchive::Loader(Document);
    if (!TJsonSerializer<T>::Load(Document, OutLoaded, Loader))
    {
        LOG_ERROR("[FAIL] loading reported failure: %s", Loader.GetErrorString().Data());
        return false;
    }

    if (Loader.HasErrors())
    {
        LOG_ERROR("[FAIL] loading recorded errors: %s", Loader.GetErrorString().Data());
        return false;
    }

    return true;
}

bool JsonArchive_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A nested struct survives save then load field by field");
    {
        const FTestActor Source = MakeActor();

        FTestActor Loaded;
        TEST_EXPECT(SaveThenLoad(Source, Loaded));
        TEST_EXPECT(Loaded.Name.Equals("StreetLight_01"));
        TEST_EXPECT(Loaded.Count == 7);
        TEST_EXPECT(Loaded.Shape == ETestShape::Capsule);
        TEST_EXPECT(Loaded.Translation.X == 15.0f);
        TEST_EXPECT(Loaded.Translation.Y == 2.5f);
        TEST_EXPECT(Loaded.Translation.Z == -3.0f);
        TEST_EXPECT(Loaded.Values.Size() == 3);
        TEST_EXPECT(Loaded.Light.Intensity == 5000.0f);
        TEST_EXPECT(Loaded.Light.bCastShadows);
        TEST_EXPECT(Loaded == Source);
    }

    TEST_SECTION("A field still at its default is left out of the output");
    {
        FTestActor Actor;
        Actor.Count = 3;

        FJsonValue Document;
        TJsonSerializer<FTestActor>::Save(Document, Actor);

        TEST_EXPECT(Document.Find("Count") != nullptr);
        TEST_EXPECT(Document.Find("Name") == nullptr);
        TEST_EXPECT(Document.Find("Shape") == nullptr);
        TEST_EXPECT(Document.Find("Translation") == nullptr);
        TEST_EXPECT(Document.Find("Light") == nullptr);

        TEST_EXPECT(Document.Find("Values") != nullptr);
    }

    TEST_SECTION("A field missing from the input takes its default without an error");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Count\": 12}"), Document));

        FTestActor Loaded;
        Loaded.Name  = String("stale");
        Loaded.Shape = ETestShape::Sphere;

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(!Loader.HasErrors());
        TEST_EXPECT(Loaded.Count == 12);
        TEST_EXPECT(Loaded.Name.IsEmpty());
        TEST_EXPECT(Loaded.Shape == ETestShape::Box);
        TEST_EXPECT(Loaded.Light.Intensity == 1.0f);
    }

    TEST_SECTION("A wrong-typed field records one error carrying its path");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Count\": \"not a number\", \"Name\": \"kept\"}"), Document));

        FTestActor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT(Loader.GetErrors()[0].Equals("Count: expected a number, found a string"));

        TEST_EXPECT(Loaded.Count == 0);
        TEST_EXPECT(Loaded.Name.Equals("kept"));
    }

    TEST_SECTION("An error inside a nested struct carries the whole path");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Light\": {\"Intensity\": true}}"), Document));

        FTestActor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT(Loader.GetErrors()[0].Equals("Light.Intensity: expected a number, found a bool"));
    }

    TEST_SECTION("Scalars round-trip through every integer width");
    {
        int8   Small  = -128;
        int8   SmallLoaded = 0;
        uint64 Large  = 18000000000000000000ULL;
        uint64 LargeLoaded = 0;

        TEST_EXPECT(SaveThenLoad(Small, SmallLoaded) && (SmallLoaded == Small));

        FJsonValue LargeDocument;
        TJsonSerializer<uint64>::Save(LargeDocument, Large);

        FJsonArchive Loader = FJsonArchive::Loader(LargeDocument);
        TEST_EXPECT(!TJsonSerializer<uint64>::Load(LargeDocument, LargeLoaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION("A value too large for the field is reported rather than truncated");
    {
        FJsonValue Document(static_cast<int64>(70000));

        int16 Loaded = 0;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<int16>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loaded == 0);
    }

    TEST_SECTION("An enum with a name table reads and writes as text");
    {
        FJsonValue Document;
        TJsonSerializer<ETestShape>::Save(Document, ETestShape::Sphere);
        TEST_EXPECT(Document.IsString());
        TEST_EXPECT(Document.GetStringOr("").Equals("Sphere"));

        ETestShape Loaded = ETestShape::Box;
        TEST_EXPECT(SaveThenLoad(ETestShape::Capsule, Loaded) && (Loaded == ETestShape::Capsule));

        FJsonValue AsNumber(static_cast<int64>(1));
        FJsonArchive Loader = FJsonArchive::Loader(AsNumber);
        TEST_EXPECT(TJsonSerializer<ETestShape>::Load(AsNumber, Loaded, Loader));
        TEST_EXPECT(Loaded == ETestShape::Sphere);

        FJsonValue Unknown("NotAShape");
        TEST_EXPECT(!TJsonSerializer<ETestShape>::Load(Unknown, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION("An enum without a name table reads and writes as its underlying integer");
    {
        FJsonValue Document;
        TJsonSerializer<ETestPlainEnum>::Save(Document, ETestPlainEnum::Second);
        TEST_EXPECT(Document.IsNumber() && (Document.GetInt64Or(0) == 20));

        ETestPlainEnum Loaded = ETestPlainEnum::First;
        TEST_EXPECT(SaveThenLoad(ETestPlainEnum::Second, Loaded) && (Loaded == ETestPlainEnum::Second));
    }

    TEST_SECTION("Containers round-trip");
    {
        TArray<String> Names = { String("a"), String("b"), String("c") };
        TArray<String> LoadedNames;
        TEST_EXPECT(SaveThenLoad(Names, LoadedNames));
        TEST_EXPECT(LoadedNames.Size() == 3);
        TEST_EXPECT(LoadedNames[2].Equals("c"));

        FTestFixedArray Fixed;
        Fixed[0] = 1;
        Fixed[1] = 2;
        Fixed[2] = 3;
        Fixed[3] = 4;

        FTestFixedArray LoadedFixed;
        TEST_EXPECT(SaveThenLoad(Fixed, LoadedFixed));
        TEST_EXPECT((LoadedFixed[0] == 1) && (LoadedFixed[3] == 4));

        FTestLookup Lookup;
        Lookup.Add(String("one"), 1);
        Lookup.Add(String("two"), 2);

        FTestLookup LoadedLookup;
        TEST_EXPECT(SaveThenLoad(Lookup, LoadedLookup));
        TEST_EXPECT(LoadedLookup.Size() == 2);
        TEST_EXPECT(LoadedLookup.Find(String("two")) && (*LoadedLookup.Find(String("two")) == 2));
    }

    TEST_SECTION("A static array of the wrong length is reported");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("[1, 2]"), Document));

        FTestFixedArray Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<FTestFixedArray>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION("An empty optional is left out of the output and reads back empty");
    {
        FTestWithOptional Empty;

        FJsonValue Document;
        TJsonSerializer<FTestWithOptional>::Save(Document, Empty);
        TEST_EXPECT(Document.Find("Maybe") == nullptr);

        FTestWithOptional Filled;
        Filled.Maybe.Emplace(42);

        FJsonValue FilledDocument;
        TJsonSerializer<FTestWithOptional>::Save(FilledDocument, Filled);
        TEST_EXPECT(FilledDocument.Find("Maybe") != nullptr);

        FTestWithOptional Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(FilledDocument);
        Loaded.Serialize(Loader);
        TEST_EXPECT(Loaded.Maybe.HasValue() && (Loaded.Maybe.GetValue() == 42));
    }

    TEST_SECTION("The math types round-trip as flat arrays");
    {
        Vector2 V2(1.0f, -2.0f);
        Vector2 V2Loaded;
        TEST_EXPECT(SaveThenLoad(V2, V2Loaded) && (V2Loaded.X == 1.0f) && (V2Loaded.Y == -2.0f));

        Vector4 V4(1.0f, 2.0f, 3.0f, 4.0f);
        Vector4 V4Loaded;
        TEST_EXPECT(SaveThenLoad(V4, V4Loaded) && (V4Loaded.W == 4.0f));

        Quaternion Q(0.0f, 0.7071068f, 0.0f, 0.7071068f);
        Quaternion QLoaded;
        TEST_EXPECT(SaveThenLoad(Q, QLoaded) && (QLoaded.Y == Q.Y) && (QLoaded.W == Q.W));

        Matrix4 M = Matrix4::Identity();
        M.M[3][0] = 5.0f;

        Matrix4 MLoaded;
        TEST_EXPECT(SaveThenLoad(M, MLoaded));
        TEST_EXPECT((MLoaded.M[3][0] == 5.0f) && (MLoaded.M[0][0] == 1.0f) && (MLoaded.M[1][2] == 0.0f));

        FJsonValue MatrixDocument;
        TJsonSerializer<Matrix4>::Save(MatrixDocument, M);
        TEST_EXPECT(MatrixDocument.IsArray() && (MatrixDocument.Num() == 16));

        FColor Color(10, 20, 30, 40);
        FColor ColorLoaded;
        TEST_EXPECT(SaveThenLoad(Color, ColorLoaded));
        TEST_EXPECT((ColorLoaded.R == 10) && (ColorLoaded.G == 20) && (ColorLoaded.B == 30) && (ColorLoaded.A == 40));

        FFloatColor FloatColor(0.25f, 0.5f, 0.75f, 1.0f);
        FFloatColor FloatColorLoaded;
        TEST_EXPECT(SaveThenLoad(FloatColor, FloatColorLoaded));
        TEST_EXPECT((FloatColorLoaded.R == 0.25f) && (FloatColorLoaded.B == 0.75f));
    }

    TEST_SECTION("A color channel outside 0 to 255 is reported");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("[0, 300, 0, 255]"), Document));

        FColor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<FColor>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION("The scopes walk an array of objects the same way in both directions");
    {
        const TArray<FTestActor> Source = { MakeActor(), FTestActor() };

        FJsonValue Document;
        {
            FJsonArchive Saver = FJsonArchive::Saver(Document);
            Saver.SetVersion(1);

            FJsonArchive::FArrayScope Actors(Saver, "Actors");
            for (int32 Index = 0; Index < Source.Size(); ++Index)
            {
                FJsonArchive::FObjectScope Actor(Saver);
                const_cast<FTestActor&>(Source[Index]).Serialize(Saver);
            }
        }

        TEST_EXPECT(Document.Find("Version") != nullptr);
        TEST_EXPECT(Document.Find("Actors") != nullptr);

        TArray<FTestActor> Loaded;
        {
            FJsonArchive Loader = FJsonArchive::Loader(Document);
            TEST_EXPECT(Loader.GetVersion() == 1);

            FJsonArchive::FArrayScope Actors(Loader, "Actors");
            TEST_EXPECT(Actors.IsValid());

            Loaded.Reset(Actors.Num());
            for (int32 Index = 0; Index < Loaded.Size(); ++Index)
            {
                FJsonArchive::FObjectScope Actor(Loader);
                Loaded[Index].Serialize(Loader);
            }

            TEST_EXPECT(!Loader.HasErrors());
        }

        TEST_EXPECT(Loaded.Size() == 2);
        TEST_EXPECT((Loaded.Size() == 2) && (Loaded[0] == Source[0]) && (Loaded[1] == Source[1]));
    }

    TEST_SECTION("An error inside an array scope names the element it came from");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Actors\": [{}, {\"Count\": \"nope\"}]}"), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, "Actors");
            for (int32 Index = 0; Index < Actors.Num(); ++Index)
            {
                FJsonArchive::FObjectScope Actor(Loader);

                FTestActor Loaded;
                Loaded.Serialize(Loader);
            }
        }

        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT((Loader.GetErrors().Size() == 1) && Loader.GetErrors()[0].Equals("Actors[1].Count: expected a number, found a string"));
    }

    TEST_SECTION("A missing scope loads its contents as defaults without an error");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{}"), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, "Actors");
            TEST_EXPECT(!Actors.IsValid());
            TEST_EXPECT(Actors.Num() == 0);
        }

        TEST_EXPECT(!Loader.HasErrors());
    }

    TEST_SECTION("A scope naming a member of the wrong type is reported");
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(StringView("{\"Actors\": 5}"), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, "Actors");
            TEST_EXPECT(!Actors.IsValid());
        }

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors()[0].Equals("Actors: expected an array, found a number"));
    }

    TEST_SECTION("A document survives a trip through a file");
    {
        const String Directory = Paths::GetProjectDir() + String("/Temp");
        const String Filename  = Directory + String("/JsonArchiveTests.json");

        const FTestActor Source = MakeActor();

        FJsonValue Document;
        TJsonSerializer<FTestActor>::Save(Document, Source);

        TEST_EXPECT(Json::SaveToFile(Filename, Document));

        FJsonValue FromDisk;
        FJsonError Error;
        TEST_EXPECT(Json::LoadFromFile(Filename, FromDisk, &Error));
        TEST_EXPECT(Document.Equals(FromDisk));

        FTestActor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(FromDisk);
        Loaded.Serialize(Loader);
        TEST_EXPECT(!Loader.HasErrors());
        TEST_EXPECT(Loaded == Source);

        TEST_EXPECT(FPlatformFile::DeleteFile(Filename.Data()));
        TEST_EXPECT(FPlatformFile::RemoveDirectory(Directory.Data()));

        FJsonValue Gone;
        TEST_EXPECT(!Json::LoadFromFile(Filename, Gone));
    }

    TEST_SECTION("Loading a file that does not exist fails without crashing");
    {
        FJsonValue Value;
        FJsonError Error;
        TEST_EXPECT(!Json::LoadFromFile(String("ThisPathDoesNotExist/Nope.json"), Value, &Error));
        TEST_EXPECT(!Error.Message.IsEmpty());
    }

    TEST_END();
}
