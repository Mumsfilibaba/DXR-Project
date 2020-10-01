#pragma once
#include "Scene/Actor.h"

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

	TSharedPtr<class Material>	Material;
	TSharedPtr<class Mesh>		Mesh;
};