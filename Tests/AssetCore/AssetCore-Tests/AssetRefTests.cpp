#include "AssetRefTests.h"

#include <AssetCore/AssetRef.h>
#include <AssetCore/AssetRegistry.h>
#include <Core/Filesystem/File.h>
#include <Core/Json/Json.h>
#include <Core/Json/JsonSerializer.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

struct FTestTexture
{
};

template<>
struct TAssetTypeOf<FTestTexture>
{
    static constexpr EAssetType Value = EAssetType::Texture;
};

struct FTestMaterial
{
    TAssetRef<FTestTexture> BaseColor;
    TAssetRef<FTestTexture> Normal;

    void Serialize(FJsonArchive& Archive)
    {
        Archive.Field(TEXT("BaseColor"), BaseColor);
        Archive.Field(TEXT("Normal"), Normal, TAssetRef<FTestTexture>());
    }
};

static constexpr FGuid BrickTexture(0x11111111u, 0x1u, 0x1u, 0x1u);
static constexpr FGuid CubeMesh(0x22222222u, 0x2u, 0x2u, 0x2u);
static constexpr FGuid MissingAsset(0x99999999u, 0x9u, 0x9u, 0x9u);

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

static bool LoadMaterial(const CHAR_T* Text, FTestMaterial& OutMaterial, FString& OutErrors)
{
    FJsonValue Document;
    if (!Json::Parse(FStringView(Text), Document))
    {
        return false;
    }

    FJsonArchive Loader = FJsonArchive::Loader(Document);
    OutMaterial.Serialize(Loader);
    OutErrors = Loader.GetErrorString();
    return !Loader.HasErrors();
}

