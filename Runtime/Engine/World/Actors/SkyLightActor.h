#pragma once
#include "Engine/World/Actors/Actor.h"
#include "RHI/RHITexture.h"

class FSkyLightComponent;

class ENGINE_API FSkyLightActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FSkyLightActor, FActor);

    FSkyLightActor(const FObjectInitializer& ObjectInitializer);
    ~FSkyLightActor();

    using FActor::Initialize;
    void Initialize(const FRHITextureRef& InCubeMap);

    FSkyLightComponent* GetLightComponent() const
    {
        return LightComponent;
    }

private:
    FSkyLightComponent* LightComponent;
};
