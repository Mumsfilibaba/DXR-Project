#pragma once
#include <vector>

#include <DirectXMath.h>

/*
* Component Base-Class
*/

class Component
{
public:
	Component();
	~Component();
};

/*
* Actor
*/

class Actor
{
public:
	Actor();
	~Actor();

	void AddComponent(Component* InComponent);

private:
	std::vector<Component*> Components;
};