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
    SetTranslation(CVector3(x, y, z));
}

void CActorTransform::SetTranslation(const CVector3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void CActorTransform::SetScale(float x, float y, float z)
{
    SetScale(CVector3(x, y, z));
}

void CActorTransform::SetScale(const CVector3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void CActorTransform::SetRotation(float x, float y, float z)
{
    SetRotation(CVector3(x, y, z));
}

void CActorTransform::SetRotation(const CVector3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void CActorTransform::CalculateMatrix()
{
    CMatrix4 ScaleMatrix       = CMatrix4::Scale(Scale);
    CMatrix4 RotationMatrix    = CMatrix4::RotationRollPitchYaw(Rotation);
    CMatrix4 TranslationMatrix = CMatrix4::Translation(Translation);
    
    Matrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
    Matrix = Matrix.Transpose();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CActor

CActor::CActor(class CScene* InSceneOwner)
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

CActor::~CActor()
{
    for (CComponent* CurrentComponent : Components)
    {
        SafeDelete(CurrentComponent);
    }

    Components.Clear();
}

void CActor::Start()
{
    for (CComponent* Component : Components)
    {
        if (Component->IsStartable())
        {
            Component->Start();
        }
    }
}

void CActor::Tick(CTimestamp DeltaTime)
{
    for (CComponent* Component : Components)
    {
        if (Component->IsTickable())
        {
            Component->Tick(DeltaTime);
        }
    }
}

void CActor::AddComponent(CComponent* InComponent)
{
    Check(InComponent != nullptr);
    Components.Emplace(InComponent);

    if (SceneOwner)
    {
        SceneOwner->OnAddedComponent(InComponent);
    }
}

void CActor::SetName(const String& InName)
{
    Name = InName;
}

bool CActor::HasComponentOfClass(class CClassType* ComponentClass) const
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

CComponent* CActor::GetComponentOfClass(class CClassType* ComponentClass) const
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
