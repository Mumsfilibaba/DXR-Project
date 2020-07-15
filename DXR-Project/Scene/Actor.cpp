#include "Actor.h"

/*
* Component Base-Class
*/

Component::Component()
{
}

Component::~Component()
{
}

/*
* Actor
*/

Actor::Actor()
	: Components()
{
}

Actor::~Actor()
{
}

void Actor::AddComponent(Component* InComponent)
{
	Components.emplace_back(InComponent);
}
