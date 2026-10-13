#include "AssetRegistryTests.h"

#include <AssetCore/AssetFile.h>
#include <AssetCore/AssetRegistry.h>
#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

static constexpr FGuid BrickTexture(0x11111111u, 0x1u, 0x1u, 0x1u);
static constexpr FGuid CubeMesh(0x22222222u, 0x2u, 0x2u, 0x2u);
static constexpr FGuid BrickMaterial(0x33333333u, 0x3u, 0x3u, 0x3u);
static constexpr FGuid LevelScene(0x44444444u, 0x4u, 0x4u, 0x4u);
static constexpr FGuid ChairModel(0x55555555u, 0x5u, 0x5u, 0x5u);
static constexpr FGuid NewTexture(0x66666666u, 0x6u, 0x6u, 0x6u);

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

static bool WriteAsset(const FString& Filename, const FGuid& Guid, EAssetType Type)
{
    FAssetFileWriter Writer(Guid, Type);
    Writer.SetChunk(AssetChunk::Data, TArray<uint8>({ 1, 2, 3 }));

    FString Error;
    return Writer.SaveToFile(Filename, Error);
}

static bool WriteTextAsset(const FString& Filename, const FGuid& Guid, const CHAR_T* Type)
{
    return WriteText(Filename, FString::Printf(TEXT("{\n    \"Version\": 1,\n    \"Guid\": \"%s\",\n    \"Type\": \"%s\"\n}\n"), *Guid.ToString(), Type));
}

static bool ContainsError(const FAssetRegistry& Registry, const CHAR_T* First, const CHAR_T* Second = nullptr)
{
    for (const FString& Error : Registry.GetErrors())
    {
        if (Error.Contains(First) && (!Second || Error.Contains(Second)))
        {
            return true;
        }
    }

    return false;
}

