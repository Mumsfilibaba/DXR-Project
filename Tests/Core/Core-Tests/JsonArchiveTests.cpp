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
typedef TMap<FString, int32>   FTestLookup;

struct FTestWithOptional
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field(TEXT("Maybe"), Maybe);
    }

    TOptional<int32> Maybe;
};

struct FTestLight
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field(TEXT("Intensity"), Intensity, 1.0f);
        Archive.Field(TEXT("bCastShadows"), bCastShadows, false);
        Archive.Field(TEXT("Color"), Color, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f));
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
        Archive.Field(TEXT("Name"), Name, FString(TEXT("")));
        Archive.Field(TEXT("Count"), Count, 0);
        Archive.Field(TEXT("Shape"), Shape, ETestShape::Box);
        Archive.Field(TEXT("Translation"), Translation, FVector3(0.0f, 0.0f, 0.0f));
        Archive.Field(TEXT("Values"), Values);
        Archive.Field(TEXT("Light"), Light, FTestLight());
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

    FString       Name;
    int32         Count = 0;
    ETestShape    Shape = ETestShape::Box;
    FVector3      Translation{ 0.0f, 0.0f, 0.0f };
    TArray<int32> Values;
    FTestLight    Light;
};

static FTestActor MakeActor()
{
    FTestActor Actor;
    Actor.Name        = FString(TEXT("StreetLight_01"));
    Actor.Count       = 7;
    Actor.Shape       = ETestShape::Capsule;
    Actor.Translation = FVector3(15.0f, 2.5f, -3.0f);
    Actor.Values      = { 1, 2, 3 };

    Actor.Light.Intensity    = 5000.0f;
    Actor.Light.bCastShadows = true;
    Actor.Light.Color        = FFloatColor(0.5f, 0.25f, 0.125f, 1.0f);
    return Actor;
}

struct FTestInventory
{
    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field(TEXT("Lights"), Lights);
        Archive.Field(TEXT("Counts"), Counts);
        Archive.Field(TEXT("Pair"), Pair);
    }

    TArray<FTestLight>     Lights;
    TMap<FString, int32>   Counts;
    TStaticArray<int32, 2> Pair;
};

