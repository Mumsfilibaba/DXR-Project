#pragma once
#include "Actor.h"

class Scene
{
public:
	Scene();
	~Scene();

	void AddActor(Actor* InActor);

	FORCEINLINE const std::vector<Actor*> GetActors() const
	{
		return Actors;
	}

private:
	std::vector<Actor*> Actors;
};