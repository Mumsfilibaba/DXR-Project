#pragma once
#include <vector>

#include <DirectXMath.h>

/*
* Component Base-Class
*/

class Actor;

class Component
{
public:
	Component(Actor* InOwningActor);
	virtual ~Component();

public:
	Actor* OwningActor = nullptr;
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

	FORCEINLINE void SetTransform(const XMFLOAT4X4& InTransform)
	{
		Transform = InTransform;
	}

	FORCEINLINE const XMFLOAT4X4& GetTransform() const
	{
		return Transform;
	}

	FORCEINLINE Component* GetComponent() const
	{
		return Components.front();
	}

public:
	XMFLOAT4X4 Transform;

private:
	std::vector<Component*> Components;
};