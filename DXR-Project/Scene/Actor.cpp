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
	XMMATRIX XmMatrix = XMMatrixIdentity();
	XMStoreFloat4x4(&Transform, XmMatrix);
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
