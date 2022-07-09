#include "Actor.h"
#include "Scene.h"

#include "Components/Component.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CTransform

CActorTransform::CActorTransform()
    : Matrix()
    , Translation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
{
    CalculateMatrix();
}

void CActorTransform::SetTranslation(float x, float y, float z)
{
    SetTranslation(FVector3(x, y, z));
}

void CActorTransform::SetTranslation(const FVector3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void CActorTransform::SetScale(float x, float y, float z)
{
    SetScale(FVector3(x, y, z));
}

void CActorTransform::SetScale(const FVector3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void CActorTransform::SetRotation(float x, float y, float z)
{
    SetRotation(FVector3(x, y, z));
}

void CActorTransform::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void CActorTransform::CalculateMatrix()
{
    FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale);
    FMatrix4 RotationMatrix    = FMatrix4::RotationRollPitchYaw(Rotation);
    FMatrix4 TranslationMatrix = FMatrix4::Translation(Translation);
    
    Matrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
    Matrix = Matrix.Transpose();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FActor

FActor::FActor(class FScene* InSceneOwner)
    : CCoreObject()
    , Name()
    , SceneOwner(InSceneOwner)
    , Transform()
    , Components()
    , bIsStartable(true)
    , bIsTickable(true)
{
    CORE_OBJECT_INIT();
}

FActor::~FActor()
{
    for (CComponent* CurrentComponent : Components)
    {
        SafeDelete(CurrentComponent);
    }

    Components.Clear();
}

void FActor::Start()
{
    for (CComponent* Component : Components)
    {
        if (Component->IsStartable())
        {
            Component->Start();
        }
    }
}

void FActor::Tick(FTimestamp DeltaTime)
{
    for (CComponent* Component : Components)
    {
        if (Component->IsTickable())
        {
            Component->Tick(DeltaTime);
        }
    }
}

void FActor::AddComponent(CComponent* InComponent)
{
    Check(InComponent != nullptr);
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

bool FActor::HasComponentOfClass(class CClassType* ComponentClass) const
{
    for (CComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return true;
        }
    }

    return false;
}

CComponent* FActor::GetComponentOfClass(class CClassType* ComponentClass) const
{
    for (CComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return Component;
        }
    }

    return nullptr;
}
