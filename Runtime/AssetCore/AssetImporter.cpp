#include "AssetCore/AssetImporter.h"
#include "AssetCore/AssetFile.h"
#include "AssetCore/AssetMetadata.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Platform/PlatformFile.h"

static constexpr uint64 FnvOffsetBasis = 0xcbf29ce484222325ull;
static constexpr uint64 FnvPrime       = 0x100000001b3ull;
static constexpr uint32 HashBlockSize  = 1024 * 1024;

static FAssetImportResult MakeError(EAssetImportError Error, const FString& Message)
{
    FAssetImportResult Result;
    Result.Error   = Error;
    Result.Message = Message;
    return Result;
}

static FString GetMountName(const FContentMount& Mount)
{
    return Mount.Root.SubString(1, Mount.Root.Length() - 2);
}

static FString GetMountRootDirectory(const FContentMount& Mount)
{
    return File::GetDirectoryOf(Mount.Directory);
}

static bool IsValidAssetName(const FString& Name)
{
    if (Name.IsEmpty() || Name.StartsWith(TEXT("/")) || Name.EndsWith(TEXT("/")) || Name.Contains(TEXT("//")))
    {
        return false;
    }

    for (const CHAR_T Char : { '\\', ':', '*', '?', '"', '<', '>', '|' })
    {
        if (Name.Contains(Char))
        {
            return false;
        }
    }

    const FString Wrapped = FString(TEXT("/")) + Name + TEXT("/");
    return !Wrapped.Contains(TEXT("/./")) && !Wrapped.Contains(TEXT("/../"));
}

static const CHAR_T* GetFileExtension(EAssetType Type)
{
    const CHAR_T* TextExtension = AssetTypes::GetTextExtension(Type);
    return TextExtension ? TextExtension : FAssetFile::FileExtension;
}

bool FAssetImporter::ComputeSourceHash(const FString& DiskPath, FString& OutHash)
{
    TFileRef<IPlatformFile> SourceFile = FPlatformFile::OpenForRead(DiskPath);
    if (!SourceFile.IsValid())
    {
        return false;
    }

    TArray<uint8> Block;
    Block.Resize(HashBlockSize);

    uint64 Hash      = FnvOffsetBasis;
    int64  Remaining = SourceFile->Size();

    while (Remaining > 0)
    {
        const uint32 BytesToRead = static_cast<uint32>(Remaining < HashBlockSize ? Remaining : HashBlockSize);
        if (SourceFile->Read(Block.Data(), BytesToRead) != static_cast<int32>(BytesToRead))
        {
            return false;
        }

        for (uint32 Index = 0; Index < BytesToRead; ++Index)
        {
            Hash = (Hash ^ Block[Index]) * FnvPrime;
        }

        Remaining -= BytesToRead;
    }

    OutHash = FString::Printf(TEXT("%016llx"), Hash);
    return true;
}

FString FAssetImporter::ComputeImportHash(const FJsonValue& Document)
{
    FJsonValue Hashed = Document;
    if (Hashed.IsObject())
    {
        Hashed.RemoveMember(ImportHashMember);
    }

    const FString                             Text = Json::ToString(Hashed);
    const TStringConversion<ANSICHAR, CHAR_T> Utf8 = StringCast<ANSICHAR>(Text);

    uint64 Hash = FnvOffsetBasis;
    for (int32 Index = 0; Index < Utf8.Length(); ++Index)
    {
        Hash = (Hash ^ static_cast<uint8>(Utf8.Get()[Index])) * FnvPrime;
    }

    return FString::Printf(TEXT("%016llx"), Hash);
}

bool FAssetImporter::IsEditedTextAsset(const FString& DiskPath)
{
    FJsonValue Document;
    FString    RecordedHash;

    const FJsonValue* HashValue = Json::LoadFromFile(DiskPath, Document) && Document.IsObject() ? Document.Find(ImportHashMember) : nullptr;
    if (!HashValue || !HashValue->TryGetString(RecordedHash))
    {
        return true;
    }

    return !RecordedHash.Equals(ComputeImportHash(Document));
}

FAssetImporter::FAssetImporter(const FContentMounts& InMounts)
    : Mounts(InMounts)
    , Factories()
{
}

FAssetImporter::~FAssetImporter() = default;

