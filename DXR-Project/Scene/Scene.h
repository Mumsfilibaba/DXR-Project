#pragma once
#include "Actor.h"

class Scene
{
public:
	Scene();
	~Scene();

	void AddActor(Actor* InActor);

private:
	std::vector<Actor*> Actors;
};