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

	FORCEINLINE const TArray<Actor*>& GetActors() const
	{
		return Actors;
	}

	FORCEINLINE const TArray<Light*>& GetLights() const
	{
		return Lights;
	}

	FORCEINLINE const TArray<MeshDrawCommand>& GetMeshDrawCommands() const
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

	TArray<Actor*> Actors;
	TArray<Light*> Lights;
	TArray<MeshDrawCommand> MeshDrawCommands;

	Camera* CurrentCamera = nullptr;
};