void FAssetImporter::RegisterFactory(const TSharedPtr<IAssetFactory>& Factory)
{
    if (Factory && !Factories.Contains(Factory))
    {
        Factories.Insert(0, Factory);
    }
}

void FAssetImporter::UnregisterFactory(const TSharedPtr<IAssetFactory>& Factory)
{
    Factories.Remove(Factory);
}

IAssetFactory* FAssetImporter::FindFactory(const FString& SourcePath, const FString& FactoryName) const
{
    const FString Extension = File::ExtractExtension(SourcePath);
    for (const TSharedPtr<IAssetFactory>& Factory : Factories)
    {
        if (!FactoryName.IsEmpty() && !FactoryName.Equals(Factory->GetName()))
        {
            continue;
        }

        for (const FString& FactoryExtension : Factory->GetExtensions())
        {
            if (Extension.Equals(FactoryExtension, EStringCaseType::NoCase))
            {
                return Factory.Get();
            }
        }
    }

    return nullptr;
}

FString FAssetImporter::MakeSourcePath(const FString& DiskPath) const
{
    const FString AbsolutePath = File::MakeAbsolute(DiskPath);

    const FContentMount* Deepest = nullptr;
    for (const FContentMount& Mount : Mounts.GetMounts())
    {
        const FString Root = GetMountRootDirectory(Mount);
        if (File::IsUnderDirectory(AbsolutePath, Root) && (!Deepest || Root.Length() > GetMountRootDirectory(*Deepest).Length()))
        {
            Deepest = &Mount;
        }
    }

    if (!Deepest)
    {
        return AbsolutePath;
    }

    return GetMountName(*Deepest) + TEXT(":") + File::MakeRelative(AbsolutePath, GetMountRootDirectory(*Deepest));
}

FString FAssetImporter::ResolveSourcePath(const FString& SourcePath) const
{
    const int32 Colon = SourcePath.FindChar(':');
    if (Colon != FString::InvalidIndex && Colon > 0)
    {
        const FString Name = SourcePath.SubString(0, Colon);
        for (const FContentMount& Mount : Mounts.GetMounts())
        {
            if (GetMountName(Mount).Equals(Name))
            {
                return File::NormalizePath(File::CombinePath(GetMountRootDirectory(Mount), SourcePath.SubString(Colon + 1, SourcePath.Length() - Colon - 1)));
            }
        }
    }

    // Not one of the roots, so it was recorded absolute, which may start with a drive such as "C:"
    return File::NormalizePath(SourcePath);
}

FAssetImportResult FAssetImporter::ImportFile(const FString& SourcePath, const FString& DestinationFolder, const FAssetImportOptions& Options)
{
    const FString AbsoluteSource = File::MakeAbsolute(SourcePath);
    if (!FPlatformFile::IsFile(*AbsoluteSource))
    {
        return MakeError(EAssetImportError::SourceMissing, FString::Printf(TEXT("'%s' doesn't exist"), *AbsoluteSource));
    }

    IAssetFactory* Factory = FindFactory(AbsoluteSource, Options.Factory);
    if (!Factory)
    {
        return MakeError(EAssetImportError::NoFactory, Options.Factory.IsEmpty()
            ? FString::Printf(TEXT("Nothing imports '%s' files, such as '%s'"), *File::ExtractExtension(AbsoluteSource), *AbsoluteSource)
            : FString::Printf(TEXT("No factory named '%s' imports '%s' files, such as '%s'"), *Options.Factory, *File::ExtractExtension(AbsoluteSource), *AbsoluteSource));
    }

    FString DestinationDirectory;
    if (!Mounts.ToDiskPath(DestinationFolder, DestinationDirectory))
    {
        return MakeError(EAssetImportError::InvalidDestination, FString::Printf(TEXT("'%s' isn't a folder under /Engine/, /Project/ or another mount"), *DestinationFolder));
    }

    FString ImportSource = AbsoluteSource;
    if (Options.bCopyToContentSources)
    {
        FAssetImportResult CopyResult = CopyToContentSources(*Factory, AbsoluteSource, DestinationFolder, ImportSource);
        if (CopyResult.Error != EAssetImportError::None)
        {
            return CopyResult;
        }
    }

    const FJsonValue Settings = Options.Settings.IsObject() ? Options.Settings : FJsonValue::MakeObject();
    return RunImport(*Factory, ImportSource, DestinationFolder, Settings, File::ExtractFilenameWithoutExtension(AbsoluteSource), Options.bReplaceExisting, Options.bOverwriteEditedAssets);
}

