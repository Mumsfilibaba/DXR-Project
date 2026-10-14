#include "AssetImporterTests.h"

#include <AssetCore/AssetFile.h>
#include <AssetCore/AssetImporter.h>
#include <AssetCore/AssetMetadata.h>
#include <Core/Containers/StringConversion.h>
#include <Core/Filesystem/File.h>
#include <Core/Json/Json.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

class FTestImageFactory final : public IAssetFactory
{
public:
    virtual const CHAR_T* GetName() const override
    {
        return TEXT("TestImage");
    }

    virtual TArray<FString> GetExtensions() const override
    {
        return { FString(TEXT(".testimage")) };
    }

    virtual TArray<EAssetType> GetAssetTypes() const override
    {
        return { EAssetType::Texture };
    }

    virtual void GetCompanionFiles(const FString& SourcePath, TArray<FString>& OutFiles) const override
    {
        for (const FString& Line : ReadLines(SourcePath))
        {
            if (Line.StartsWith(TEXT("uses ")))
            {
                OutFiles.Add(File::CombinePath(File::GetDirectoryOf(SourcePath), Line.SubString(5, Line.Length() - 5)));
            }
        }
    }

    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) override
    {
        const TArray<FString> Lines = ReadLines(Context.GetSourcePath());
        if (Lines.Contains(FString(TEXT("fail"))))
        {
            OutError = TEXT("the test image says to fail");
            return false;
        }

        int64 Scale = 1;
        if (const FJsonValue* ScaleValue = Context.GetSettings().Find(TEXT("Scale")))
        {
            if (!ScaleValue->TryGetInt64(Scale))
            {
                OutError = TEXT("Scale isn't a number");
                return false;
            }
        }

        OutSettings = FJsonValue::MakeObject();
        OutSettings.AddMember(TEXT("Scale"), FJsonValue(Scale));

        FImportedAsset& Image = OutAssets.Emplace();
        Image.Name = Context.GetImportName();
        Image.Type = EAssetType::Texture;
        Image.Guid = Context.GetGuid(Image.Name, Image.Type);
        Image.Data = ReadBytes(Context.GetSourcePath());
        Image.Data.Add(static_cast<uint8>(Scale));
        Image.Thumbnail = { 0x89, 'P', 'N', 'G' };

        if (Lines.Contains(FString(TEXT("second"))))
        {
            FImportedAsset& Second = OutAssets.Emplace();
            Second.Name = Context.GetImportName() + TEXT("_Second");
            Second.Type = EAssetType::Texture;
            Second.Data = { 1, 2, 3 };
            Second.Dependencies.Add(Context.GetGuid(Context.GetImportName(), EAssetType::Texture));
        }

        return true;
    }

    static TArray<uint8> ReadBytes(const FString& Path)
    {
        TArray<uint8> Bytes;
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Path);
        if (FileHandle)
        {
            File::ReadFile(FileHandle.Get(), Bytes);
        }

        return Bytes;
    }

    static TArray<FString> ReadLines(const FString& Path)
    {
        const TArray<uint8> Bytes = ReadBytes(Path);
        const FString       Text  = StringCast<CHAR_T>(reinterpret_cast<const ANSICHAR*>(Bytes.Data()), Bytes.Size()).ToString();

        TArray<FString> Lines;
        FString         Line;
        for (int32 Index = 0; Index <= Text.Length(); ++Index)
        {
            if (Index == Text.Length() || Text[Index] == '\n')
            {
                Line = Line.Trim();
                if (!Line.IsEmpty())
                {
                    Lines.Add(Line);
                }

                Line.Clear();
            }
            else
            {
                Line += Text[Index];
            }
        }

        return Lines;
    }
};

class FTestBundleFactory final : public IAssetFactory
{
public:
    virtual const CHAR_T* GetName() const override
    {
        return TEXT("TestBundle");
    }

    virtual TArray<FString> GetExtensions() const override
    {
        return { FString(TEXT(".testbundle")) };
    }

