#include "AssetCore/AssetRegistry.h"
#include "AssetCore/AssetFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"

static bool IsAssetFile(const FString& Path)
{
    const FString Extension = File::ExtractExtension(Path);
    return Extension.Equals(FAssetFile::FileExtension, EStringCaseType::NoCase) || AssetTypes::FindByTextExtension(Extension) != EAssetType::Unknown;
}

static FString GetAssetName(const FString& VirtualPath)
{
    return File::ExtractFilenameWithoutExtension(VirtualPath);
}

static void SortByPath(TArray<FAssetData>& Assets)
{
    Assets.SortWithPredicate([](const FAssetData& Left, const FAssetData& Right)
    {
        return Left.Path.Compare(Right.Path) < 0;
    });
}

FAssetRegistry::FAssetRegistry()
    : Mounts()
    , Assets()
    , GuidsByPath()
    , Errors()
    , OnAssetAddedEvent()
    , OnAssetRemovedEvent()
    , OnAssetRenamedEvent()
{
}

FAssetRegistry::~FAssetRegistry() = default;

void FAssetRegistry::SetMounts(const FContentMounts& InMounts)
{
    Mounts = InMounts;
}

void FAssetRegistry::AddError(const FString& Error)
{
    LOG_WARNING("[FAssetRegistry]: %s", *Error);
    Errors.Add(Error);
}

bool FAssetRegistry::ReadAssetFile(const FString& DiskPath, const FString& VirtualPath, FAssetData& OutAsset)
{
    OutAsset.Path = VirtualPath;
    OutAsset.Name = GetAssetName(VirtualPath);

    const FString    Extension = File::ExtractExtension(DiskPath);
    const EAssetType TextType  = AssetTypes::FindByTextExtension(Extension);

    if (TextType == EAssetType::Unknown)
    {
        FAssetFileReader Reader;
        FString          Error;

        if (!Reader.Open(DiskPath, Error))
        {
            AddError(FString::Printf(TEXT("'%s' is left out: %s"), *VirtualPath, *Error));
            return false;
        }

        OutAsset.Guid = Reader.GetGuid();
        OutAsset.Type = Reader.GetType();
    }
    else
    {
        FJsonValue Document;
        FJsonError JsonError;

        if (!Json::LoadFromFile(DiskPath, Document, &JsonError))
        {
            AddError(FString::Printf(TEXT("'%s' is left out: it is not valid JSON (%s)"), *VirtualPath, *JsonError.ToString()));
            return false;
        }

        const FJsonValue* GuidValue = Document.IsObject() ? Document.Find(TEXT("Guid")) : nullptr;
        const FJsonValue* TypeValue = Document.IsObject() ? Document.Find(TEXT("Type")) : nullptr;

        FString GuidText;
        if (!GuidValue || !GuidValue->TryGetString(GuidText) || !FGuid::Parse(GuidText, OutAsset.Guid))
        {
            AddError(FString::Printf(TEXT("'%s' is left out: it has no valid Guid field"), *VirtualPath));
            return false;
        }

        FString TypeText;
        OutAsset.Type = (TypeValue && TypeValue->TryGetString(TypeText)) 
            ? AssetTypes::FindByName(TypeText) 
            : EAssetType::Unknown;

        if (OutAsset.Type != TextType)
        {
            AddError(FString::Printf(TEXT("'%s' is left out: its Type field is '%s' instead of '%s'"), *VirtualPath, *TypeText, AssetTypes::GetName(TextType)));
            return false;
        }
    }

    if (!OutAsset.Guid.IsValid())
    {
        AddError(FString::Printf(TEXT("'%s' is left out: its GUID is all zeros"), *VirtualPath));
        return false;
    }

    return true;
}

