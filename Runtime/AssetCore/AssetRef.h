#pragma once
#include "AssetCore/AssetType.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonArchive.h"
#include "Core/Misc/Guid.h"

class FAssetRegistry;
struct FAssetData;

class ASSETCORE_API FAssetRef
{
public:

    /** @return Returns a reference to Asset, with its current path */
    NODISCARD static FAssetRef FromAsset(const FAssetData& Asset);

    FAssetRef() = default;

    explicit FAssetRef(const FGuid& InGuid, const FString& InPath = FString());
    ~FAssetRef();

    /**
     * @brief Find the asset in Registry
     * @param OutError Receives why it wasn't found, naming the reference, when not nullptr
     * @return Returns the asset, or nullptr when the reference is unset or its GUID isn't registered
     */
    const FAssetData* Resolve(const FAssetRegistry& Registry, FString* OutError = nullptr) const;

    /** @return Returns true if Path changed to the asset's current path, which it has after the asset was moved or renamed */
    bool UpdatePath(const FAssetRegistry& Registry);

    /** @return Returns the path and GUID, such as "'/Project/Textures/Brick.dxrasset' (6f9619ff...)", the GUID alone without a path, or "unset" */
    NODISCARD FString ToString() const;

    void Serialize(FJsonArchive& Archive);

    FORCEINLINE bool IsSet() const
    {
        return Guid.IsValid();
    }

    FORCEINLINE const FGuid& GetGuid() const
    {
        return Guid;
    }

    /** @return Returns the virtual path the asset had when the reference was made or last updated, for display and messages */
    FORCEINLINE const FString& GetPath() const
    {
        return Path;
    }

    /** @brief References are equal when they name the same GUID, whatever their paths */
    FORCEINLINE bool operator==(const FAssetRef& Other) const
    {
        return Guid == Other.Guid;
    }

protected:
    const FAssetData* ResolveAs(const FAssetRegistry& Registry, EAssetType ExpectedType, FString* OutError) const;

private:
    FGuid   Guid;
    FString Path;
};

template<typename T>
struct TAssetTypeOf;

template<typename T>
class TAssetRef : public FAssetRef
{
public:
    static constexpr EAssetType ExpectedType = TAssetTypeOf<T>::Value;

    using FAssetRef::FAssetRef;

    FORCEINLINE const FAssetData* Resolve(const FAssetRegistry& Registry, FString* OutError = nullptr) const
    {
        return ResolveAs(Registry, ExpectedType, OutError);
    }
};

template<>
struct TJsonOmitEmpty<FAssetRef>
{
    static bool IsEmpty(const FAssetRef& Value)
    {
        return !Value.IsSet();
    }
};

template<typename T>
struct TJsonOmitEmpty<TAssetRef<T>>
{
    static bool IsEmpty(const TAssetRef<T>& Value)
    {
        return !Value.IsSet();
    }
};
