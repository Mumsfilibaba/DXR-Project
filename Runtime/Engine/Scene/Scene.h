#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Renderer/MeshDrawCommand.h"

#include "Core/Time/Timespan.h"
#include "Core/Containers/Array.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FScene

class ENGINE_API FScene
{
public:

    /**
     * @brief: Default constructor
     */
    FScene();

    /**
     * @brief: Destructor
     */
    ~FScene();

    /**
     * @brief: Create a new actor and add it to the scene 
     * 
     * @return: Returns the newly created actor
     */
    class FActor* MakeActor();

    /**
     * @brief: Start game 
     */
    void Start();

     /** @brief: Ticks all actors in the scene, should be called once per frame */
    void Tick(FTimespan DeltaTime);

    /**
     * @brief: Adds a camera into the scene 
     * 
     * @param InCamera: Camera to add to the scene
     */
    void AddCamera(FCamera* InCamera);

    /**
     * @brief: Adds an actor into the scene 
     * 
     * @param InActor: Actor to add to the scene
     */
    void AddActor(FActor* InActor);

    /**
     * @brief: Adds an light into the scene
     *
     * @param InLight: Light to add to the scene
     */
    void AddLight(FLight* InLight);

    /**
     * @brief: Function called when adding a component
     * 
     * @param NewComponent: New component just added to the scene
     */
    void OnAddedComponent(FComponent* NewComponent);

    /**
     * @brief: Retrieve all components of a certain type
     * 
     * @return: Returns an array of all components of the specified type
     */
    template<typename ComponentType>
    FORCEINLINE TArray<ComponentType> GetAllComponentsOfType() const
    {
        // TODO: Cache this result

        TArray<ComponentType> Components;
        for (FActor* Actor : Actors)
        {
            ComponentType* Component = Actor->GetComponentOfType<ComponentType>();
            if (Component)
            {
                Components.Emplace(*Component);
            }
        }

        return Move(Components);
    }

    /**
     * @brief: Retrieve all actors of the scene
     * 
     * @return: Returns a reference to an array of all actors in the scene
     */
    FORCEINLINE const TArray<FActor*>& GetActors() const
    {
        return Actors;
    }

    /**
     * @brief: Retrieve all lights of the scene
     *
     * @return: Returns a reference to an array of all lights in the scene
     */
    FORCEINLINE const TArray<FLight*>& GetLights() const
    {
        return Lights;
    }

    /**
     * @brief: Retrieve all MeshDrawCommands of the scene
     *
     * @return: Returns a reference to an array of all MeshDrawCommands in the scene
     */
    FORCEINLINE const TArray<FMeshDrawCommand>& GetMeshDrawCommands() const
    {
        return MeshDrawCommands;
    }

    /**
     * @brief: Retrieve the camera of the scene
     * 
     * @return: Returns a pointer to the camera of the scene
     */
    FORCEINLINE FCamera* GetCamera() const
    {
        return CurrentCamera;
    }

private:
    void AddMeshComponent(class FMeshComponent* Component);

    TArray<FActor*> Actors;
    TArray<FLight*> Lights;
    TArray<FMeshDrawCommand> MeshDrawCommands;

    FCamera* CurrentCamera = nullptr;
};