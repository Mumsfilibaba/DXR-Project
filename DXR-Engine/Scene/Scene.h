#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Rendering/MeshDrawCommand.h"

#include "Time/Timestamp.h"

#include "Core/Containers/Array.h"

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

    const TArray<Actor*>& GetActors() const { return Actors; }
    const TArray<Light*>& GetLights() const { return Lights; }

    const TArray<MeshDrawCommand>& GetMeshDrawCommands() const { return MeshDrawCommands; }
     
    Camera* GetCamera() const { return CurrentCamera; }

    static Scene* LoadFromFile(const std::string& Filepath);

private:
    void AddMeshComponent(class MeshComponent* Component);

    TArray<Actor*> Actors;
    TArray<Light*> Lights;
    TArray<MeshDrawCommand> MeshDrawCommands;

    Camera* CurrentCamera = nullptr;
};