static FString LoadInventoryError(const CHAR_T* Text)
{
    FJsonValue Document;
    if (!Json::Parse(FStringView(Text), Document))
    {
        return FString();
    }

    FTestInventory Inventory;
    FJsonArchive Loader = FJsonArchive::Loader(Document);
    Inventory.Serialize(Loader);
    return (Loader.GetErrors().Size() == 1) ? Loader.GetErrors()[0] : FString();
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

    TEST_SECTION(TEXT("A nested struct survives save then load field by field"));
    {
        const FTestActor Source = MakeActor();

        FTestActor Loaded;
        TEST_EXPECT(SaveThenLoad(Source, Loaded));
        TEST_EXPECT(Loaded.Name.Equals(TEXT("StreetLight_01")));
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

    TEST_SECTION(TEXT("A field still at its default is left out of the output"));
    {
        FTestActor Actor;
        Actor.Count = 3;

        FJsonValue Document;
        TJsonSerializer<FTestActor>::Save(Document, Actor);

        TEST_EXPECT(Document.Find(TEXT("Count")) != nullptr);
        TEST_EXPECT(Document.Find(TEXT("Name")) == nullptr);
        TEST_EXPECT(Document.Find(TEXT("Shape")) == nullptr);
        TEST_EXPECT(Document.Find(TEXT("Translation")) == nullptr);
        TEST_EXPECT(Document.Find(TEXT("Light")) == nullptr);

        TEST_EXPECT(Document.Find(TEXT("Values")) != nullptr);
    }

    TEST_SECTION(TEXT("A field missing from the input takes its default without an error"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"Count\": 12}")), Document));

        FTestActor Loaded;
        Loaded.Name  = FString(TEXT("stale"));
        Loaded.Shape = ETestShape::Sphere;

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(!Loader.HasErrors());
        TEST_EXPECT(Loaded.Count == 12);
        TEST_EXPECT(Loaded.Name.IsEmpty());
        TEST_EXPECT(Loaded.Shape == ETestShape::Box);
        TEST_EXPECT(Loaded.Light.Intensity == 1.0f);
    }

    TEST_SECTION(TEXT("A wrong-typed field records one error carrying its path"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"Count\": \"not a number\", \"Name\": \"kept\"}")), Document));

        FTestActor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT(Loader.GetErrors()[0].Equals(TEXT("Count: expected a number, found a string")));

        TEST_EXPECT(Loaded.Count == 0);
        TEST_EXPECT(Loaded.Name.Equals(TEXT("kept")));
    }

    TEST_SECTION(TEXT("An error inside a nested struct carries the whole path"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"Light\": {\"Intensity\": true}}")), Document));

        FTestActor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        Loaded.Serialize(Loader);

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT(Loader.GetErrors()[0].Equals(TEXT("Light.Intensity: expected a number, found a bool")));
    }

    TEST_SECTION(TEXT("Scalars round-trip through every integer width"));
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

    TEST_SECTION(TEXT("A value too large for the field is reported rather than truncated"));
    {
        FJsonValue Document(static_cast<int64>(70000));

        int16 Loaded = 0;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<int16>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loaded == 0);
    }

    TEST_SECTION(TEXT("An enum with a name table reads and writes as text"));
    {
        FJsonValue Document;
        TJsonSerializer<ETestShape>::Save(Document, ETestShape::Sphere);
        TEST_EXPECT(Document.IsString());
        TEST_EXPECT(Document.GetStringOr(TEXT("")).Equals(TEXT("Sphere")));

        ETestShape Loaded = ETestShape::Box;
        TEST_EXPECT(SaveThenLoad(ETestShape::Capsule, Loaded) && (Loaded == ETestShape::Capsule));

        FJsonValue AsNumber(static_cast<int64>(1));
        FJsonArchive Loader = FJsonArchive::Loader(AsNumber);
        TEST_EXPECT(TJsonSerializer<ETestShape>::Load(AsNumber, Loaded, Loader));
        TEST_EXPECT(Loaded == ETestShape::Sphere);

        FJsonValue Unknown(TEXT("NotAShape"));
        TEST_EXPECT(!TJsonSerializer<ETestShape>::Load(Unknown, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION(TEXT("An enum without a name table reads and writes as its underlying integer"));
    {
        FJsonValue Document;
        TJsonSerializer<ETestPlainEnum>::Save(Document, ETestPlainEnum::Second);
        TEST_EXPECT(Document.IsNumber() && (Document.GetInt64Or(0) == 20));

        ETestPlainEnum Loaded = ETestPlainEnum::First;
        TEST_EXPECT(SaveThenLoad(ETestPlainEnum::Second, Loaded) && (Loaded == ETestPlainEnum::Second));
    }

    TEST_SECTION(TEXT("Containers round-trip"));
    {
        TArray<FString> Names = { FString(TEXT("a")), FString(TEXT("b")), FString(TEXT("c")) };
        TArray<FString> LoadedNames;
        TEST_EXPECT(SaveThenLoad(Names, LoadedNames));
        TEST_EXPECT(LoadedNames.Size() == 3);
        TEST_EXPECT(LoadedNames[2].Equals(TEXT("c")));

        FTestFixedArray Fixed;
        Fixed[0] = 1;
        Fixed[1] = 2;
        Fixed[2] = 3;
        Fixed[3] = 4;

        FTestFixedArray LoadedFixed;
        TEST_EXPECT(SaveThenLoad(Fixed, LoadedFixed));
        TEST_EXPECT((LoadedFixed[0] == 1) && (LoadedFixed[3] == 4));

        FTestLookup Lookup;
        Lookup.Add(FString(TEXT("one")), 1);
        Lookup.Add(FString(TEXT("two")), 2);

        FTestLookup LoadedLookup;
        TEST_EXPECT(SaveThenLoad(Lookup, LoadedLookup));
        TEST_EXPECT(LoadedLookup.Size() == 2);
        TEST_EXPECT(LoadedLookup.Find(FString(TEXT("two"))) && (*LoadedLookup.Find(FString(TEXT("two"))) == 2));
    }

    TEST_SECTION(TEXT("A static array of the wrong length is reported"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("[1, 2]")), Document));

        FTestFixedArray Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<FTestFixedArray>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION(TEXT("An empty optional is left out of the output and reads back empty"));
    {
        FTestWithOptional Empty;

        FJsonValue Document;
        TJsonSerializer<FTestWithOptional>::Save(Document, Empty);
        TEST_EXPECT(Document.Find(TEXT("Maybe")) == nullptr);

        FTestWithOptional Filled;
        Filled.Maybe.Emplace(42);

        FJsonValue FilledDocument;
        TJsonSerializer<FTestWithOptional>::Save(FilledDocument, Filled);
        TEST_EXPECT(FilledDocument.Find(TEXT("Maybe")) != nullptr);

        FTestWithOptional Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(FilledDocument);
        Loaded.Serialize(Loader);
        TEST_EXPECT(Loaded.Maybe.HasValue() && (Loaded.Maybe.GetValue() == 42));
    }

    TEST_SECTION(TEXT("The math types round-trip as flat arrays"));
    {
        FVector2 V2(1.0f, -2.0f);
        FVector2 V2Loaded;
        TEST_EXPECT(SaveThenLoad(V2, V2Loaded) && (V2Loaded.X == 1.0f) && (V2Loaded.Y == -2.0f));

        FVector4 V4(1.0f, 2.0f, 3.0f, 4.0f);
        FVector4 V4Loaded;
        TEST_EXPECT(SaveThenLoad(V4, V4Loaded) && (V4Loaded.W == 4.0f));

        FQuaternion Q(0.0f, 0.7071068f, 0.0f, 0.7071068f);
        FQuaternion QLoaded;
        TEST_EXPECT(SaveThenLoad(Q, QLoaded) && (QLoaded.Y == Q.Y) && (QLoaded.W == Q.W));

        FMatrix4 M = FMatrix4::Identity();
        M.M[3][0] = 5.0f;

        FMatrix4 MLoaded;
        TEST_EXPECT(SaveThenLoad(M, MLoaded));
        TEST_EXPECT((MLoaded.M[3][0] == 5.0f) && (MLoaded.M[0][0] == 1.0f) && (MLoaded.M[1][2] == 0.0f));

        FJsonValue MatrixDocument;
        TJsonSerializer<FMatrix4>::Save(MatrixDocument, M);
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

    TEST_SECTION(TEXT("A color channel outside 0 to 255 is reported"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("[0, 300, 0, 255]")), Document));

        FColor Loaded;
        FJsonArchive Loader = FJsonArchive::Loader(Document);
        TEST_EXPECT(!TJsonSerializer<FColor>::Load(Document, Loaded, Loader));
        TEST_EXPECT(Loader.HasErrors());
    }

    TEST_SECTION(TEXT("The scopes walk an array of objects the same way in both directions"));
    {
        const TArray<FTestActor> Source = { MakeActor(), FTestActor() };

        FJsonValue Document;
        {
            FJsonArchive Saver = FJsonArchive::Saver(Document);
            Saver.SetVersion(1);

            FJsonArchive::FArrayScope Actors(Saver, TEXT("Actors"));
            for (int32 Index = 0; Index < Source.Size(); ++Index)
            {
                FJsonArchive::FObjectScope Actor(Saver);
                const_cast<FTestActor&>(Source[Index]).Serialize(Saver);
            }
        }

        TEST_EXPECT(Document.Find(TEXT("Version")) != nullptr);
        TEST_EXPECT(Document.Find(TEXT("Actors")) != nullptr);

        TArray<FTestActor> Loaded;
        {
            FJsonArchive Loader = FJsonArchive::Loader(Document);
            TEST_EXPECT(Loader.GetVersion() == 1);

            FJsonArchive::FArrayScope Actors(Loader, TEXT("Actors"));
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

    TEST_SECTION(TEXT("An error inside an array scope names the element it came from"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"Actors\": [{}, {\"Count\": \"nope\"}]}")), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, TEXT("Actors"));
            for (int32 Index = 0; Index < Actors.Num(); ++Index)
            {
                FJsonArchive::FObjectScope Actor(Loader);

                FTestActor Loaded;
                Loaded.Serialize(Loader);
            }
        }

        TEST_EXPECT(Loader.GetErrors().Size() == 1);
        TEST_EXPECT((Loader.GetErrors().Size() == 1) && Loader.GetErrors()[0].Equals(TEXT("Actors[1].Count: expected a number, found a string")));
    }

    TEST_SECTION(TEXT("A missing scope loads its contents as defaults without an error"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{}")), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, TEXT("Actors"));
            TEST_EXPECT(!Actors.IsValid());
            TEST_EXPECT(Actors.Num() == 0);
        }

        TEST_EXPECT(!Loader.HasErrors());
    }

    TEST_SECTION(TEXT("A scope naming a member of the wrong type is reported"));
    {
        FJsonValue Document;
        TEST_EXPECT(Json::Parse(FStringView(TEXT("{\"Actors\": 5}")), Document));

        FJsonArchive Loader = FJsonArchive::Loader(Document);
        {
            FJsonArchive::FArrayScope Actors(Loader, TEXT("Actors"));
            TEST_EXPECT(!Actors.IsValid());
        }

        TEST_EXPECT(Loader.HasErrors());
        TEST_EXPECT(Loader.GetErrors()[0].Equals(TEXT("Actors: expected an array, found a number")));
    }

    TEST_SECTION(TEXT("A document survives a trip through a file"));
    {
        const FString Directory = Paths::GetProjectDir() + FString(TEXT("/Temp"));
        const FString Filename  = Directory + FString(TEXT("/JsonArchiveTests.json"));

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

    TEST_SECTION(TEXT("Loading a file that does not exist fails without crashing"));
    {
        FJsonValue Value;
        FJsonError Error;
        TEST_EXPECT(!Json::LoadFromFile(FString(TEXT("ThisPathDoesNotExist/Nope.json")), Value, &Error));
        TEST_EXPECT(!Error.Message.IsEmpty());
    }

    TEST_SECTION(TEXT("Errors inside a serialized array, static array or map name the element"));
    {
        TEST_EXPECT(LoadInventoryError(TEXT("{ \"Lights\": [ {}, { \"Intensity\": true } ] }"))
            .Equals(TEXT("Lights[1].Intensity: expected a number, found a bool")));

        TEST_EXPECT(LoadInventoryError(TEXT("{ \"Lights\": [ {}, {}, 5 ] }"))
            .Equals(TEXT("Lights[2]: expected an object, found a number")));

        TEST_EXPECT(LoadInventoryError(TEXT("{ \"Counts\": { \"Apples\": 1, \"Pears\": \"many\" } }"))
            .Equals(TEXT("Counts.Pears: expected a number, found a string")));

        TEST_EXPECT(LoadInventoryError(TEXT("{ \"Pair\": [ 1, \"two\" ] }"))
            .Equals(TEXT("Pair[1]: expected a number, found a string")));
    }

    TEST_END();
}
