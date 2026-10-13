#include "AssetCore/AssetFactory.h"
#include "AssetCore/AssetFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

FAssetImportContext::FAssetImportContext(const FString& InSourcePath, const FString& InDestinationDirectory, const FJsonValue& InSettings, const FString& InImportName)
    : SourcePath(InSourcePath)
    , DestinationDirectory(InDestinationDirectory)
    , Settings(InSettings)
    , ImportName(InImportName)
    , Guids()
{
}

FAssetImportContext::~FAssetImportContext() = default;

FGuid FAssetImportContext::GetGuid(const FString& Name)
{
    if (const FGuid* Known = Guids.Find(Name))
    {
        return *Known;
    }

    FGuid            Guid;
    FAssetFileReader Reader;
    FString          Error;
    
    const FString AssetFile = File::CombinePath(DestinationDirectory, Name + FAssetFile::FileExtension);
    if (FPlatformFile::IsFile(*AssetFile) && Reader.Open(AssetFile, Error) && Reader.GetGuid().IsValid())
    {
        Guid = Reader.GetGuid();
    }
    else
    {
        Guid = FGuid::Create();
    }

    Guids.Add(Name, Guid);
    return Guid;
}
