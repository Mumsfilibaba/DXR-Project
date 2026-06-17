#include "Engine/World/World.h"
#include "Engine/World/Components/ActorComponent.h"
#include "Engine/World/Actors/Actor.h"

FOBJECT_IMPLEMENT_CLASS(FActor);

FActorTransform::FActorTransform()
    : TransformMatrix()
    , Translation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
{
    CalculateMatrix();
}

void FActorTransform::SetTranslation(float x, float y, float z)
{
    SetTranslation(Vector3(x, y, z));
}

void FActorTransform::SetTranslation(const Vector3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void FActorTransform::SetScale(float x, float y, float z)
{
    SetScale(Vector3(x, y, z));
}

void FActorTransform::SetScale(const Vector3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void FActorTransform::SetRotation(float x, float y, float z)
{
    SetRotation(Vector3(x, y, z));
}

void FActorTransform::SetRotation(const Vector3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void FActorTransform::CalculateMatrix()
{
    Matrix4 ScaleMatrix       = Matrix4::Scale(Scale);
    Matrix4 RotationMatrix    = Matrix4::RotationRollPitchYaw(Rotation);
    Matrix4 TranslationMatrix = Matrix4::Translation(Translation);

    TransformMatrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
}

FActor::FActor(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Name()
    , World(nullptr)
    , bIsStartable(true)
    , bIsTickable(true)
    , Transform()
    , Components()
{
}

FActor::~FActor()
{
    for (FActorComponent* CurrentComponent : Components)
    {
        SAFE_DELETE(CurrentComponent);
    }

    Components.Clear();
}

void FActor::Start()
{
    for (FActorComponent* Component : Components)
    {
        if (Component->IsStartable())
        {
            Component->Start();
        }
    }
}

void FActor::Tick(float DeltaTime)
{
    for (FActorComponent* Component : Components)
    {
        if (Component->IsTickable())
        {
            Component->Tick(DeltaTime);
        }
    }
}

void FActor::AddComponent(FActorComponent* InComponent)
{
    CHECK(InComponent != nullptr);
    CHECK(InComponent->GetActorOwner() == nullptr);

    // Set this actor as the owner
    InComponent->SetActorOwner(this);
    Components.Emplace(InComponent);

    if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(InComponent))
    {
        CHECK(World != nullptr);
        World->AddSceneComponent(SceneComponent);
    }
}

void FActor::SetName(const String& InName)
{
    Name = InName;
}

bool FActor::HasComponentOfClass(class FObjectClass* ComponentClass) const
{
    for (FActorComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return true;
        }
    }

    return false;
}

FActorComponent* FActor::GetComponentOfClass(class FObjectClass* ComponentClass) const
{
    for (FActorComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return Component;
        }
    }

    return nullptr;
}
