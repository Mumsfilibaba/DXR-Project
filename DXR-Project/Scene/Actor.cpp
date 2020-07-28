#include "Actor.h"

/*
* Component Base-Class
*/

Component::Component(Actor* InOwningActor)
	: OwningActor(InOwningActor)
{
	VALIDATE(InOwningActor != nullptr);
}

Component::~Component()
{
}

/*
* Actor
*/

Actor::Actor()
	: Components()
	, Transform()
{
}

Actor::~Actor()
{
	for (Component* CurrentComponent : Components)
	{
		SAFEDELETE(CurrentComponent);
	}

	Components.clear();
}

void Actor::AddComponent(Component* InComponent)
{
	VALIDATE(InComponent != nullptr);
	Components.emplace_back(InComponent);
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
	, Position()
{
	SetPosition(0.0f, 0.0f, 0.0f);
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

void Transform::CalculateMatrix()
{
	XMVECTOR XmPosition = XMLoadFloat3(&Position);
	XMMATRIX XmMatrix	= XMMatrixTranslationFromVector(XmPosition);
	XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XmMatrix));
}