FAssetImportResult FAssetImporter::Reimport(const FString& AssetPath, const FString& LocatedSourcePath, bool bOverwriteEditedAssets)
{
    FString AssetFile;
    if (!Mounts.ToDiskPath(AssetPath, AssetFile))
    {
        return MakeError(EAssetImportError::InvalidDestination, FString::Printf(TEXT("'%s' isn't under /Engine/, /Project/ or another mount"), *AssetPath));
    }

    FAssetMetadata Metadata;
    FString        Error;

    if (!AssetMetadata::ReadFromFile(AssetFile, Metadata, Error))
    {
        return MakeError(EAssetImportError::WriteFailed, FString::Printf(TEXT("'%s' can't be reimported: %s"), *AssetPath, *Error));
    }

    const FString SourcePath = LocatedSourcePath.IsEmpty() ? ResolveSourcePath(Metadata.SourceFile) : File::MakeAbsolute(LocatedSourcePath);
    if (!FPlatformFile::IsFile(*SourcePath))
    {
        return MakeError(EAssetImportError::SourceMissing, LocatedSourcePath.IsEmpty() 
            ? FString::Printf(TEXT("'%s' was imported from '%s' (%s), which doesn't exist anymore. Locate the source to reimport it."), 
                *AssetPath, *Metadata.SourceFile, *SourcePath) 
            : FString::Printf(TEXT("'%s' doesn't exist"), *SourcePath));
    }

    IAssetFactory* Factory = FindFactory(SourcePath, Metadata.Factory);
    if (!Factory)
    {
        return MakeError(EAssetImportError::NoFactory, FString::Printf(TEXT("No factory named '%s' imports '%s' files, such as '%s'"), *Metadata.Factory, *File::ExtractExtension(SourcePath), *SourcePath));
    }

    // The asset's name can hold folders under the import's destination, such as "Textures/Brick"
    const FString NamedPath = FString(TEXT("/")) + Metadata.Name + FAssetFile::FileExtension;
    if (Metadata.Name.IsEmpty() || !AssetPath.EndsWith(NamedPath, EStringCaseType::NoCase))
    {
        return MakeError(EAssetImportError::InvalidDestination, FString::Printf(TEXT("'%s' records the name '%s', which isn't where it is"), *AssetPath, *Metadata.Name));
    }

    const FString DestinationFolder = AssetPath.SubString(0, AssetPath.Length() - NamedPath.Length());
    const FString ImportName        = Metadata.ImportName.IsEmpty() ? Metadata.Name : Metadata.ImportName;
    return RunImport(*Factory, SourcePath, DestinationFolder, Metadata.ImportSettings, ImportName, true, bOverwriteEditedAssets);
}

