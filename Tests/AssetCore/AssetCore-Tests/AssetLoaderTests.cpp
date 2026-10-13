#include "AssetLoaderTests.h"

#include <AssetCore/AssetLoader.h>
#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

class FLoaderTestTexture : public FAsset
{
public:
    FString LoadedFrom;
};

class FLoaderTestMesh : public FAsset
{
public:
    FString LoadedFrom;
};

template<>
struct TAssetTypeOf<FLoaderTestTexture>
{
    static constexpr EAssetType Value = EAssetType::Texture;
};

template<>
struct TAssetTypeOf<FLoaderTestMesh>
{
    static constexpr EAssetType Value = EAssetType::Mesh;
};

static constexpr FGuid BrickTexture(0x11111111u, 0x1u, 0x1u, 0x1u);
static constexpr FGuid CubeMesh(0x22222222u, 0x2u, 0x2u, 0x2u);
static constexpr FGuid BrokenTexture(0x33333333u, 0x3u, 0x3u, 0x3u);
static constexpr FGuid MissingAsset(0x99999999u, 0x9u, 0x9u, 0x9u);

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

bool AssetLoader_Test()
{
    TEST_BEGIN();

    const FString Root = File::MakeAbsolute(TEXT("AssetLoaderTests.Scratch"));
    File::DeleteDirectoryTree(Root);

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::EngineRoot, Root + TEXT("/Engine Content")));
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, Root + TEXT("/Project Content")));

    const FString RegistryFile = Root + TEXT("/Registry.json");
    TEST_EXPECT(WriteText(RegistryFile, FString::Printf(TEXT("{ \"Version\": 1, \"Assets\": [ ")
        TEXT("{ \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Project/Textures/Brick.dxrasset\" }, ")
        TEXT("{ \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Project/Textures/Broken.dxrasset\" }, ")
        TEXT("{ \"Guid\": \"%s\", \"Type\": \"Mesh\", \"Path\": \"/Engine/Meshes/Cube.dxrasset\" } ] }"),
        *BrickTexture.ToString(), *BrokenTexture.ToString(), *CubeMesh.ToString())));

    FAssetRegistry Registry;
    Registry.SetMounts(Mounts);

    FString Error;
    TEST_EXPECT(Registry.LoadFromFile(RegistryFile, Error));

    FAssetLoader Loader(Registry);

    int32 NumTextureLoads = 0;
    Loader.RegisterLoader<FLoaderTestTexture>([&NumTextureLoads](const FAssetData& Asset, const FString& DiskPath, FString& OutError)
    {
        ++NumTextureLoads;
        if (Asset.Name.Equals(TEXT("Broken")))
        {
            OutError = TEXT("the payload is corrupt");
            return TSharedPtr<FLoaderTestTexture>();
        }

        TSharedPtr<FLoaderTestTexture> Texture = MakeSharedPtr<FLoaderTestTexture>();
        Texture->LoadedFrom = DiskPath;
        return Texture;
    });

    TEST_SECTION(TEXT("An asset loads through its type's loader from its file, and remembers which asset it is"));
    {
        TSharedPtr<FLoaderTestTexture> Texture = Loader.Load<FLoaderTestTexture>(BrickTexture, &Error);
        TEST_EXPECT(Texture.IsValid() && NumTextureLoads == 1);
        TEST_EXPECT(Texture->LoadedFrom.Equals(Root + TEXT("/Project Content/Textures/Brick.dxrasset")));
        TEST_EXPECT(Texture->IsAsset() && Texture->GetAssetRef().GetGuid() == BrickTexture);
        TEST_EXPECT(Texture->GetAssetRef().GetPath().Equals(TEXT("/Project/Textures/Brick.dxrasset")));
        TEST_EXPECT(Loader.IsLoaded(BrickTexture) && Loader.GetNumLoaded() == 1);
    }

    TEST_SECTION(TEXT("Loading it again returns the same object without loading the file"));
    {
        TSharedPtr<FLoaderTestTexture> First  = Loader.Load<FLoaderTestTexture>(BrickTexture);
        TSharedPtr<FLoaderTestTexture> Second = Loader.Load<FLoaderTestTexture>(TAssetRef<FLoaderTestTexture>(BrickTexture, TEXT("/Project/Old/Brick.dxrasset")));
        TEST_EXPECT(First.Get() == Second.Get() && NumTextureLoads == 1);
        TEST_EXPECT(Loader.FindLoaded<FLoaderTestTexture>(BrickTexture).Get() == First.Get());
    }

    TEST_SECTION(TEXT("An unregistered GUID, an asset of another type, a type without a loader, and a failed load each say why"));
    {
        TEST_EXPECT(!Loader.Load<FLoaderTestTexture>(MissingAsset, &Error).IsValid() && Error.Contains(TEXT("isn't in the asset registry")));
        TEST_EXPECT(!Loader.Load<FLoaderTestTexture>(CubeMesh, &Error).IsValid() && Error.Contains(TEXT("is a Mesh, not a Texture")));
        TEST_EXPECT(!Loader.Load<FLoaderTestMesh>(CubeMesh, &Error).IsValid() && Error.Contains(TEXT("no loader for Mesh assets")));
        TEST_EXPECT(!Loader.Load<FLoaderTestMesh>(BrickTexture, &Error).IsValid() && Error.Contains(TEXT("loaded as a Texture, not a Mesh")));

        const int32 LoadsBefore = NumTextureLoads;
        TEST_EXPECT(!Loader.Load<FLoaderTestTexture>(BrokenTexture, &Error).IsValid());
        TEST_EXPECT(Error.Contains(TEXT("/Project/Textures/Broken.dxrasset")) && Error.Contains(TEXT("the payload is corrupt")));
        TEST_EXPECT(!Loader.IsLoaded(BrokenTexture));

        TEST_EXPECT(!Loader.Load<FLoaderTestTexture>(BrokenTexture).IsValid() && NumTextureLoads == LoadsBefore + 2);
        TEST_EXPECT(!Loader.FindLoaded<FLoaderTestTexture>(MissingAsset).IsValid());
    }

    TEST_SECTION(TEXT("Another asset type loads the same way once it has a loader"));
    {
        Loader.RegisterLoader<FLoaderTestMesh>([](const FAssetData&, const FString& DiskPath, FString&)
        {
            TSharedPtr<FLoaderTestMesh> Mesh = MakeSharedPtr<FLoaderTestMesh>();
            Mesh->LoadedFrom = DiskPath;
            return Mesh;
        });

        TSharedPtr<FLoaderTestMesh> Mesh = Loader.Load<FLoaderTestMesh>(CubeMesh, &Error);
        TEST_EXPECT(Mesh.IsValid() && Mesh->GetAssetRef().GetGuid() == CubeMesh);
        TEST_EXPECT(Mesh->LoadedFrom.Equals(Root + TEXT("/Engine Content/Meshes/Cube.dxrasset")));
        TEST_EXPECT(Loader.Load<FLoaderTestMesh>(CubeMesh).Get() == Mesh.Get());
    }

    TEST_SECTION(TEXT("Unloading lets go of the asset, which loads from its file again next time, while others keep their object"));
    {
        TSharedPtr<FLoaderTestTexture> Kept = Loader.Load<FLoaderTestTexture>(BrickTexture);
        const int32 LoadsBefore = NumTextureLoads;

        TEST_EXPECT(Loader.Unload(BrickTexture) && !Loader.IsLoaded(BrickTexture) && !Loader.Unload(BrickTexture));
        TEST_EXPECT(Kept.IsValid() && Kept->IsAsset());

        TSharedPtr<FLoaderTestTexture> Reloaded = Loader.Load<FLoaderTestTexture>(BrickTexture);
        TEST_EXPECT(Reloaded.IsValid() && Reloaded.Get() != Kept.Get() && NumTextureLoads == LoadsBefore + 1);

        Loader.UnloadAll();
        TEST_EXPECT(Loader.GetNumLoaded() == 0);

        TEST_EXPECT(Loader.UnregisterLoader(EAssetType::Mesh) && !Loader.UnregisterLoader(EAssetType::Mesh));
        TEST_EXPECT(!Loader.Load<FLoaderTestMesh>(CubeMesh, &Error).IsValid() && Error.Contains(TEXT("no loader")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
