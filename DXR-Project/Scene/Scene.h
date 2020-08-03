#pragma once
#include "Actor.h"
#include "Camera.h"
#include "Light.h"
#include "MeshDrawCommand.h"

class Scene
{
public:
	Scene();
	~Scene();

	void AddCamera(Camera* InCamera);
	void AddActor(Actor* InActor);
	void AddLight(Light* InLight);

	void OnAddedComponent(Component* NewComponent);

	FORCEINLINE const std::vector<Actor*>& GetActors() const
	{
		return Actors;
	}

	FORCEINLINE const std::vector<MeshDrawCommand>& GetMeshDrawCommands() const
	{
		return MeshDrawCommands;
	}
	 
	FORCEINLINE Camera* GetCamera() const
	{
		return CurrentCamera;
	}

	static Scene* LoadFromFile(const std::string& Filepath, class D3D12Device* Device);

private:
	void AddMeshComponent(class MeshComponent* Component);

	std::vector<Actor*> Actors;
	std::vector<Light*> Light;
	std::vector<MeshDrawCommand> MeshDrawCommands;

	Camera* CurrentCamera = nullptr;
};