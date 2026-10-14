#include "AssetCore/AssetFactory.h"
#include "AssetCore/AssetFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Platform/PlatformFile.h"

FAssetImportContext::FAssetImportContext(const FString& InSourcePath, const FString& InDestinationFolder, const FString& InDestinationDirectory, const FJsonValue& InSettings, const FString& InImportName)
    : SourcePath(InSourcePath)
    , DestinationFolder(InDestinationFolder)
    , DestinationDirectory(InDestinationDirectory)
    , Settings(InSettings)
    , ImportName(InImportName)
    , Guids()
    , RequestedGuids()
    , Warnings()
{
}

FAssetImportContext::~FAssetImportContext() = default;

FGuid FAssetImportContext::GetGuid(const FString& Name, EAssetType Type)
{
    const CHAR_T* TextExtension = AssetTypes::GetTextExtension(Type);
    const FString FileName      = Name + (TextExtension ? TextExtension : FAssetFile::FileExtension);

    if (const FGuid* Known = Guids.Find(FileName))
    {
        return *Known;
    }

    if (const FGuid* Requested = RequestedGuids.Find(Name))
    {
        Guids.Add(FileName, *Requested);
        return *Requested;
    }

    FGuid Guid;

    const FString AssetFile = File::CombinePath(DestinationDirectory, FileName);
    if (FPlatformFile::IsFile(*AssetFile))
    {
        if (TextExtension)
        {
            FJsonValue        Document;
            FString           GuidText;
            const FJsonValue* GuidValue = Json::LoadFromFile(AssetFile, Document) && Document.IsObject() ? Document.Find(TEXT("Guid")) : nullptr;

            if (!GuidValue || !GuidValue->TryGetString(GuidText) || !FGuid::Parse(GuidText, Guid))
            {
                Guid = FGuid();
            }
        }
        else
        {
            FAssetFileReader Reader;
            FString          Error;

            if (Reader.Open(AssetFile, Error))
            {
                Guid = Reader.GetGuid();
            }
        }
    }

    if (!Guid.IsValid())
    {
        Guid = FGuid::Create();
    }

    Guids.Add(FileName, Guid);
    return Guid;
}

void FAssetImportContext::RequestGuid(const FString& Name, const FGuid& Guid)
{
    RequestedGuids.Add(Name, Guid);
}

FString FAssetImportContext::GetAssetPath(const FString& Name, EAssetType Type) const
{
    const CHAR_T* TextExtension = AssetTypes::GetTextExtension(Type);
    return File::CombinePath(DestinationFolder, Name + (TextExtension ? TextExtension : FAssetFile::FileExtension));
}

void FAssetImportContext::AddWarning(const FString& Warning)
{
    Warnings.Add(Warning);
}
