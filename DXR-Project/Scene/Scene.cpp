#include "Scene.h"

Scene::Scene()
	: Actors()
{
}

Scene::~Scene()
{
}

void Scene::AddActor(Actor* InActor)
{
	Actors.emplace_back(InActor);
}