bool AssetRef_Test()
{
    TEST_BEGIN();

    const FString Root = File::MakeAbsolute(TEXT("AssetRefTests.Scratch"));
    File::DeleteDirectoryTree(Root);

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::EngineRoot, Root + TEXT("/Engine Content")));
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, Root + TEXT("/Project Content")));

    const FString RegistryFile = Root + TEXT("/Registry.json");
    TEST_EXPECT(WriteText(RegistryFile, FString::Printf(TEXT("{ \"Version\": 1, \"Assets\": [ ")
        TEXT("{ \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Project/Textures/Brick.dxrasset\" }, ")
        TEXT("{ \"Guid\": \"%s\", \"Type\": \"Mesh\", \"Path\": \"/Engine/Meshes/Cube.dxrasset\" } ] }"), *BrickTexture.ToString(), *CubeMesh.ToString())));

    FAssetRegistry Registry;
    Registry.SetMounts(Mounts);

    FString Error;
    TEST_EXPECT(Registry.LoadFromFile(RegistryFile, Error));

    TEST_SECTION(TEXT("A reference resolves by GUID, whatever its path says"));
    {
        const FAssetRef Brick(BrickTexture, TEXT("/Project/Old Folder/Brick.dxrasset"));
        TEST_EXPECT(Brick.IsSet());

        const FAssetData* Asset = Brick.Resolve(Registry);
        TEST_EXPECT(Asset && Asset->Path.Equals(TEXT("/Project/Textures/Brick.dxrasset")));

        const FAssetRef FromAsset = FAssetRef::FromAsset(*Registry.FindByGuid(CubeMesh));
        TEST_EXPECT(FromAsset.GetGuid() == CubeMesh && FromAsset.GetPath().Equals(TEXT("/Engine/Meshes/Cube.dxrasset")));
        TEST_EXPECT(FromAsset.Resolve(Registry) == Registry.FindByGuid(CubeMesh));
    }

    TEST_SECTION(TEXT("An unset or unregistered reference doesn't resolve, and says why with its path"));
    {
        const FAssetRef Unset;
        TEST_EXPECT(!Unset.IsSet() && Unset.ToString().Equals(TEXT("unset")));
        TEST_EXPECT(!Unset.Resolve(Registry, &Error) && Error.Contains(TEXT("No asset")));

        const FAssetRef Missing(MissingAsset, TEXT("/Project/Gone.dxrasset"));
        TEST_EXPECT(!Missing.Resolve(Registry, &Error));
        TEST_EXPECT(Error.Contains(TEXT("/Project/Gone.dxrasset")) && Error.Contains(MissingAsset.ToString()) && Error.Contains(TEXT("registry")));

        TEST_EXPECT(FAssetRef(MissingAsset).ToString().Equals(MissingAsset.ToString()));
        TEST_EXPECT(!Missing.Resolve(Registry));
    }

    TEST_SECTION(TEXT("A typed reference refuses an asset of another type"));
    {
        const TAssetRef<FTestTexture> Brick(BrickTexture);
        TEST_EXPECT(Brick.Resolve(Registry) && Brick.Resolve(Registry)->Type == EAssetType::Texture);

        const TAssetRef<FTestTexture> Cube(CubeMesh, TEXT("/Engine/Meshes/Cube.dxrasset"));
        TEST_EXPECT(!Cube.Resolve(Registry, &Error) && Error.Contains(TEXT("is a Mesh, not a Texture")));

        // Untyped, the same GUID resolves
        TEST_EXPECT(static_cast<const FAssetRef&>(Cube).Resolve(Registry) != nullptr);
    }

    TEST_SECTION(TEXT("References are equal when their GUIDs are"));
    {
        TEST_EXPECT(FAssetRef(BrickTexture, TEXT("/Project/A.dxrasset")) == FAssetRef(BrickTexture, TEXT("/Project/B.dxrasset")));
        TEST_EXPECT(!(FAssetRef(BrickTexture) == FAssetRef(CubeMesh)));
        TEST_EXPECT(FAssetRef() == FAssetRef());
    }

    TEST_SECTION(TEXT("A path updates to where the asset is now"));
    {
        FAssetRef Brick(BrickTexture, TEXT("/Project/Old Folder/Brick.dxrasset"));
        TEST_EXPECT(Brick.UpdatePath(Registry) && Brick.GetPath().Equals(TEXT("/Project/Textures/Brick.dxrasset")));
        TEST_EXPECT(!Brick.UpdatePath(Registry));

        FAssetRef Missing(MissingAsset, TEXT("/Project/Gone.dxrasset"));
        TEST_EXPECT(!Missing.UpdatePath(Registry) && Missing.GetPath().Equals(TEXT("/Project/Gone.dxrasset")));
    }

    TEST_SECTION(TEXT("A reference serializes as { Guid, Path }, and an unset one with a default is left out"));
    {
        FTestMaterial Material;
        Material.BaseColor = TAssetRef<FTestTexture>(BrickTexture, TEXT("/Project/Textures/Brick.dxrasset"));

        FJsonValue Document;
        TJsonSerializer<FTestMaterial>::Save(Document, Material);

        const FString Text     = Json::ToString(Document, EJsonWriteFlags::Compact);
        const FString Expected = FString::Printf(TEXT("{\"BaseColor\":{\"Guid\":\"%s\",\"Path\":\"/Project/Textures/Brick.dxrasset\"}}"), *BrickTexture.ToString());
        TEST_EXPECT(Text.Equals(Expected));

        FTestMaterial Loaded;
        FString Errors;
        TEST_EXPECT(LoadMaterial(*Text, Loaded, Errors));
        TEST_EXPECT(Loaded.BaseColor.GetGuid() == BrickTexture && Loaded.BaseColor.GetPath().Equals(TEXT("/Project/Textures/Brick.dxrasset")));
        TEST_EXPECT(!Loaded.Normal.IsSet());
    }

    TEST_SECTION(TEXT("An unset reference is left out even without a default, and reads back unset"));
    {
        FTestMaterial Material;
        Material.BaseColor = TAssetRef<FTestTexture>(FGuid(), TEXT("/Project/Only/A/Path.dxrasset"));

        FJsonValue Document;
        TJsonSerializer<FTestMaterial>::Save(Document, Material);

        const FString Text = Json::ToString(Document, EJsonWriteFlags::Compact);
        TEST_EXPECT(Text.Equals(TEXT("{}")));

        FTestMaterial Loaded;
        FString Errors;
        TEST_EXPECT(LoadMaterial(*Text, Loaded, Errors) && !Loaded.BaseColor.IsSet() && !Loaded.Normal.IsSet());
    }

    TEST_SECTION(TEXT("Path is optional when loading, and a missing or invalid Guid is an error"));
    {
        FTestMaterial Loaded;
        FString Errors;

        const FString NoPath = FString::Printf(TEXT("{ \"BaseColor\": { \"Guid\": \"%s\" } }"), *BrickTexture.ToString());
        TEST_EXPECT(LoadMaterial(*NoPath, Loaded, Errors) && Loaded.BaseColor.GetGuid() == BrickTexture && Loaded.BaseColor.GetPath().IsEmpty());

        TEST_EXPECT(!LoadMaterial(TEXT("{ \"BaseColor\": { \"Path\": \"/Project/A.dxrasset\" } }"), Loaded, Errors) && Errors.Contains(TEXT("BaseColor")));
        TEST_EXPECT(!LoadMaterial(TEXT("{ \"BaseColor\": { \"Guid\": \"not a guid\" } }"), Loaded, Errors) && Errors.Contains(TEXT("not a GUID")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
