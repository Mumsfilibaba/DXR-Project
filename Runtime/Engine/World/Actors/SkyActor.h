#pragma once
#include "Engine/World/Actors/Actor.h"
#include "Engine/Resources/Environment.h"

class FSkyboxComponent;
class FSkyLightComponent;

class ENGINE_API FSkyActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FSkyActor, FActor);

    FSkyActor(const FObjectInitializer& ObjectInitializer);
    ~FSkyActor();

    using FActor::Initialize;
    void Initialize(const TSharedPtr<FEnvironment>& InEnvironment);

    virtual const CHAR_T* GetTypeLabel() const override
    {
        return TEXT("Sky");
    }

    FSkyboxComponent* GetSkyboxComponent() const
    {
        return SkyboxComponent;
    }

    FSkyLightComponent* GetSkyLightComponent() const
    {
        return SkyLightComponent;
    }

private:
    FSkyboxComponent*   SkyboxComponent;
    FSkyLightComponent* SkyLightComponent;
};
