#include "Engine/World/Actors/CameraActor.h"
#include "Engine/World/Components/CameraComponent.h"

FOBJECT_IMPLEMENT_CLASS(FCameraActor);

FCameraActor::FCameraActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , CameraComponent(nullptr)
{
    SetName("Camera");
    GetTransform().SetTranslation(0.0f, 0.0f, -2.0f);

    CameraComponent = NewObject<FCameraComponent>();
    AddComponent(CameraComponent);
}

FCameraActor::~FCameraActor()
{
}

void FCameraActor::Initialize(const Vector3& InPosition, const Vector3& InRotation)
{
    GetTransform().SetTranslation(InPosition);
    GetTransform().SetRotation(InRotation);
}
