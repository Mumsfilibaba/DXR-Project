#pragma once
#include "Engine/World/Actors/Actor.h"
#include "Engine/Resources/Environment.h"

class FSkyLightComponent;

class ENGINE_API FSkyLightActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FSkyLightActor, FActor);

    FSkyLightActor(const FObjectInitializer& ObjectInitializer);
    ~FSkyLightActor();

    using FActor::Initialize;
    void Initialize(const TSharedPtr<FEnvironment>& InEnvironment);

    virtual const CHAR_T* GetTypeLabel() const override
    {
        return TEXT("SkyLight");
    }

    FSkyLightComponent* GetLightComponent() const
    {
        return LightComponent;
    }

private:
    FSkyLightComponent* LightComponent;
};
