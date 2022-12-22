#include "Actor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components/Component.h"

FActorTransform::FActorTransform()
    : Matrix()
    , Translation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
{
    CalculateMatrix();
}

void FActorTransform::SetTranslation(float x, float y, float z)
{
    SetTranslation(FVector3(x, y, z));
}

void FActorTransform::SetTranslation(const FVector3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void FActorTransform::SetScale(float x, float y, float z)
{
    SetScale(FVector3(x, y, z));
}

void FActorTransform::SetScale(const FVector3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void FActorTransform::SetRotation(float x, float y, float z)
{
    SetRotation(FVector3(x, y, z));
}

void FActorTransform::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void FActorTransform::CalculateMatrix()
{
    FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale);
    FMatrix4 RotationMatrix    = FMatrix4::RotationRollPitchYaw(Rotation);
    FMatrix4 TranslationMatrix = FMatrix4::Translation(Translation);
    
    Matrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
    Matrix = Matrix.Transpose();
}


FActor::FActor(FScene* InSceneOwner)
    : FObject()
    , Name()
    , SceneOwner(InSceneOwner)
    , Transform()
    , Components()
    , bIsStartable(true)
    , bIsTickable(true)
{
    FOBJECT_INIT();
}

FActor::~FActor()
{
    for (FComponent* CurrentComponent : Components)
    {
        SAFE_DELETE(CurrentComponent);
    }

    Components.Clear();
}

void FActor::Start()
{
    for (FComponent* Component : Components)
    {
        if (Component->IsStartable())
        {
            Component->Start();
        }
    }
}

void FActor::Tick(FTimespan DeltaTime)
{
    for (FComponent* Component : Components)
    {
        if (Component->IsTickable())
        {
            Component->Tick(DeltaTime);
        }
    }
}

void FActor::AddComponent(FComponent* InComponent)
{
    CHECK(InComponent != nullptr);
    Components.Emplace(InComponent);

    if (SceneOwner)
    {
        SceneOwner->OnAddedComponent(InComponent);
    }
}

void FActor::SetName(const FString& InName)
{
    Name = InName;
}

bool FActor::HasComponentOfClass(class FClass* ComponentClass) const
{
    for (FComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return true;
        }
    }

    return false;
}

FComponent* FActor::GetComponentOfClass(class FClass* ComponentClass) const
{
    for (FComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return Component;
        }
    }

    return nullptr;
}
