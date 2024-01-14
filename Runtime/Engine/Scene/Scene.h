#pragma once
#include "Camera.h"
#include "Actors/PlayerController.h"
#include "Lights/Light.h"
#include "Reflections/LightProbe.h"
#include "Core/Time/Timespan.h"
#include "Core/Containers/Array.h"
#include "Renderer/MeshDrawCommand.h"

class FRendererScene;

class ENGINE_API FScene
{
public:

    /**
     * @brief - Default constructor
     */
    FScene();

    /**
     * @brief - Destructor
     */
    ~FScene();

    /**
     * @brief  - Create a new actor and add it to the scene 
     * @return - Returns the newly created actor
     */
    FActor* CreateActor();

    /**
     * @brief - Start game 
     */
    void Start();

     /**
      * @brief          - Ticks all actors in the scene, should be called once per frame
      * @param DeltaTime - The time between this and the last tick
      */
    void Tick(FTimespan DeltaTime);

    /**
     * @brief          - Adds a camera into the scene 
     * @param InCamera - Camera to add to the scene
     */
    void AddCamera(FCamera* InCamera);

    /**
     * @brief         - Adds an actor into the scene 
     * @param InActor - Actor to add to the scene
     */
    void AddActor(FActor* InActor);

    /**
     * @brief         - Adds an light into the scene
     * @param InLight - Light to add to the scene
     */
    void AddLight(FLight* InLight);

    /**
     * @brief              - Adds a light-probe into the scene
     * @param InLightProbe - LightProbe to add to the scene
     */
    void AddLightProbe(FLightProbe* InLightProbe);

    /**
     * @brief              - Function called when adding a component
     * @param NewComponent - New component just added to the scene
     */
    void OnAddedComponent(FComponent* NewComponent);

    void SyncRendering();

    /**
     * @brief  - Retrieve all components of a certain type
     * @return - Returns an array of all components of the specified type
     */
    template<typename ComponentType>
    TArray<ComponentType> GetAllComponentsOfType() const
    {
        // TODO: Cache this result

        TArray<ComponentType> Components;
        for (FActor* Actor : Actors)
        {
            if (ComponentType* Component = Actor->GetComponentOfType<ComponentType>())
            {
                Components.Emplace(*Component);
            }
        }

        return ::Move(Components);
    }

    /**
     * @return - Returns a pointer to the Renderers View of the scene
     */
    FRendererScene* GetRendererScene()
    {
        return RendererScene;
    }

    /**
     * @return - Returns a pointer to the Renderers View of the scene
     */
    const FRendererScene* GetRendererScene() const
    {
        return RendererScene;
    }

    /**
     * @return - Returns a reference to an array of all actors in the scene
     */
    const TArray<FActor*>& GetActors() const
    {
        return Actors;
    }

    /**
     * @return - Returns a reference to an array of all actors in the scene
     */
    const TArray<FPlayerController*>& GetPlayerControllers() const
    {
        return PlayerControllers;
    }

    /**
     * @return - Returns a pointer to the first PlayerController in the scene
     */
    FPlayerController* GetFirstPlayerController() const
    {
        if (!PlayerControllers.IsEmpty())
        {
            return PlayerControllers.FirstElement();
        }

        return nullptr;
    }

    /**
     * @return - Returns a reference to an array of all lights in the scene
     */
    const TArray<FLight*>& GetLights() const
    {
        return Lights;
    }

    /**
     * @return - Returns a reference to an array of all light-probes in the scene
     */
    const TArray<FLightProbe*>& GetLightProbes() const
    {
        return LightProbes;
    }

    /**
     * @return - Returns a pointer to the camera of the scene
     */
    FCamera* GetCamera() const
    {
        return CurrentCamera;
    }

    /**
     * @return - Returns a reference to an array of all MeshDrawCommands in the scene
     */
    FORCEINLINE const TArray<FMeshDrawCommand>& GetMeshDrawCommands() const
    {
        return MeshDrawCommands;
    }

private:
    void AddMeshComponent(class FMeshComponent* Component);

    // These actors were added this frame is waiting to be syncronized with the renderer
    TArray<FActor*> NewActors;

    // These actors are the "permanent" storage of actors
    TArray<FActor*> Actors;

    TArray<FPlayerController*> PlayerControllers;
    TArray<FLight*>            Lights;
    TArray<FLightProbe*>       LightProbes;
    TArray<FMeshDrawCommand>   MeshDrawCommands;

    FCamera*        CurrentCamera = nullptr;
    FRendererScene* RendererScene = nullptr;
};


struct FScenePrimitive
{
    FVector3 Translation;
    FVector3 Scale;
    FVector3 Rotation;
};

class ENGINE_API FRendererScene
{
public:
    FRendererScene(FScene* InScene);
    ~FRendererScene();

    void AddPrimitive(FScenePrimitive* InPrimitive);

private:
    FScene*                  Scene;
    TArray<FScenePrimitive*> ScenePrimitives;
};