    virtual TArray<EAssetType> GetAssetTypes() const override
    {
        return { EAssetType::Texture, EAssetType::Material };
    }

    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) override
    {
        const TArray<FString> Lines = FTestImageFactory::ReadLines(Context.GetSourcePath());
        if (Lines.IsEmpty())
        {
            OutError = TEXT("the test bundle is empty");
            return false;
        }

        const FString Name = Lines.Contains(FString(TEXT("badname"))) ? FString(TEXT("../Escape")) : Context.GetImportName();

        FImportedAsset& Image = OutAssets.Emplace();
        Image.Name = FString(TEXT("Images/")) + Name;
        Image.Type = EAssetType::Texture;
        Image.Guid = Context.GetGuid(Image.Name, Image.Type);
        Image.Data = { 1, 2, 3 };

        FImportedAsset& Material = OutAssets.Emplace();
        Material.Name = FString(TEXT("Materials/")) + Name;
        Material.Type = EAssetType::Material;
        Material.Guid = Context.GetGuid(Material.Name, Material.Type);

        FJsonValue Document = FJsonValue::MakeObject();
        Document.AddMember(TEXT("Version"), FJsonValue(int64(1)));
        Document.AddMember(TEXT("Guid"), FJsonValue(Material.Guid.ToString()));
        Document.AddMember(TEXT("Type"), FJsonValue(TEXT("Material")));
        Document.AddMember(TEXT("Value"), FJsonValue(Lines[0]));

        const FString Text = Json::ToString(Document);
        const TStringConversion<ANSICHAR, CHAR_T> Utf8 = StringCast<ANSICHAR>(Text);
        Material.Data.Reset(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());

        Context.AddWarning(TEXT("the test bundle has a warning"));
        OutSettings = FJsonValue::MakeObject();
        return true;
    }
};

class FOtherImageFactory final : public IAssetFactory
{
public:
    virtual const CHAR_T* GetName() const override
    {
        return TEXT("OtherImage");
    }

    virtual TArray<FString> GetExtensions() const override
    {
        return { FString(TEXT(".testimage")) };
    }

    virtual TArray<EAssetType> GetAssetTypes() const override
    {
        return { EAssetType::Texture };
    }

    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& /*OutError*/) override
    {
        FImportedAsset& Image = OutAssets.Emplace();
        Image.Name = Context.GetImportName();
        Image.Type = EAssetType::Texture;
        Image.Data = { 9 };

        OutSettings = FJsonValue::MakeObject();
        return true;
    }
};

static FString ReadValue(const FString& DiskPath, FGuid& OutGuid)
{
    FJsonValue Document;
    FString    Value;
    FString    GuidText;

    if (Json::LoadFromFile(DiskPath, Document) && Document.IsObject() && Document.Find(TEXT("Value")) && Document.Find(TEXT("Guid")))
    {
        if (!Document.Find(TEXT("Value"))->TryGetString(Value) || !Document.Find(TEXT("Guid"))->TryGetString(GuidText) || !FGuid::Parse(GuidText, OutGuid))
        {
            Value.Clear();
        }
    }

    return Value;
}

static bool SetValue(const FString& DiskPath, const FString& Value)
{
    FJsonValue Document;
    if (!Json::LoadFromFile(DiskPath, Document) || !Document.IsObject())
    {
        return false;
    }

    Document.RemoveMember(TEXT("Value"));
    Document.AddMember(TEXT("Value"), FJsonValue(Value));
    return Json::SaveToFile(DiskPath, Document);
}

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

static bool ReadAsset(const FString& DiskPath, FGuid& OutGuid, FAssetMetadata& OutMetadata, TArray<uint8>& OutData)
{
    FAssetFileReader Reader;
    FString          Error;
    if (!Reader.Open(DiskPath, Error) || !Reader.ReadChunk(AssetChunk::Data, OutData, Error))
    {
        return false;
    }

    OutGuid = Reader.GetGuid();
    return AssetMetadata::ReadFromFile(DiskPath, OutMetadata, Error);
}