FAssetImportResult FAssetImporter::CopyToContentSources(IAssetFactory& Factory, const FString& SourcePath, const FString& DestinationFolder, FString& OutCopiedSource) const
{
    const FContentMount* Mount = Mounts.FindMount(DestinationFolder);
    if (!Mount)
    {
        return MakeError(EAssetImportError::InvalidDestination, FString::Printf(TEXT("'%s' isn't under a mount"), *DestinationFolder));
    }

    FString DestinationDirectory;
    Mounts.ToDiskPath(DestinationFolder, DestinationDirectory);

    // <Root>/ContentSources/ mirrors the folders under <Root>/Content/
    const FString FolderInMount = File::MakeRelative(DestinationDirectory, Mount->Directory);
    const FString CopyDirectory = File::NormalizePath(File::CombinePath(File::CombinePath(GetMountRootDirectory(*Mount), ContentSourcesFolder), FolderInMount));
    const FString SourceDirectory = File::GetDirectoryOf(SourcePath);

    TArray<FString> Files;
    Files.Add(SourcePath);

    Factory.GetCompanionFiles(SourcePath, Files);

    struct FCopy
    {
        FString From;
        FString To;
        bool    bExists;
    };

    // Everything is checked before anything is copied, so a conflict leaves no partial copy behind
    TArray<FCopy> Copies;
    for (const FString& FilePath : Files)
    {
        const FString AbsolutePath = File::MakeAbsolute(FilePath);
        if (!FPlatformFile::IsFile(*AbsolutePath))
        {
            return MakeError(EAssetImportError::SourceMissing, FString::Printf(TEXT("'%s', which '%s' uses, doesn't exist"), *AbsolutePath, *SourcePath));
        }

        if (!File::IsUnderDirectory(AbsolutePath, SourceDirectory))
        {
            return MakeError(EAssetImportError::CopyConflict, FString::Printf(TEXT("'%s', which '%s' uses, isn't in its folder, so it can't be copied next to it in %s"), *AbsolutePath, *SourcePath, ContentSourcesFolder));
        }

        FCopy& Copy = Copies.Emplace();
        Copy.From    = AbsolutePath;
        Copy.To      = File::CombinePath(CopyDirectory, File::MakeRelative(AbsolutePath, SourceDirectory));
        Copy.bExists = FPlatformFile::IsFile(*Copy.To);

        FString FromHash;
        FString ToHash;

        if (Copy.bExists && (!ComputeSourceHash(Copy.From, FromHash) || !ComputeSourceHash(Copy.To, ToHash) || !FromHash.Equals(ToHash)))
        {
            return MakeError(EAssetImportError::CopyConflict, FString::Printf(TEXT("'%s' exists already with other content, and isn't overwritten"), *Copy.To));
        }
    }

    for (const FCopy& Copy : Copies)
    {
        if (Copy.bExists)
        {
            continue;
        }

        if (!File::CreateDirectoryTree(File::GetDirectoryOf(Copy.To)) || !FPlatformFile::CopyFile(*Copy.From, *Copy.To, false))
        {
            return MakeError(EAssetImportError::WriteFailed, FString::Printf(TEXT("Could not copy '%s' to '%s'"), *Copy.From, *Copy.To));
        }
    }

    OutCopiedSource = Copies[0].To;
    return FAssetImportResult();
}

