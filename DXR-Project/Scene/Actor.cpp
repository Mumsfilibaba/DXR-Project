#include "Actor.h"
#include "Scene.h"

/*
* Component Base-Class
*/

Component::Component(Actor* InOwningActor)
	: CoreObject()
	, OwningActor(InOwningActor)
{
	VALIDATE(InOwningActor != nullptr);

	CORE_OBJECT_INIT();
}

Component::~Component()
{
}

/*
* Actor
*/

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
		SAFEDELETE(CurrentComponent);
	}

	Components.Clear();
}

void Actor::OnAddedToScene(Scene* InScene)
{
	CurrentScene = InScene;
}

void Actor::AddComponent(Component* InComponent)
{
	VALIDATE(InComponent != nullptr);
	Components.EmplaceBack(InComponent);

	if (CurrentScene)
	{
		CurrentScene->OnAddedComponent(InComponent);
	}
}

void Actor::SetDebugName(const std::string& InDebugName)
{
	DebugName = InDebugName;
}

/*
* Transform
*/

Transform::Transform()
	: Matrix()
	, Position(0.0f, 0.0f, 0.0f)
	, Scale(1.0f, 1.0f, 1.0f)
{
	CalculateMatrix();
}

void Transform::SetPosition(float X, float Y, float Z)
{
	XMVECTOR XmPosition = XMVectorSet(X, Y, Z, 0.0f);
	XMStoreFloat3(&Position, XmPosition);

	CalculateMatrix();
}

void Transform::SetPosition(const XMFLOAT3& InPosition)
{
	Position = InPosition;
	CalculateMatrix();
}

void Transform::SetScale(float X, float Y, float Z)
{
	XMVECTOR XmScale = XMVectorSet(X, Y, Z, 0.0f);
	XMStoreFloat3(&Scale, XmScale);

	CalculateMatrix();
}

void Transform::SetScale(const XMFLOAT3& InScale)
{
	Scale = InScale;
	CalculateMatrix();
}

void Transform::CalculateMatrix()
{
	XMVECTOR XmPosition = XMLoadFloat3(&Position);
	XMVECTOR XmScale	= XMLoadFloat3(&Scale);
	
	XMMATRIX XmMatrix = XMMatrixMultiply(XMMatrixScalingFromVector(XmScale), XMMatrixTranslationFromVector(XmPosition));
	XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XmMatrix));
}
