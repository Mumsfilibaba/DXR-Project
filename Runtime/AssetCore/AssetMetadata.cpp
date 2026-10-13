#include "AssetCore/AssetMetadata.h"
#include "AssetCore/AssetFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"

static bool ReadGuidArray(const FJsonValue* Value, TArray<FGuid>& OutGuids, FString& OutError)
{
    OutGuids.Clear();
    if (!Value)
    {
        return true;
    }

    if (!Value->IsArray())
    {
        OutError = FString(TEXT("Dependencies isn't an array"));
        return false;
    }

    for (int32 Index = 0; Index < Value->Num(); ++Index)
    {
        FString Text;
        FGuid   Guid;
        if (!(*Value)[Index].TryGetString(Text) || !FGuid::Parse(Text, Guid))
        {
            OutError = FString::Printf(TEXT("Dependencies[%d] isn't a GUID"), Index);
            return false;
        }

        OutGuids.Add(Guid);
    }

    return true;
}

static bool ReadOptionalString(const FJsonValue& Object, const CHAR_T* Name, FString& OutValue, FString& OutError)
{
    const FJsonValue* Value = Object.Find(Name);
    if (Value && !Value->TryGetString(OutValue))
    {
        OutError = FString::Printf(TEXT("%s isn't a string"), Name);
        return false;
    }

    return true;
}

FJsonValue AssetMetadata::ToJson(const FAssetMetadata& Metadata)
{
    FJsonValue Dependencies = FJsonValue::MakeArray();
    for (const FGuid& Guid : Metadata.Dependencies)
    {
        Dependencies.Add(FJsonValue(Guid.ToString()));
    }

    FJsonValue Value = FJsonValue::MakeObject();
    Value.AddMember(TEXT("Name"), FJsonValue(Metadata.Name));
    Value.AddMember(TEXT("ImportName"), FJsonValue(Metadata.ImportName));
    Value.AddMember(TEXT("SourceFile"), FJsonValue(Metadata.SourceFile));
    Value.AddMember(TEXT("SourceHash"), FJsonValue(Metadata.SourceHash));
    Value.AddMember(TEXT("Factory"), FJsonValue(Metadata.Factory));
    Value.AddMember(TEXT("ImportSettings"), Metadata.ImportSettings.IsObject() ? FJsonValue(Metadata.ImportSettings) : FJsonValue::MakeObject());
    Value.AddMember(TEXT("Dependencies"), ::Move(Dependencies));
    return Value;
}

bool AssetMetadata::FromJson(const FJsonValue& Value, FAssetMetadata& OutMetadata, FString& OutError)
{
    if (!Value.IsObject())
    {
        OutError = FString(TEXT("The metadata isn't a JSON object"));
        return false;
    }

    FAssetMetadata Metadata;
    if (!ReadOptionalString(Value, TEXT("Name"), Metadata.Name, OutError) ||
        !ReadOptionalString(Value, TEXT("ImportName"), Metadata.ImportName, OutError) ||
        !ReadOptionalString(Value, TEXT("SourceFile"), Metadata.SourceFile, OutError) ||
        !ReadOptionalString(Value, TEXT("SourceHash"), Metadata.SourceHash, OutError) ||
        !ReadOptionalString(Value, TEXT("Factory"), Metadata.Factory, OutError) ||
        !ReadGuidArray(Value.Find(TEXT("Dependencies")), Metadata.Dependencies, OutError))
    {
        return false;
    }

    const FJsonValue* Settings = Value.Find(TEXT("ImportSettings"));
    if (Settings && !Settings->IsObject())
    {
        OutError = FString(TEXT("ImportSettings isn't an object"));
        return false;
    }

    Metadata.ImportSettings = Settings ? *Settings : FJsonValue::MakeObject();
    OutMetadata = ::Move(Metadata);
    return true;
}

bool AssetMetadata::ReadFromFile(const FString& DiskPath, FAssetMetadata& OutMetadata, FString& OutError)
{
    FAssetFileReader Reader;
    FJsonValue       Value;

    if (!Reader.Open(DiskPath, OutError) || !Reader.ReadMetadata(Value, OutError))
    {
        return false;
    }

    if (!FromJson(Value, OutMetadata, OutError))
    {
        OutError = FString::Printf(TEXT("The metadata of '%s' isn't valid: %s"), *DiskPath, *OutError);
        return false;
    }

    return true;
}

bool AssetMetadata::ReadDependencies(const FString& DiskPath, TArray<FGuid>& OutDependencies, FString& OutError)
{
    if (AssetTypes::FindByTextExtension(File::ExtractExtension(DiskPath)) == EAssetType::Unknown)
    {
        FAssetMetadata Metadata;
        if (!ReadFromFile(DiskPath, Metadata, OutError))
        {
            return false;
        }

        OutDependencies = ::Move(Metadata.Dependencies);
        return true;
    }

    FJsonValue Document;
    FJsonError JsonError;

    if (!Json::LoadFromFile(DiskPath, Document, &JsonError))
    {
        OutError = FString::Printf(TEXT("'%s' isn't valid JSON: %s"), *DiskPath, *JsonError.ToString());
        return false;
    }

    if (!Document.IsObject() || !ReadGuidArray(Document.Find(TEXT("Dependencies")), OutDependencies, OutError))
    {
        OutError = FString::Printf(TEXT("'%s' has invalid dependencies: %s"), *DiskPath, Document.IsObject() ? *OutError : TEXT("it isn't a JSON object"));
        return false;
    }

    return true;
}
