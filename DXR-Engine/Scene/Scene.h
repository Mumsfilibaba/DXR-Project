#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Rendering/MeshDrawCommand.h"

#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"

class CORE_API CScene
{
public:
    CScene();
    ~CScene();

    /* Create a new actor and add it to the scene */
    class CActor* MakeActor();

    /* Start game */
    void Start();

    /* Ticks all actors in the scene, should be called once per frame */
    void Tick( CTimestamp DeltaTime );

    /* Adds a camera into the scene */
    void AddCamera( CCamera* InCamera );

    /* Adds an actor into the scene */
    void AddActor( CActor* InActor );

    /* Adds a new light into the scene*/
    void AddLight( CLight* InLight );

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

    FORCEINLINE const TArray<CLight*>& GetLights() const
    {
        return Lights;
    }

    FORCEINLINE const TArray<SMeshDrawCommand>& GetMeshDrawCommands() const
    {
        return MeshDrawCommands;
    }

    FORCEINLINE CCamera* GetCamera() const
    {
        return CurrentCamera;
    }

private:
    void AddMeshComponent( class CMeshComponent* Component );

    TArray<CActor*> Actors;
    TArray<CLight*> Lights;
    TArray<SMeshDrawCommand> MeshDrawCommands;

    CCamera* CurrentCamera = nullptr;
};