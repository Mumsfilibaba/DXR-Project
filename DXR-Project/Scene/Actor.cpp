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
	, Translation(0.0f, 0.0f, 0.0f)
	, Scale(1.0f, 1.0f, 1.0f)
{
	CalculateMatrix();
}

void Transform::SetTranslation(Float x, Float y, Float z)
{
	XMVECTOR XmPosition = XMVectorSet(x, y, z, 0.0f);
	XMStoreFloat3(&Translation, XmPosition);

	CalculateMatrix();
}

void Transform::SetTranslation(const XMFLOAT3& InPosition)
{
	Translation = InPosition;
	CalculateMatrix();
}

void Transform::SetScale(Float x, Float y, Float z)
{
	XMVECTOR XmScale = XMVectorSet(x, y, z, 0.0f);
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
	XMVECTOR XmTranslation	= XMLoadFloat3(&Translation);
	XMVECTOR XmScale		= XMLoadFloat3(&Scale);
	
	XMMATRIX XmMatrix = XMMatrixMultiply(XMMatrixScalingFromVector(XmScale), XMMatrixTranslationFromVector(XmTranslation));
	XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XmMatrix));
}
