#include "AssetCore/AssetRef.h"
#include "AssetCore/AssetRegistry.h"
#include "Core/Json/JsonArchive.h"
#include "Core/Json/JsonGuid.h"
#include "Core/Json/JsonSerializer.h"

FAssetRef FAssetRef::FromAsset(const FAssetData& Asset)
{
    return FAssetRef(Asset.Guid, Asset.Path);
}

FAssetRef::FAssetRef(const FGuid& InGuid, const FString& InPath)
    : Guid(InGuid)
    , Path(InPath)
{
}

FAssetRef::~FAssetRef() = default;

const FAssetData* FAssetRef::Resolve(const FAssetRegistry& Registry, FString* OutError) const
{
    return ResolveAs(Registry, EAssetType::Unknown, OutError);
}

const FAssetData* FAssetRef::ResolveAs(const FAssetRegistry& Registry, EAssetType ExpectedType, FString* OutError) const
{
    if (!IsSet())
    {
        if (OutError)
        {
            *OutError = FString(TEXT("No asset is referenced"));
        }

        return nullptr;
    }

    const FAssetData* Asset = Registry.FindByGuid(Guid);
    if (!Asset)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s isn't in the asset registry"), *ToString());
        }

        return nullptr;
    }

    if (ExpectedType != EAssetType::Unknown && Asset->Type != ExpectedType)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s is a %s, not a %s"), *ToString(), AssetTypes::GetName(Asset->Type), AssetTypes::GetName(ExpectedType));
        }

        return nullptr;
    }

    return Asset;
}

bool FAssetRef::UpdatePath(const FAssetRegistry& Registry)
{
    const FAssetData* Asset = IsSet() ? Registry.FindByGuid(Guid) : nullptr;
    if (!Asset || Asset->Path.Equals(Path))
    {
        return false;
    }

    Path = Asset->Path;
    return true;
}

FString FAssetRef::ToString() const
{
    if (!IsSet())
    {
        return FString(TEXT("unset"));
    }

    return Path.IsEmpty() ? Guid.ToString() : FString::Printf(TEXT("'%s' (%s)"), *Path, *Guid.ToString());
}

void FAssetRef::Serialize(FJsonArchive& Archive)
{
    if (Archive.IsSaving())
    {
        Archive.Field(TEXT("Guid"), Guid);
    }
    else
    {
        const int32 NumErrors = Archive.GetErrors().Size();
        if (!Archive.OptionalField(TEXT("Guid"), Guid) && Archive.GetErrors().Size() == NumErrors)
        {
            Archive.AddError(TEXT("An asset reference needs a Guid"));
        }
    }

    Archive.Field(TEXT("Path"), Path, FString());
}
