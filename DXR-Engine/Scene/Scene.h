#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Rendering/MeshDrawCommand.h"

#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"

class Scene
{
public:
    Scene();
    ~Scene();

    void Tick( CTimestamp DeltaTime );

    void AddCamera( Camera* InCamera );
    void AddActor( CActor* InActor );
    void AddLight( Light* InLight );

    void OnAddedComponent( CComponent* NewComponent );

    template<typename TComponent>
    FORCEINLINE const TArray<TComponent> GetAllComponentsOfType() const
    {
        // TODO: Cache this result

        TArray<TComponent> Components;
        for ( CActor* Actor : Actors )
        {
            TComponent* Component = Actor->GetComponentOfType<TComponent>();
            if ( Component )
            {
                Components.Emplace( *Component );
            }
        }

        return Move( Components );
    }

    FORCEINLINE const TArray<CActor*>& GetActors() const
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

private:
    void AddMeshComponent( class MeshComponent* Component );

    TArray<CActor*> Actors;
    TArray<Light*> Lights;
    TArray<MeshDrawCommand> MeshDrawCommands;

    Camera* CurrentCamera = nullptr;
};