FAssetImportResult FAssetImporter::RunImport(IAssetFactory& Factory, const FString& SourcePath, const FString& DestinationFolder, const FJsonValue& Settings, const FString& ImportName, bool bReplaceExisting, bool bOverwriteEditedAssets) const
{
    FString DestinationDirectory;
    if (!Mounts.ToDiskPath(DestinationFolder, DestinationDirectory))
    {
        return MakeError(EAssetImportError::InvalidDestination, FString::Printf(TEXT("'%s' isn't a folder under a mount"), *DestinationFolder));
    }

    FAssetImportContext    Context(SourcePath, DestinationFolder, DestinationDirectory, Settings, ImportName);
    TArray<FImportedAsset> Assets;
    FJsonValue             UsedSettings;
    FString                Error;

    if (!Factory.Import(Context, Assets, UsedSettings, Error))
    {
        return MakeError(EAssetImportError::ImportFailed, FString::Printf(TEXT("The %s factory could not import '%s': %s"), Factory.GetName(), *SourcePath, *Error));
    }

    if (Assets.IsEmpty())
    {
        return MakeError(EAssetImportError::ImportFailed, FString::Printf(TEXT("The %s factory imported nothing from '%s'"), Factory.GetName(), *SourcePath));
    }

    // Everything is checked before anything is written
    const TArray<EAssetType> AllowedTypes = Factory.GetAssetTypes();
    TArray<FJsonValue>       TextDocuments;
    TextDocuments.Resize(Assets.Size());

    for (int32 Index = 0; Index < Assets.Size(); ++Index)
    {
        FImportedAsset& Asset    = Assets[Index];
        const FString   FileName = Asset.Name + GetFileExtension(Asset.Type);

        if (!IsValidAssetName(Asset.Name) || !AllowedTypes.Contains(Asset.Type))
        {
            return MakeError(EAssetImportError::ImportFailed, FString::Printf(TEXT("The %s factory returned an asset named '%s' of type %s, which it can't produce"), Factory.GetName(), *Asset.Name, AssetTypes::GetName(Asset.Type)));
        }

        for (int32 Previous = 0; Previous < Index; ++Previous)
        {
            if ((Assets[Previous].Name + GetFileExtension(Assets[Previous].Type)).Equals(FileName, EStringCaseType::NoCase))
            {
                return MakeError(EAssetImportError::ImportFailed, FString::Printf(TEXT("The %s factory returned two assets named '%s'"), Factory.GetName(), *FileName));
            }
        }

        if (!bReplaceExisting && FPlatformFile::IsFile(*File::CombinePath(DestinationDirectory, FileName)))
        {
            return MakeError(EAssetImportError::DestinationExists, FString::Printf(TEXT("'%s' exists already; reimport it, or replace it"), *File::CombinePath(DestinationFolder, FileName)));
        }

        if (!Asset.Guid.IsValid())
        {
            Asset.Guid = Context.GetGuid(Asset.Name, Asset.Type);
        }

        if (AssetTypes::GetTextExtension(Asset.Type))
        {
            const FString Text = StringCast<CHAR_T>(reinterpret_cast<const ANSICHAR*>(Asset.Data.Data()), Asset.Data.Size()).ToString();
            if (!Json::Parse(FStringView(Text), TextDocuments[Index]) || !TextDocuments[Index].IsObject())
            {
                return MakeError(EAssetImportError::ImportFailed, FString::Printf(TEXT("The %s factory returned '%s', which isn't a JSON object"), Factory.GetName(), *FileName));
            }

            TextDocuments[Index].RemoveMember(ImportHashMember);
            TextDocuments[Index].AddMember(ImportHashMember, FJsonValue(ComputeImportHash(TextDocuments[Index])));
        }
    }

    FAssetMetadata Metadata;
    Metadata.ImportName     = ImportName;
    Metadata.SourceFile     = MakeSourcePath(SourcePath);
    Metadata.Factory        = Factory.GetName();
    Metadata.ImportSettings = UsedSettings.IsObject() ? UsedSettings : Settings;

    if (!ComputeSourceHash(SourcePath, Metadata.SourceHash))
    {
        return MakeError(EAssetImportError::WriteFailed, FString::Printf(TEXT("Could not read '%s'"), *SourcePath));
    }

    FAssetImportResult Result;
    Result.Warnings = Context.GetWarnings();

    for (int32 Index = 0; Index < Assets.Size(); ++Index)
    {
        FImportedAsset& Asset       = Assets[Index];
        const FString   FileName    = Asset.Name + GetFileExtension(Asset.Type);
        const FString   AssetFile   = File::CombinePath(DestinationDirectory, FileName);
        const FString   VirtualPath = File::CombinePath(DestinationFolder, FileName);

        if (AssetTypes::GetTextExtension(Asset.Type))
        {
            if (!bOverwriteEditedAssets && FPlatformFile::IsFile(*AssetFile) && IsEditedTextAsset(AssetFile))
            {
                Result.KeptAssets.Add(VirtualPath);
                continue;
            }

            if (!File::CreateDirectoryTree(File::GetDirectoryOf(AssetFile)) || !Json::SaveToFile(AssetFile, TextDocuments[Index]))
            {
                return MakeError(EAssetImportError::WriteFailed, FString::Printf(TEXT("Could not write '%s'"), *AssetFile));
            }

            Result.WrittenAssets.Add(VirtualPath);
            continue;
        }

        Metadata.Name         = Asset.Name;
        Metadata.Dependencies = Asset.Dependencies;

        FAssetFileWriter Writer(Asset.Guid, Asset.Type);
        Writer.SetMetadata(AssetMetadata::ToJson(Metadata));
        Writer.SetChunk(AssetChunk::Data, ::Move(Asset.Data));
    
        if (!Asset.Thumbnail.IsEmpty())
        {
            Writer.SetChunk(AssetChunk::Thumbnail, ::Move(Asset.Thumbnail));
        }

        if (!File::CreateDirectoryTree(File::GetDirectoryOf(AssetFile)) || !Writer.SaveToFile(AssetFile, Error))
        {
            return MakeError(EAssetImportError::WriteFailed, Error.IsEmpty() ? FString::Printf(TEXT("Could not write '%s'"), *AssetFile) : Error);
        }

        Result.WrittenAssets.Add(VirtualPath);
    }

    return Result;
}
