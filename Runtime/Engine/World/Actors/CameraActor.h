#pragma once
#include "Engine/World/Actors/Actor.h"

class FCameraComponent;

class ENGINE_API FCameraActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FCameraActor, FActor);

    FCameraActor(const FObjectInitializer& ObjectInitializer);
    ~FCameraActor();

    using FActor::Initialize;
    void Initialize(const FVector3& InPosition, const FVector3& InRotation);

    virtual const CHAR_T* GetTypeLabel() const override
    {
        return "Camera";
    }

    FCameraComponent* GetCameraComponent() const
    {
        return CameraComponent;
    }

private:
    FCameraComponent* CameraComponent;
};
