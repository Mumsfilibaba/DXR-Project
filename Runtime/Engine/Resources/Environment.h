#pragma once
#include "AssetCore/Asset.h"
#include "RHI/RHIResources.h"
#include "Engine/EngineModule.h"

class ENGINE_API FEnvironment : public FAsset
{
public:
    FEnvironment(const FRHITextureRef& InSkyCubeMap, const FRHITextureRef& InSpecularCubeMap, const FRHITextureRef& InDiffuseCubeMap);
    ~FEnvironment();

    FORCEINLINE const FRHITextureRef& GetSkyCubeMap() const
    {
        return SkyCubeMap;
    }

    FORCEINLINE const FRHITextureRef& GetSpecularCubeMap() const
    {
        return SpecularCubeMap;
    }

    FORCEINLINE const FRHITextureRef& GetDiffuseCubeMap() const
    {
        return DiffuseCubeMap;
    }

private:
    FRHITextureRef SkyCubeMap;
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;
};

template<>
struct TAssetTypeOf<FEnvironment>
{
    static constexpr EAssetType Value = EAssetType::Environment;
};
