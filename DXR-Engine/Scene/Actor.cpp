#include "Actor.h"
#include "Scene.h"

Component::Component(Actor* InOwningActor)
    : CoreObject()
    , OwningActor(InOwningActor)
{
    Assert(InOwningActor != nullptr);
    CORE_OBJECT_INIT();
}

Actor::Actor()
    : CoreObject()
    , Components()
    , Transform()
{
    CORE_OBJECT_INIT();
}

Actor::~Actor()
{
    for (Component* CurrentComponent : Components)
    {
        SafeDelete(CurrentComponent);
    }

    Components.Clear();
}

void Actor::AddComponent(Component* InComponent)
{
    Assert(InComponent != nullptr);
    Components.EmplaceBack(InComponent);

    if (Scene)
    {
        Scene->OnAddedComponent(InComponent);
    }
}

void Actor::SetName(const std::string& InName)
{
    Name = InName;
}

Transform::Transform()
    : Matrix()
    , Translation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
{
    CalculateMatrix();
}

void Transform::SetTranslation(float x, float y, float z)
{
    SetTranslation(XMFLOAT3(x, y, z));
}

void Transform::SetTranslation(const XMFLOAT3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void Transform::SetScale(float x, float y, float z)
{
    SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(const XMFLOAT3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void Transform::SetRotation(float x, float y, float z)
{
    SetRotation(XMFLOAT3(x, y, z));
}

void Transform::SetRotation(const XMFLOAT3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void Transform::CalculateMatrix()
{
    XMVECTOR XmTranslation = XMLoadFloat3(&Translation);
    XMVECTOR XmScale       = XMLoadFloat3(&Scale);
    // Convert into Roll, Pitch, Yaw
    XMVECTOR XmRotation = XMVectorSet(Rotation.z, Rotation.y, Rotation.x, 0.0f);
    
    XMMATRIX XmMatrix = XMMatrixMultiply(
            XMMatrixMultiply(XMMatrixScalingFromVector(XmScale), 
            XMMatrixRotationRollPitchYawFromVector(XmRotation)),
            XMMatrixTranslationFromVector(XmTranslation));
    XMStoreFloat3x4(&TinyMatrix, XmMatrix);
    XmMatrix = XMMatrixTranspose(XmMatrix);
    XMStoreFloat4x4(&Matrix, XmMatrix);

    XMMATRIX XmMatrixInv = XMMatrixInverse(nullptr, XmMatrix);
    XMStoreFloat4x4(&MatrixInv, XMMatrixTranspose(XmMatrixInv));
}
