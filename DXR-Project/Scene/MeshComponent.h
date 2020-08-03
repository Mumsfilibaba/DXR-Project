#pragma once
#include "Actor.h"

/*
* MeshComponent
*/

class MeshComponent : public Component
{
public:
	MeshComponent(Actor* InOwningActor)
		: Component(InOwningActor)
		, Material(nullptr)
		, Mesh(nullptr)
	{
	}

	~MeshComponent() = default;

public:
	std::shared_ptr<class Material> Material;
	std::shared_ptr<class Mesh> Mesh;
};