void FAssetRegistry::Rescan()
{
    Errors.Clear();

    struct FFoundFile
    {
        FString VirtualPath;
        FString DiskPath;
    };

    TArray<FFoundFile> Found;
    for (const FContentMount& Mount : Mounts.GetMounts())
    {
        if (!FPlatformFile::IsDirectory(*Mount.Directory))
        {
            continue;
        }

        File::IterateDirectoryTree(Mount.Directory, [this, &Found](const FString& Path, bool bIsDirectory)
        {
            FString VirtualPath;
            if (!bIsDirectory && IsAssetFile(Path) && Mounts.ToVirtualPath(Path, VirtualPath))
            {
                FFoundFile& FoundFile = Found.Emplace();
                FoundFile.VirtualPath = VirtualPath;
                FoundFile.DiskPath    = Path;
            }

            return true;
        });
    }

    // Sorted, so which of two files with one GUID is kept doesn't depend on the order the disk lists them in
    Found.SortWithPredicate([](const FFoundFile& Left, const FFoundFile& Right)
    {
        return Left.VirtualPath.Compare(Right.VirtualPath) < 0;
    });

    TMap<FGuid, FAssetData> NewAssets;
    for (int32 Index = 0; Index < Found.Size(); ++Index)
    {
        // A nested mount's folder is walked by its parent mount too
        if (Index > 0 && Found[Index].VirtualPath.Equals(Found[Index - 1].VirtualPath))
        {
            continue;
        }

        FAssetData Asset;
        if (!ReadAssetFile(Found[Index].DiskPath, Found[Index].VirtualPath, Asset))
        {
            continue;
        }

        if (const FAssetData* Existing = NewAssets.Find(Asset.Guid))
        {
            AddError(FString::Printf(TEXT("'%s' is left out: it has the GUID %s, which '%s' already has"), *Asset.Path, *Asset.Guid.ToString(), *Existing->Path));
            continue;
        }

        NewAssets.Add(Asset.Guid, Asset);
    }

    ApplyIndex(::Move(NewAssets));
    LOG_INFO("[FAssetRegistry]: Indexed %d assets in %d mounts, with %d errors", Assets.Size(), Mounts.GetMounts().Size(), Errors.Size());
}

void FAssetRegistry::ApplyIndex(TMap<FGuid, FAssetData>&& NewAssets)
{
    TArray<FAssetData> Removed;
    TArray<FAssetData> Added;
    TArray<FAssetData> Renamed;
    TArray<FString>    OldPaths;

    Assets.Foreach([&NewAssets, &Removed](const FGuid& Guid, const FAssetData& Asset)
    {
        if (!NewAssets.Contains(Guid))
        {
            Removed.Add(Asset);
        }
    });

    TArray<FAssetData> Sorted;
    NewAssets.Foreach([&Sorted](const FGuid&, const FAssetData& Asset)
    {
        Sorted.Add(Asset);
    });

    SortByPath(Removed);
    SortByPath(Sorted);

    for (const FAssetData& Asset : Sorted)
    {
        const FAssetData* Previous = Assets.Find(Asset.Guid);
        if (!Previous)
        {
            Added.Add(Asset);
        }
        else if (!Previous->Path.Equals(Asset.Path))
        {
            Renamed.Add(Asset);
            OldPaths.Add(Previous->Path);
        }
    }

    Assets = ::Move(NewAssets);
    GuidsByPath.Clear();

    Assets.Foreach([this](const FGuid& Guid, const FAssetData& Asset)
    {
        GuidsByPath.Add(Asset.Path, Guid);
    });

    for (const FAssetData& Asset : Removed)
    {
        OnAssetRemovedEvent.Broadcast(Asset);
    }

    for (int32 Index = 0; Index < Renamed.Size(); ++Index)
    {
        OnAssetRenamedEvent.Broadcast(Renamed[Index], OldPaths[Index]);
    }

    for (const FAssetData& Asset : Added)
    {
        OnAssetAddedEvent.Broadcast(Asset);
    }
}

bool FAssetRegistry::SaveToFile(const FString& Filename, FString& OutError) const
{
    FJsonValue Entries = FJsonValue::MakeArray();
    for (const FAssetData& Asset : GetAssets())
    {
        FJsonValue Entry = FJsonValue::MakeObject();
        Entry.AddMember(TEXT("Guid"), FJsonValue(Asset.Guid.ToString()));
        Entry.AddMember(TEXT("Type"), FJsonValue(AssetTypes::GetName(Asset.Type)));
        Entry.AddMember(TEXT("Path"), FJsonValue(Asset.Path));
        Entries.Add(::Move(Entry));
    }

    FJsonValue Document = FJsonValue::MakeObject();
    Document.AddMember(TEXT("Version"), FJsonValue(PrecomputedVersion));
    Document.AddMember(TEXT("Assets"), ::Move(Entries));

    if (!Json::SaveToFile(Filename, Document))
    {
        OutError = FString::Printf(TEXT("Could not write '%s'"), *Filename);
        return false;
    }

    return true;
}