bool AssetRegistry_Test()
{
    TEST_BEGIN();

    const FString Root          = File::MakeAbsolute(TEXT("AssetRegistryTests.Scratch"));
    const FString EngineContent  = Root + TEXT("/Engine Content");
    const FString ProjectContent = Root + TEXT("/Project Content");
    File::DeleteDirectoryTree(Root);

    TEST_EXPECT(WriteAsset(EngineContent + TEXT("/Textures/Brick.dxrasset"), BrickTexture, EAssetType::Texture));
    TEST_EXPECT(WriteAsset(EngineContent + TEXT("/Meshes/Cube.dxrasset"), CubeMesh, EAssetType::Mesh));
    TEST_EXPECT(WriteText(EngineContent + TEXT("/Textures/Brick.png"), TEXT("Not an asset")));
    TEST_EXPECT(WriteText(EngineContent + TEXT("/Readme.txt"), TEXT("Not an asset")));
    TEST_EXPECT(WriteTextAsset(ProjectContent + TEXT("/Materials/Brick.dxrmaterial"), BrickMaterial, TEXT("Material")));
    TEST_EXPECT(WriteTextAsset(ProjectContent + TEXT("/Scenes/Level 1.dxrscene"), LevelScene, TEXT("Scene")));
    TEST_EXPECT(WriteTextAsset(ProjectContent + TEXT("/Models/Chair.dxrmodel"), ChairModel, TEXT("Model")));

    TEST_EXPECT(WriteText(ProjectContent + TEXT("/Broken.dxrasset"), TEXT("Not an asset file")));
    TEST_EXPECT(WriteText(ProjectContent + TEXT("/Materials/NoGuid.dxrmaterial"), TEXT("{ \"Version\": 1, \"Type\": \"Material\" }")));
    TEST_EXPECT(WriteTextAsset(ProjectContent + TEXT("/Scenes/WrongType.dxrscene"), FGuid(0x7u, 0x7u, 0x7u, 0x7u), TEXT("Material")));
    TEST_EXPECT(WriteText(ProjectContent + TEXT("/Scenes/NotJson.dxrscene"), TEXT("{ \"Guid\": ")));
    TEST_EXPECT(WriteAsset(ProjectContent + TEXT("/Textures/Copy.dxrasset"), BrickTexture, EAssetType::Texture));

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::EngineRoot, EngineContent));
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, ProjectContent));

    TEST_SECTION(TEXT("A scan indexes .dxrasset and text assets by GUID and by path"));
    {
        FAssetRegistry Registry;
        Registry.SetMounts(Mounts);
        Registry.Rescan();
        TEST_EXPECT(Registry.GetNumAssets() == 5);

        const FAssetData* Brick = Registry.FindByGuid(BrickTexture);
        TEST_EXPECT(Brick && Brick->Path.Equals(TEXT("/Engine/Textures/Brick.dxrasset")) && Brick->Type == EAssetType::Texture && Brick->Name.Equals(TEXT("Brick")));

        const FAssetData* Level = Registry.FindByPath(TEXT("/Project/Scenes/Level 1.dxrscene"));
        TEST_EXPECT(Level && Level->Guid == LevelScene && Level->Type == EAssetType::Scene && Level->Name.Equals(TEXT("Level 1")));

        TEST_EXPECT(Registry.FindByGuid(CubeMesh) && Registry.FindByGuid(CubeMesh)->Type == EAssetType::Mesh);
        TEST_EXPECT(Registry.FindByGuid(BrickMaterial) && Registry.FindByGuid(BrickMaterial)->Type == EAssetType::Material);
        TEST_EXPECT(Registry.FindByGuid(ChairModel) && Registry.FindByGuid(ChairModel)->Type == EAssetType::Model);
        TEST_EXPECT(Registry.FindByPath(TEXT("/Engine/Textures/Brick.png")) == nullptr);
        TEST_EXPECT(Registry.FindByGuid(NewTexture) == nullptr);

        const TArray<FAssetData> Assets = Registry.GetAssets();
        TEST_EXPECT(Assets.Size() == 5 && Assets[0].Path.Equals(TEXT("/Engine/Meshes/Cube.dxrasset")) && Assets[4].Path.Equals(TEXT("/Project/Scenes/Level 1.dxrscene")));
    }

    TEST_SECTION(TEXT("Files that can't be indexed are left out with an error each, and of two files with one GUID the first is kept"));
    {
        FAssetRegistry Registry;
        Registry.SetMounts(Mounts);
        Registry.Rescan();

        TEST_EXPECT(Registry.GetErrors().Size() == 5);
        TEST_EXPECT(ContainsError(Registry, TEXT("/Project/Broken.dxrasset"), TEXT("too small")));
        TEST_EXPECT(ContainsError(Registry, TEXT("/Project/Materials/NoGuid.dxrmaterial"), TEXT("Guid")));
        TEST_EXPECT(ContainsError(Registry, TEXT("/Project/Scenes/WrongType.dxrscene"), TEXT("'Material' instead of 'Scene'")));
        TEST_EXPECT(ContainsError(Registry, TEXT("/Project/Scenes/NotJson.dxrscene"), TEXT("JSON")));
        TEST_EXPECT(ContainsError(Registry, TEXT("/Project/Textures/Copy.dxrasset"), TEXT("/Engine/Textures/Brick.dxrasset")));
        TEST_EXPECT(Registry.FindByGuid(BrickTexture)->Path.Equals(TEXT("/Engine/Textures/Brick.dxrasset")));
        TEST_EXPECT(Registry.FindByPath(TEXT("/Project/Textures/Copy.dxrasset")) == nullptr);
    }

    TEST_SECTION(TEXT("A mount whose folder doesn't exist holds nothing"));
    {
        FContentMounts Missing;
        TEST_EXPECT(Missing.Mount(TEXT("/Missing/"), Root + TEXT("/Missing Content")));

        FAssetRegistry Registry;
        Registry.SetMounts(Missing);
        Registry.Rescan();
        TEST_EXPECT(Registry.GetNumAssets() == 0 && Registry.GetErrors().IsEmpty());
    }

    TEST_SECTION(TEXT("Rescanning broadcasts what was added, removed and moved"));
    {
        FAssetRegistry Registry;
        Registry.SetMounts(Mounts);

        TArray<FString> Added;
        TArray<FString> Removed;
        TArray<FString> Renamed;
        Registry.GetOnAssetAddedEvent().AddLambda([&Added](const FAssetData& Asset)
        {
            Added.Add(Asset.Path);
        });

        Registry.GetOnAssetRemovedEvent().AddLambda([&Removed](const FAssetData& Asset)
        {
            Removed.Add(Asset.Path);
        });

        Registry.GetOnAssetRenamedEvent().AddLambda([&Renamed](const FAssetData& Asset, const FString& OldPath)
        {
            Renamed.Add(OldPath + TEXT(" -> ") + Asset.Path);
        });

        Registry.Rescan();
        TEST_EXPECT(Added.Size() == 5 && Added[0].Equals(TEXT("/Engine/Meshes/Cube.dxrasset")) && Removed.IsEmpty() && Renamed.IsEmpty());

        Added.Clear();
        Registry.Rescan();
        TEST_EXPECT(Added.IsEmpty() && Removed.IsEmpty() && Renamed.IsEmpty());

        TEST_EXPECT(FPlatformFile::DeleteFile(*(EngineContent + TEXT("/Meshes/Cube.dxrasset"))));
        TEST_EXPECT(File::CreateDirectoryTree(ProjectContent + TEXT("/Materials/Walls")));
        TEST_EXPECT(FPlatformFile::MoveFile(*(ProjectContent + TEXT("/Materials/Brick.dxrmaterial")), *(ProjectContent + TEXT("/Materials/Walls/Red Brick.dxrmaterial"))));
        TEST_EXPECT(WriteAsset(ProjectContent + TEXT("/Textures/New.dxrasset"), NewTexture, EAssetType::Texture));

        Registry.Rescan();
        TEST_EXPECT(Removed.Size() == 1 && Removed[0].Equals(TEXT("/Engine/Meshes/Cube.dxrasset")));
        TEST_EXPECT(Renamed.Size() == 1 && Renamed[0].Equals(TEXT("/Project/Materials/Brick.dxrmaterial -> /Project/Materials/Walls/Red Brick.dxrmaterial")));
        TEST_EXPECT(Added.Size() == 1 && Added[0].Equals(TEXT("/Project/Textures/New.dxrasset")));

        const FAssetData* Moved = Registry.FindByGuid(BrickMaterial);
        TEST_EXPECT(Moved && Moved->Name.Equals(TEXT("Red Brick")) && Registry.FindByPath(TEXT("/Project/Materials/Brick.dxrmaterial")) == nullptr);
    }

    TEST_SECTION(TEXT("The index round trips through a registry file, which a packaged game loads instead of scanning"));
    {
        FAssetRegistry Scanned;
        Scanned.SetMounts(Mounts);
        Scanned.Rescan();

        const FString Filename = Root + TEXT("/") + FAssetRegistry::PrecomputedFilename;
        FString Error;
        TEST_EXPECT(Scanned.SaveToFile(Filename, Error));

        FAssetRegistry Loaded;
        Loaded.SetMounts(Mounts);

        int32 NumAdded = 0;
        Loaded.GetOnAssetAddedEvent().AddLambda([&NumAdded](const FAssetData&)
        {
            ++NumAdded;
        });

        TEST_EXPECT(Loaded.LoadFromFile(Filename, Error));
        TEST_EXPECT(NumAdded == Scanned.GetNumAssets() && Loaded.GetErrors().IsEmpty());

        const TArray<FAssetData> Expected = Scanned.GetAssets();
        const TArray<FAssetData> Actual   = Loaded.GetAssets();
        TEST_EXPECT(Expected.Size() == Actual.Size());
        for (int32 Index = 0; Index < Expected.Size() && Index < Actual.Size(); ++Index)
        {
            TEST_EXPECT(Expected[Index].Guid == Actual[Index].Guid && Expected[Index].Type == Actual[Index].Type);
            TEST_EXPECT(Expected[Index].Path.Equals(Actual[Index].Path) && Expected[Index].Name.Equals(Actual[Index].Name));
        }
    }

    TEST_SECTION(TEXT("A registry file that is missing, broken, newer or inconsistent is refused and leaves the index alone"));
    {
        FAssetRegistry Registry;
        Registry.SetMounts(Mounts);
        Registry.Rescan();
        const int32 NumAssets = Registry.GetNumAssets();

        const FString Valid = FString::Printf(TEXT("{ \"Version\": 1, \"Assets\": [ { \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Engine/A.dxrasset\" } ] }"), *NewTexture.ToString());
        TEST_EXPECT(WriteText(Root + TEXT("/Valid.json"), Valid));
        TEST_EXPECT(WriteText(Root + TEXT("/Newer.json"), TEXT("{ \"Version\": 2, \"Assets\": [] }")));
        TEST_EXPECT(WriteText(Root + TEXT("/Broken.json"), TEXT("{ \"Version\": 1, \"Assets\": [")));
        TEST_EXPECT(WriteText(Root + TEXT("/Unmounted.json"), FString::Printf(TEXT("{ \"Version\": 1, \"Assets\": [ { \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Other/A.dxrasset\" } ] }"), *NewTexture.ToString())));
        TEST_EXPECT(WriteText(Root + TEXT("/Twice.json"), FString::Printf(TEXT("{ \"Version\": 1, \"Assets\": [ { \"Guid\": \"%s\", \"Type\": \"Texture\", \"Path\": \"/Engine/A.dxrasset\" }, { \"Guid\": \"%s\", \"Type\": \"Mesh\", \"Path\": \"/Engine/B.dxrasset\" } ] }"), *NewTexture.ToString(), *NewTexture.ToString())));

        FString Error;
        TEST_EXPECT(!Registry.LoadFromFile(Root + TEXT("/Missing.json"), Error) && Error.Contains(TEXT("Could not open")));
        TEST_EXPECT(!Registry.LoadFromFile(Root + TEXT("/Newer.json"), Error) && Error.Contains(TEXT("newer engine")));
        TEST_EXPECT(!Registry.LoadFromFile(Root + TEXT("/Broken.json"), Error) && Error.Contains(TEXT("JSON")));
        TEST_EXPECT(!Registry.LoadFromFile(Root + TEXT("/Unmounted.json"), Error) && Error.Contains(TEXT("under a mount")));
        TEST_EXPECT(!Registry.LoadFromFile(Root + TEXT("/Twice.json"), Error) && Error.Contains(TEXT("twice")));
        TEST_EXPECT(Registry.GetNumAssets() == NumAssets);

        TEST_EXPECT(Registry.LoadFromFile(Root + TEXT("/Valid.json"), Error));
        TEST_EXPECT(Registry.GetNumAssets() == 1 && Registry.FindByPath(TEXT("/Engine/A.dxrasset")) && Registry.FindByPath(TEXT("/Engine/A.dxrasset"))->Name.Equals(TEXT("A")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
