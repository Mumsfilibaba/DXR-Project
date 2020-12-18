#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Rendering/MeshDrawCommand.h"

#include "Time/Timestamp.h"

/*
* Scene
*/

class Scene
{
public:
	Scene();
	~Scene();

	void Tick(Timestamp DeltaTime);

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

	template<typename TComponent>
	FORCEINLINE const TArray<TComponent> GetAllComponentsOfType() const
	{
		// TODO: Cache this result

		TArray<TComponent> Components;
		for (Actor* Actor : Actors)
		{
			TComponent* Component = Actor->GetComponentOfType<TComponent>();
			if (Component)
			{
				Components.EmplaceBack(*Component);
			}
		}

		return Move(Components);
	}

	FORCEINLINE const TArray<MeshDrawCommand>& GetMeshDrawCommands() const
	{
		return MeshDrawCommands;
	}
	 
	FORCEINLINE Camera* GetCamera() const
	{
		return CurrentCamera;
	}

	static Scene* LoadFromFile(const std::string& Filepath);
	
	static void SetCurrentScene(Scene* InCurrentScene);
	static Scene* GetCurrentScene();

private:
	void AddMeshComponent(class MeshComponent* Component);

	TArray<Actor*> Actors;
	TArray<Light*> Lights;
	TArray<MeshDrawCommand> MeshDrawCommands;

	Camera* CurrentCamera = nullptr;

	static Scene* CurrentScene;
};