bool FAssetRegistry::LoadFromFile(const FString& Filename, FString& OutError)
{
    FJsonValue Document;
    FJsonError JsonError;

    if (!Json::LoadFromFile(Filename, Document, &JsonError))
    {
        OutError = FPlatformFile::IsFile(*Filename) 
            ? FString::Printf(TEXT("'%s' is not valid JSON: %s"), *Filename, *JsonError.ToString())
            : FString::Printf(TEXT("Could not open '%s'"), *Filename);

        return false;
    }

    const FJsonValue* Version = Document.IsObject() ? Document.Find(TEXT("Version")) : nullptr;
    const FJsonValue* Entries = Document.IsObject() ? Document.Find(TEXT("Assets")) : nullptr;

    int64 VersionNumber = 0;
    if (!Version || !Version->TryGetInt64(VersionNumber) || !Entries || !Entries->IsArray())
    {
        OutError = FString::Printf(TEXT("'%s' is not an asset registry"), *Filename);
        return false;
    }

    if (VersionNumber > PrecomputedVersion)
    {
        OutError = FString::Printf(TEXT("'%s' is asset registry version %lld, which a newer engine wrote; this one reads up to %d"),
            *Filename, VersionNumber, PrecomputedVersion);
        return false;
    }

    TMap<FGuid, FAssetData> NewAssets;
    for (int32 Index = 0; Index < Entries->Num(); ++Index)
    {
        const FJsonValue& Entry = (*Entries)[Index];

        FString    GuidText;
        FString    TypeText;
        FAssetData Asset;

        const FJsonValue* GuidValue = Entry.IsObject() ? Entry.Find(TEXT("Guid")) : nullptr;
        const FJsonValue* TypeValue = Entry.IsObject() ? Entry.Find(TEXT("Type")) : nullptr;
        const FJsonValue* PathValue = Entry.IsObject() ? Entry.Find(TEXT("Path")) : nullptr;

        if (!GuidValue || !GuidValue->TryGetString(GuidText) || !FGuid::Parse(GuidText, Asset.Guid) || !Asset.Guid.IsValid() ||
            !TypeValue || !TypeValue->TryGetString(TypeText) || (Asset.Type = AssetTypes::FindByName(TypeText)) == EAssetType::Unknown ||
            !PathValue || !PathValue->TryGetString(Asset.Path) || !Mounts.FindMount(Asset.Path))
        {
            OutError = FString::Printf(TEXT("Asset %d of '%s' needs a GUID, a known Type and a Path under a mount"), Index, *Filename);
            return false;
        }

        if (NewAssets.Contains(Asset.Guid))
        {
            OutError = FString::Printf(TEXT("'%s' lists the GUID %s twice"), *Filename, *Asset.Guid.ToString());
            return false;
        }

        Asset.Name = GetAssetName(Asset.Path);
        NewAssets.Add(Asset.Guid, Asset);
    }

    Errors.Clear();
    ApplyIndex(::Move(NewAssets));
    return true;
}

const FAssetData* FAssetRegistry::FindByGuid(const FGuid& Guid) const
{
    return Assets.Find(Guid);
}

const FAssetData* FAssetRegistry::FindByPath(const FString& VirtualPath) const
{
    const FGuid* Guid = GuidsByPath.Find(VirtualPath);
    return Guid ? Assets.Find(*Guid) : nullptr;
}

TArray<FAssetData> FAssetRegistry::GetAssets() const
{
    TArray<FAssetData> Result;
    Assets.Foreach([&Result](const FGuid&, const FAssetData& Asset)
    {
        Result.Add(Asset);
    });

    SortByPath(Result);
    return Result;
}
