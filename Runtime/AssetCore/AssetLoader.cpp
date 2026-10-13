#include "AssetCore/AssetLoader.h"

FAssetLoader::FAssetLoader(const FAssetRegistry& InRegistry)
    : Registry(InRegistry)
    , Loaders()
    , Loaded()
{
}

FAssetLoader::~FAssetLoader() = default;

void FAssetLoader::AddLoader(TUniquePtr<FLoader>&& Loader)
{
    for (TUniquePtr<FLoader>& Existing : Loaders)
    {
        if (Existing->Type == Loader->Type)
        {
            Existing = ::Move(Loader);
            return;
        }
    }

    Loaders.Add(::Move(Loader));
}

bool FAssetLoader::UnregisterLoader(EAssetType Type)
{
    for (int32 Index = 0; Index < Loaders.Size(); ++Index)
    {
        if (Loaders[Index]->Type == Type)
        {
            Loaders.RemoveAt(Index);
            return true;
        }
    }

    return false;
}

FAssetLoader::FLoader* FAssetLoader::FindLoader(EAssetType Type) const
{
    for (const TUniquePtr<FLoader>& Loader : Loaders)
    {
        if (Loader->Type == Type)
        {
            return Loader.Get();
        }
    }

    return nullptr;
}

const FAssetLoader::FLoadedAsset* FAssetLoader::FindLoadedAsset(const FGuid& Guid, EAssetType Type, FString* OutError) const
{
    const FLoadedAsset* Existing = Loaded.Find(Guid);
    if (!Existing)
    {
        return nullptr;
    }

    if (Existing->Type != Type)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s is loaded as a %s, not a %s"), *Guid.ToString(), AssetTypes::GetName(Existing->Type), AssetTypes::GetName(Type));
        }

        return nullptr;
    }

    return Existing;
}

bool FAssetLoader::ResolveForLoad(const FGuid& Guid, EAssetType Type, FAssetData& OutAsset, FString& OutDiskPath, FString* OutError) const
{
    const FAssetData* Asset = Registry.FindByGuid(Guid);
    if (!Asset)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s isn't in the asset registry"), *Guid.ToString());
        }

        return false;
    }

    if (Asset->Type != Type)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s is a %s, not a %s"), *FAssetRef::FromAsset(*Asset).ToString(), AssetTypes::GetName(Asset->Type), AssetTypes::GetName(Type));
        }

        return false;
    }

    if (!Registry.GetMounts().ToDiskPath(Asset->Path, OutDiskPath))
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("The asset %s isn't under any mount"), *FAssetRef::FromAsset(*Asset).ToString());
        }

        return false;
    }

    OutAsset = *Asset;
    return true;
}

void FAssetLoader::AddLoaded(const FGuid& Guid, EAssetType Type, const TSharedPtr<FAsset>& Object)
{
    FLoadedAsset& Entry = Loaded.Add(Guid);
    Entry.Type   = Type;
    Entry.Object = Object;
}

bool FAssetLoader::Unload(const FGuid& Guid)
{
    return Loaded.RemoveKey(Guid);
}

void FAssetLoader::UnloadAll()
{
    Loaded.Clear();
}

bool FAssetLoader::IsLoaded(const FGuid& Guid) const
{
    return Loaded.Contains(Guid);
}
