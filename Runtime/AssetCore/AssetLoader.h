#pragma once
#include "AssetCore/Asset.h"
#include "AssetCore/AssetRegistry.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility/NonCopyable.h"

template<typename T>
using TAssetLoadFunction = TFunction<TSharedPtr<T>(const FAssetData& Asset, const FString& DiskPath, FString& OutError)>;

class ASSETCORE_API FAssetLoader : public FNonCopyable
{
public:
    explicit FAssetLoader(const FAssetRegistry& InRegistry);
    ~FAssetLoader();

    /** @brief Load assets of T's type with Function from now on, replacing any loader that type had */
    template<typename T>
    void RegisterLoader(TAssetLoadFunction<T> Function);

    /** @return Returns false if Type had no loader */
    bool UnregisterLoader(EAssetType Type);

    /**
     * @brief Load the asset with Guid as a T, or return it if it is loaded already
     * @param OutError Receives why it couldn't be loaded, when not nullptr: the GUID isn't registered, the asset isn't a T,
     * there is no loader for its type, or the loader failed
     * @return Returns the object, or an empty pointer
     */
    template<typename T>
    TSharedPtr<T> Load(const FGuid& Guid, FString* OutError = nullptr);

    template<typename T>
    TSharedPtr<T> Load(const TAssetRef<T>& Ref, FString* OutError = nullptr);

    /** @return Returns the loaded asset with Guid as a T, or an empty pointer when it isn't loaded or isn't a T */
    template<typename T>
    TSharedPtr<T> FindLoaded(const FGuid& Guid) const;

    /** @brief Stop keeping the asset, so loading it again loads it from its file. Others holding the object keep it. */
    bool Unload(const FGuid& Guid);

    /** @brief Stop keeping every loaded asset */
    void UnloadAll();

    NODISCARD bool IsLoaded(const FGuid& Guid) const;

    FORCEINLINE int32 GetNumLoaded() const
    {
        return Loaded.Size();
    }

private:
    struct FLoadedAsset
    {
        EAssetType         Type = EAssetType::Unknown;
        TSharedPtr<FAsset> Object;
    };

    struct FLoader
    {
        virtual ~FLoader() = default;

        EAssetType Type = EAssetType::Unknown;
    };

    template<typename T>
    struct TLoader : public FLoader
    {
        TAssetLoadFunction<T> Function;
    };

    void AddLoader(TUniquePtr<FLoader>&& Loader);
    FLoader* FindLoader(EAssetType Type) const;
    const FLoadedAsset* FindLoadedAsset(const FGuid& Guid, EAssetType Type, FString* OutError) const;
    bool ResolveForLoad(const FGuid& Guid, EAssetType Type, FAssetData& OutAsset, FString& OutDiskPath, FString* OutError) const;
    void AddLoaded(const FGuid& Guid, EAssetType Type, const TSharedPtr<FAsset>& Object);

    const FAssetRegistry&         Registry;
    TArray<TUniquePtr<FLoader>>   Loaders;
    TMap<FGuid, FLoadedAsset>     Loaded;
};

template<typename T>
void FAssetLoader::RegisterLoader(TAssetLoadFunction<T> Function)
{
    TUniquePtr<TLoader<T>> Loader = MakeUniquePtr<TLoader<T>>();
    Loader->Type     = TAssetTypeOf<T>::Value;
    Loader->Function = ::Move(Function);
    AddLoader(::Move(Loader));
}

template<typename T>
TSharedPtr<T> FAssetLoader::Load(const FGuid& Guid, FString* OutError)
{
    static_assert(TIsBaseOf<FAsset, T>::Value, "An asset type has to inherit FAsset, which keeps the asset it was loaded from");
    constexpr EAssetType Type = TAssetTypeOf<T>::Value;

    FString Error;
    if (const FLoadedAsset* Existing = FindLoadedAsset(Guid, Type, &Error))
    {
        return StaticCastSharedPtr<T>(Existing->Object);
    }

    // Loaded as another type, which FindLoadedAsset said
    if (!Error.IsEmpty())
    {
        if (OutError)
        {
            *OutError = Error;
        }

        return TSharedPtr<T>();
    }

    FAssetData Asset;
    FString    DiskPath;
    if (!ResolveForLoad(Guid, Type, Asset, DiskPath, OutError))
    {
        return TSharedPtr<T>();
    }

    TLoader<T>* Loader = static_cast<TLoader<T>*>(FindLoader(Type));
    if (!Loader)
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("There is no loader for %s assets, so %s can't be loaded"), AssetTypes::GetName(Type), *FAssetRef::FromAsset(Asset).ToString());
        }

        return TSharedPtr<T>();
    }

    TSharedPtr<T> Object = Loader->Function(Asset, DiskPath, Error);
    if (!Object.IsValid())
    {
        if (OutError)
        {
            *OutError = FString::Printf(TEXT("Could not load %s: %s"), *FAssetRef::FromAsset(Asset).ToString(), *Error);
        }

        return TSharedPtr<T>();
    }

    Object->SetAssetRef(FAssetRef::FromAsset(Asset));
    AddLoaded(Guid, Type, Object);
    return Object;
}

template<typename T>
TSharedPtr<T> FAssetLoader::Load(const TAssetRef<T>& Ref, FString* OutError)
{
    return Load<T>(Ref.GetGuid(), OutError);
}

template<typename T>
TSharedPtr<T> FAssetLoader::FindLoaded(const FGuid& Guid) const
{
    const FLoadedAsset* Existing = FindLoadedAsset(Guid, TAssetTypeOf<T>::Value, nullptr);
    return Existing ? StaticCastSharedPtr<T>(Existing->Object) : TSharedPtr<T>();
}
