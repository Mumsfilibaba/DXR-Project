#pragma once
#include "RHI/RHITexture.h"
#include "Engine/World/Components/LightComponent.h"

class ENGINE_API FSkyLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSkyLightComponent, FLightComponent);

    FSkyLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FSkyLightComponent();

    void SetCubeMap(const FRHITextureRef& InCubeMap);

    const FRHITextureRef& GetCubeMap() const
    {
        return CubeMap;
    }

private:
    FRHITextureRef CubeMap;
};
