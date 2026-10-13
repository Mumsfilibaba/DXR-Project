#pragma once
#include "AssetCore/AssetRef.h"

class ASSETCORE_API FAsset
{
public:

    /** @brief Remember the asset this object was loaded from, or forget it with an unset reference */
    void SetAssetRef(const FAssetRef& InAssetRef);

    /** @return Returns true if the object was loaded from an asset */
    FORCEINLINE bool IsAsset() const
    {
        return AssetRef.IsSet();
    }

    /** @return Returns the asset the object was loaded from, unset when it wasn't loaded from one */
    FORCEINLINE const FAssetRef& GetAssetRef() const
    {
        return AssetRef;
    }

private:
    FAssetRef AssetRef;
};
