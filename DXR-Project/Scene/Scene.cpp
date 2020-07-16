#include "Scene.h"

Scene::Scene()
	: Actors()
{
}

Scene::~Scene()
{
	for (Actor* CurrentActor : Actors)
	{
		SAFEDELETE(CurrentActor);
	}

	Actors.clear();
}

void Scene::AddActor(Actor* InActor)
{
	VALIDATE(InActor != nullptr);
	Actors.emplace_back(InActor);
}
