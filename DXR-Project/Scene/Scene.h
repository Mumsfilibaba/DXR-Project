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

	static Scene* LoadFromFile(const std::string& Filepath, class D3D12Device* Device);

private:
	std::vector<Actor*> Actors;
};