bool AssetImporter_Test()
{
    TEST_BEGIN();

    const FString Root       = File::MakeAbsolute(TEXT("AssetImporterTests.Scratch"));
    const FString EngineDir  = Root + TEXT("/Engine");
    const FString ProjectDir = EngineDir + TEXT("/My Game");
    const FString Outside    = Root + TEXT("/Downloads");
    File::DeleteDirectoryTree(Root);

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::EngineRoot, EngineDir + TEXT("/Content")));
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, ProjectDir + TEXT("/Content")));
    TEST_EXPECT(File::CreateDirectoryTree(ProjectDir + TEXT("/Content")));

    FAssetImporter Importer(Mounts);
    Importer.RegisterFactory(MakeSharedPtr<FTestImageFactory>());

    TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("brick\n")));
    TEST_EXPECT(WriteText(EngineDir + TEXT("/Sources/Default.testimage"), TEXT("default\n")));
    TEST_EXPECT(WriteText(Outside + TEXT("/Stone.testimage"), TEXT("stone\nuses Maps/Stone Detail.txt\n")));
    TEST_EXPECT(WriteText(Outside + TEXT("/Maps/Stone Detail.txt"), TEXT("detail")));

    TEST_SECTION(TEXT("A factory is found by its extension"));
    {
        TEST_EXPECT(Importer.FindFactory(TEXT("C:/Art/Brick.TestImage")) != nullptr);
        TEST_EXPECT(Importer.FindFactory(TEXT("C:/Art/Brick.png")) == nullptr);

        const FAssetImportResult Result = Importer.ImportFile(Root + TEXT("/Unknown.png"), TEXT("/Project"));
        TEST_EXPECT(Result.Error == EAssetImportError::SourceMissing);

        TEST_EXPECT(WriteText(Root + TEXT("/Unknown.png"), TEXT("png")));
        TEST_EXPECT(Importer.ImportFile(Root + TEXT("/Unknown.png"), TEXT("/Project")).Error == EAssetImportError::NoFactory);
        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("/Nowhere")).Error == EAssetImportError::InvalidDestination);
    }

    TEST_SECTION(TEXT("An import writes the asset with its source, hash, settings and dependencies in META"));
    {
        const FAssetImportResult Result = Importer.ImportFile(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("/Project/Textures"));
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 1);
        TEST_EXPECT(Result.WrittenAssets.Size() == 1 && Result.WrittenAssets[0].Equals(TEXT("/Project/Textures/Brick.dxrasset")));

        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Guid, Metadata, Data));
        TEST_EXPECT(Guid.IsValid() && Metadata.Name.Equals(TEXT("Brick")) && Metadata.Factory.Equals(TEXT("TestImage")));
        TEST_EXPECT(Metadata.SourceFile.Equals(TEXT("Project:Sources/Brick.testimage")));
        TEST_EXPECT(Metadata.Dependencies.IsEmpty());

        FString Hash;
        TEST_EXPECT(FAssetImporter::ComputeSourceHash(ProjectDir + TEXT("/Sources/Brick.testimage"), Hash) && Metadata.SourceHash.Equals(Hash));

        const FJsonValue* Scale = Metadata.ImportSettings.Find(TEXT("Scale"));
        int64 ScaleValue = 0;
        TEST_EXPECT(Scale && Scale->TryGetInt64(ScaleValue) && ScaleValue == 1);
        TEST_EXPECT(Data.Size() == 7 && Data[6] == 1);
    }

    TEST_SECTION(TEXT("A source is recorded relative to the deepest root holding it, or absolute outside them"));
    {
        TEST_EXPECT(Importer.MakeSourcePath(EngineDir + TEXT("/Sources/Default.testimage")).Equals(TEXT("Engine:Sources/Default.testimage")));
        TEST_EXPECT(Importer.MakeSourcePath(ProjectDir + TEXT("/Sources/A.png")).Equals(TEXT("Project:Sources/A.png")));
        TEST_EXPECT(Importer.MakeSourcePath(Outside + TEXT("/Stone.testimage")).Equals(Outside + TEXT("/Stone.testimage")));

        TEST_EXPECT(Importer.ResolveSourcePath(TEXT("Engine:Sources/Default.testimage")).Equals(EngineDir + TEXT("/Sources/Default.testimage")));
        TEST_EXPECT(Importer.ResolveSourcePath(TEXT("Project:Sources/A.png")).Equals(ProjectDir + TEXT("/Sources/A.png")));
        TEST_EXPECT(Importer.ResolveSourcePath(Outside + TEXT("/Stone.testimage")).Equals(Outside + TEXT("/Stone.testimage")));
    }

    TEST_SECTION(TEXT("An existing asset is only replaced when asked, and keeps its GUID"));
    {
        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Guid, Metadata, Data));

        const FAssetImportResult Refused = Importer.ImportFile(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("/Project/Textures"));
        TEST_EXPECT(Refused.Error == EAssetImportError::DestinationExists && Refused.Message.Contains(TEXT("/Project/Textures/Brick.dxrasset")));

        FAssetImportOptions Options;
        Options.bReplaceExisting = true;
        Options.Settings = FJsonValue::MakeObject();
        Options.Settings.AddMember(TEXT("Scale"), FJsonValue(int64(3)));
        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("/Project/Textures"), Options).Error == EAssetImportError::None);

        FGuid ReplacedGuid;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), ReplacedGuid, Metadata, Data));
        TEST_EXPECT(ReplacedGuid == Guid && Data.Size() == 7 && Data[6] == 3);
    }

    TEST_SECTION(TEXT("Assets of one import can reference each other, and their dependencies are listed"));
    {
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Pair.testimage"), TEXT("pair\nsecond\n")));
        const FAssetImportResult Result = Importer.ImportFile(ProjectDir + TEXT("/Sources/Pair.testimage"), TEXT("/Project"));
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 2);

        FGuid          PairGuid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Pair.dxrasset"), PairGuid, Metadata, Data));

        TArray<FGuid> Dependencies;
        FString       Error;
        TEST_EXPECT(AssetMetadata::ReadDependencies(ProjectDir + TEXT("/Content/Pair_Second.dxrasset"), Dependencies, Error));
        TEST_EXPECT(Dependencies.Size() == 1 && Dependencies[0] == PairGuid);
        TEST_EXPECT(AssetMetadata::ReadDependencies(ProjectDir + TEXT("/Content/Pair.dxrasset"), Dependencies, Error) && Dependencies.IsEmpty());

        // Reimporting either asset updates both under the import's name, rather than naming new ones after the asset
        const FAssetImportResult Reimported = Importer.Reimport(TEXT("/Project/Pair_Second.dxrasset"));
        TEST_EXPECT(Reimported.Error == EAssetImportError::None && Reimported.WrittenAssets.Size() == 2);
        TEST_EXPECT(Reimported.WrittenAssets.Size() == 2 && Reimported.WrittenAssets[0].Equals(TEXT("/Project/Pair.dxrasset")) && Reimported.WrittenAssets[1].Equals(TEXT("/Project/Pair_Second.dxrasset")));
        TEST_EXPECT(Metadata.ImportName.Equals(TEXT("Pair")));
    }

    TEST_SECTION(TEXT("A text asset's dependencies are its Dependencies field"));
    {
        const FGuid Referenced(0x1u, 0x2u, 0x3u, 0x4u);
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Content/Brick.dxrmaterial"), FString::Printf(TEXT("{ \"Version\": 1, \"Guid\": \"%s\", \"Type\": \"Material\", \"Dependencies\": [ \"%s\" ] }"), *FGuid(0x9u, 0x9u, 0x9u, 0x9u).ToString(), *Referenced.ToString())));
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Content/Plain.dxrmaterial"), TEXT("{ \"Version\": 1, \"Type\": \"Material\" }")));
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Content/Broken.dxrmaterial"), TEXT("{ \"Dependencies\": [ 7 ] }")));

        TArray<FGuid> Dependencies;
        FString       Error;
        TEST_EXPECT(AssetMetadata::ReadDependencies(ProjectDir + TEXT("/Content/Brick.dxrmaterial"), Dependencies, Error) && Dependencies.Size() == 1 && Dependencies[0] == Referenced);
        TEST_EXPECT(AssetMetadata::ReadDependencies(ProjectDir + TEXT("/Content/Plain.dxrmaterial"), Dependencies, Error) && Dependencies.IsEmpty());
        TEST_EXPECT(!AssetMetadata::ReadDependencies(ProjectDir + TEXT("/Content/Broken.dxrmaterial"), Dependencies, Error) && Error.Contains(TEXT("Dependencies[0]")));
    }

    TEST_SECTION(TEXT("A source is copied with its companions to ContentSources, and the copy is what's recorded"));
    {
        FAssetImportOptions Options;
        Options.bCopyToContentSources = true;
        TEST_EXPECT(Importer.ImportFile(Outside + TEXT("/Stone.testimage"), TEXT("/Project/Textures/Rock"), Options).Error == EAssetImportError::None);

        const FString Copied = ProjectDir + TEXT("/ContentSources/Textures/Rock/Stone.testimage");
        TEST_EXPECT(FPlatformFile::IsFile(*Copied));
        TEST_EXPECT(FPlatformFile::IsFile(*(ProjectDir + TEXT("/ContentSources/Textures/Rock/Maps/Stone Detail.txt"))));

        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Rock/Stone.dxrasset"), Guid, Metadata, Data));
        TEST_EXPECT(Metadata.SourceFile.Equals(TEXT("Project:ContentSources/Textures/Rock/Stone.testimage")));

        // Copies that already hold the same bytes are fine
        Options.bReplaceExisting = true;
        TEST_EXPECT(Importer.ImportFile(Outside + TEXT("/Stone.testimage"), TEXT("/Project/Textures/Rock"), Options).Error == EAssetImportError::None);
    }

    TEST_SECTION(TEXT("A copy is never overwritten with other content, and a companion outside the source's folder isn't copied"));
    {
        TEST_EXPECT(WriteText(Outside + TEXT("/Maps/Stone Detail.txt"), TEXT("changed detail")));

        FAssetImportOptions Options;
        Options.bCopyToContentSources = true;
        Options.bReplaceExisting      = true;
        const FAssetImportResult Conflict = Importer.ImportFile(Outside + TEXT("/Stone.testimage"), TEXT("/Project/Textures/Rock"), Options);
        TEST_EXPECT(Conflict.Error == EAssetImportError::CopyConflict && Conflict.Message.Contains(TEXT("Stone Detail.txt")));
        TEST_EXPECT(FTestImageFactory::ReadBytes(ProjectDir + TEXT("/ContentSources/Textures/Rock/Maps/Stone Detail.txt")).Size() == 6);

        TEST_EXPECT(WriteText(Outside + TEXT("/Nested/Pebble.testimage"), TEXT("pebble\nuses ../Maps/Stone Detail.txt\n")));
        const FAssetImportResult Outsider = Importer.ImportFile(Outside + TEXT("/Nested/Pebble.testimage"), TEXT("/Project/Textures"), Options);
        TEST_EXPECT(Outsider.Error == EAssetImportError::CopyConflict && Outsider.Message.Contains(TEXT("isn't in its folder")));
        TEST_EXPECT(!FPlatformFile::IsFile(*(ProjectDir + TEXT("/ContentSources/Textures/Pebble.testimage"))));
    }

    TEST_SECTION(TEXT("A reimport keeps the GUID and the settings, and reads the source again"));
    {
        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Guid, Metadata, Data));

        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Brick.testimage"), TEXT("brick, repainted\n")));
        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset")).Error == EAssetImportError::None);

        FGuid          ReimportedGuid;
        FAssetMetadata Reimported;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), ReimportedGuid, Reimported, Data));
        TEST_EXPECT(ReimportedGuid == Guid && Data.Size() == 18 && Data[17] == 3);
        TEST_EXPECT(!Reimported.SourceHash.Equals(Metadata.SourceHash));
    }

    TEST_SECTION(TEXT("A reimport of a missing source says so, and a located source is recorded"));
    {
        TEST_EXPECT(FPlatformFile::MoveFile(*(ProjectDir + TEXT("/Sources/Brick.testimage")), *(Outside + TEXT("/Moved Brick.testimage"))));

        const FAssetImportResult Missing = Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset"));
        TEST_EXPECT(Missing.Error == EAssetImportError::SourceMissing);
        TEST_EXPECT(Missing.Message.Contains(TEXT("Project:Sources/Brick.testimage")) && Missing.Message.Contains(TEXT("Locate")));

        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset"), Outside + TEXT("/Missing.testimage")).Error == EAssetImportError::SourceMissing);
        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset"), Outside + TEXT("/Moved Brick.testimage")).Error == EAssetImportError::None);

        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Guid, Metadata, Data));
        TEST_EXPECT(Metadata.SourceFile.Equals(Outside + TEXT("/Moved Brick.testimage")));
        TEST_EXPECT(!FPlatformFile::IsFile(*(ProjectDir + TEXT("/Content/Textures/Moved Brick.dxrasset"))));
        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset")).Error == EAssetImportError::None);
        TEST_EXPECT(Importer.Reimport(TEXT("/Nowhere/Brick.dxrasset")).Error == EAssetImportError::InvalidDestination);
    }

    TEST_SECTION(TEXT("A failing factory writes nothing and says why"));
    {
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Bad.testimage"), TEXT("bad\nfail\n")));
        const FAssetImportResult Result = Importer.ImportFile(ProjectDir + TEXT("/Sources/Bad.testimage"), TEXT("/Project"));
        TEST_EXPECT(Result.Error == EAssetImportError::ImportFailed && Result.Message.Contains(TEXT("the test image says to fail")));
        TEST_EXPECT(!FPlatformFile::IsFile(*(ProjectDir + TEXT("/Content/Bad.dxrasset"))));
    }

    TEST_SECTION(TEXT("An import writes text assets and assets in folders, and a reimport keeps the text assets edited since"));
    {
        Importer.RegisterFactory(MakeSharedPtr<FTestBundleFactory>());
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Kit.testbundle"), TEXT("first\n")));

        const FString ImagePath    = TEXT("/Project/Kit/Images/Kit.dxrasset");
        const FString MaterialPath = TEXT("/Project/Kit/Materials/Kit.dxrmaterial");
        const FString MaterialFile = ProjectDir + TEXT("/Content/Kit/Materials/Kit.dxrmaterial");

        const FAssetImportResult Imported = Importer.ImportFile(ProjectDir + TEXT("/Sources/Kit.testbundle"), TEXT("/Project/Kit"));
        TEST_EXPECT(Imported.Error == EAssetImportError::None && Imported.WrittenAssets.Size() == 2);
        TEST_EXPECT(Imported.WrittenAssets.Size() == 2 && Imported.WrittenAssets[0].Equals(ImagePath) && Imported.WrittenAssets[1].Equals(MaterialPath));
        TEST_EXPECT(Imported.Warnings.Size() == 1 && Imported.Warnings[0].Contains(TEXT("warning")));
        TEST_EXPECT(FPlatformFile::IsFile(*MaterialFile) && !FAssetImporter::IsEditedTextAsset(MaterialFile));

        FGuid MaterialGuid;
        TEST_EXPECT(ReadValue(MaterialFile, MaterialGuid).Equals(TEXT("first")) && MaterialGuid.IsValid());

        const FAssetImportResult Reimported = Importer.Reimport(ImagePath);
        TEST_EXPECT(Reimported.Error == EAssetImportError::None && Reimported.WrittenAssets.Size() == 2 && Reimported.KeptAssets.IsEmpty());
        TEST_EXPECT(!FPlatformFile::IsFile(*(ProjectDir + TEXT("/Content/Kit/Images/Images/Kit.dxrasset"))));

        TEST_EXPECT(SetValue(MaterialFile, TEXT("edited")) && FAssetImporter::IsEditedTextAsset(MaterialFile));
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Kit.testbundle"), TEXT("second\n")));

        const FAssetImportResult Kept = Importer.Reimport(ImagePath);
        TEST_EXPECT(Kept.Error == EAssetImportError::None && Kept.WrittenAssets.Size() == 1 && Kept.WrittenAssets[0].Equals(ImagePath));
        TEST_EXPECT(Kept.KeptAssets.Size() == 1 && Kept.KeptAssets[0].Equals(MaterialPath));

        FGuid KeptGuid;
        TEST_EXPECT(ReadValue(MaterialFile, KeptGuid).Equals(TEXT("edited")) && KeptGuid == MaterialGuid);

        const FAssetImportResult Overwritten = Importer.Reimport(ImagePath, FString(), true);
        TEST_EXPECT(Overwritten.Error == EAssetImportError::None && Overwritten.WrittenAssets.Size() == 2 && Overwritten.KeptAssets.IsEmpty());

        FGuid OverwrittenGuid;
        TEST_EXPECT(ReadValue(MaterialFile, OverwrittenGuid).Equals(TEXT("second")) && OverwrittenGuid == MaterialGuid);
        TEST_EXPECT(!FAssetImporter::IsEditedTextAsset(MaterialFile));

        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Escape.testbundle"), TEXT("x\nbadname\n")));
        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Escape.testbundle"), TEXT("/Project/Kit")).Error == EAssetImportError::ImportFailed);
    }

    TEST_SECTION(TEXT("An import can name its factory among those for the extension, and a reimport uses the one that imported it"));
    {
        TSharedPtr<FOtherImageFactory> OtherFactory = MakeSharedPtr<FOtherImageFactory>();
        Importer.RegisterFactory(OtherFactory);
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Pick.testimage"), TEXT("pick\n")));

        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;

        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Pick.testimage"), TEXT("/Project/Picked")).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Picked/Pick.dxrasset"), Guid, Metadata, Data) && Metadata.Factory.Equals(TEXT("OtherImage")) && Data.Size() == 1);

        FAssetImportOptions Options;
        Options.Factory = TEXT("TestImage");
        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Pick.testimage"), TEXT("/Project/Named"), Options).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Named/Pick.dxrasset"), Guid, Metadata, Data) && Metadata.Factory.Equals(TEXT("TestImage")) && Data.Size() > 1);

        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Named/Pick.dxrasset")).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Named/Pick.dxrasset"), Guid, Metadata, Data) && Metadata.Factory.Equals(TEXT("TestImage")) && Data.Size() > 1);

        Options.Factory = TEXT("Nope");
        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Pick.testimage"), TEXT("/Project/Nope"), Options).Error == EAssetImportError::NoFactory);

        Importer.UnregisterFactory(OtherFactory);
    }

    TEST_SECTION(TEXT("An import can ask for an asset's GUID by name, and a reimport keeps it"));
    {
        const FGuid Requested = FGuid::Create();
        TEST_EXPECT(WriteText(ProjectDir + TEXT("/Sources/Fixed.testimage"), TEXT("fixed\n")));

        FAssetImportOptions Options;
        Options.Guids.Add(TEXT("Fixed"), Requested);

        FGuid          Guid;
        FAssetMetadata Metadata;
        TArray<uint8>  Data;

        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Fixed.testimage"), TEXT("/Project/Fixed"), Options).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Fixed/Fixed.dxrasset"), Guid, Metadata, Data) && Guid == Requested);

        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Fixed/Fixed.dxrasset")).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Fixed/Fixed.dxrasset"), Guid, Metadata, Data) && Guid == Requested);

        TEST_EXPECT(Importer.ImportFile(ProjectDir + TEXT("/Sources/Fixed.testimage"), TEXT("/Project/Unfixed")).Error == EAssetImportError::None);
        TEST_EXPECT(ReadAsset(ProjectDir + TEXT("/Content/Unfixed/Fixed.dxrasset"), Guid, Metadata, Data) && Guid.IsValid() && Guid != Requested);
    }

    TEST_SECTION(TEXT("The source hash is 64-bit FNV-1a"));
    {
        TEST_EXPECT(WriteText(Root + TEXT("/abc.txt"), TEXT("abc")));
        FString Hash;
        TEST_EXPECT(FAssetImporter::ComputeSourceHash(Root + TEXT("/abc.txt"), Hash) && Hash.Equals(TEXT("e71fa2190541574b")));
        TEST_EXPECT(!FAssetImporter::ComputeSourceHash(Root + TEXT("/Missing.txt"), Hash));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
