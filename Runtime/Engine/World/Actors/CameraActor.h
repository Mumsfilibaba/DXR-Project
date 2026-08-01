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
    void Initialize(const Vector3& InPosition, const Vector3& InRotation);

    FCameraComponent* GetCameraComponent() const
    {
        return CameraComponent;
    }

private:
    FCameraComponent* CameraComponent;
};
