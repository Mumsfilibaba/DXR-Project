#pragma once
#include "Actor.h"

#include "Rendering/Camera.h"

class Scene
{
public:
	Scene();
	~Scene();

	void AddCamera(Camera* InCamera);
	void AddActor(Actor* InActor);

	FORCEINLINE const std::vector<Actor*> GetActors() const
	{
		return Actors;
	}

	FORCEINLINE Camera* GetCamera() const
	{
		return CurrentCamera;
	}

	static Scene* LoadFromFile(const std::string& Filepath, class D3D12Device* Device);

private:
	Camera* CurrentCamera = nullptr;
	std::vector<Actor*> Actors;
};