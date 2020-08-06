#pragma once
#include "Actor.h"

/*
* MeshComponent
*/

class MeshComponent : public Component
{
	CORE_OBJECT(MeshComponent, Component);

public:
	MeshComponent(Actor* InOwningActor)
		: Component(InOwningActor)
		, Material(nullptr)
		, Mesh(nullptr)
	{
		CORE_OBJECT_INIT();
	}

	~MeshComponent() = default;

public:
	std::shared_ptr<class Material> Material;
	std::shared_ptr<class Mesh> Mesh;
};