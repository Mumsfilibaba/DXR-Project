#pragma once
#include "RHI/RHITexture.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FSkyboxComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSkyboxComponent, FSceneComponent);

    FSkyboxComponent(const FObjectInitializer& ObjectInitializer);
    ~FSkyboxComponent() = default;

    void SetCubeMap(const FRHITextureRef& InCubeMap);

    FORCEINLINE const FRHITextureRef& GetCubeMap() const 
    {
        return CubeMap;
    }

private:
    FRHITextureRef CubeMap;
};
