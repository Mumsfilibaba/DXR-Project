#pragma once
#include "Actor.h"
#include "Camera.h"

#include "Lights/Light.h"

#include "Renderer/MeshDrawCommand.h"

#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Scene

class ENGINE_API CScene
{
public:

    /**
     * @brief: Default constructor
     */
    CScene();

    /**
     * @brief: Destructor
     */
    ~CScene();

    /**
     * @brief: Create a new actor and add it to the scene 
     * 
     * @return: Returns the newly created actor
     */
    class CActor* MakeActor();

    /**
     * @brief: Start game 
     */
    void Start();

    /* Ticks all actors in the scene, should be called once per frame */
    void Tick(CTimestamp DeltaTime);

    /**
     * @brief: Adds a camera into the scene 
     * 
     * @param InCamera: Camera to add to the scene
     */
    void AddCamera(CCamera* InCamera);

    /**
     * @brief: Adds an actor into the scene 
     * 
     * @param InActor: Actor to add to the scene
     */
    void AddActor(CActor* InActor);

    /**
     * @brief: Adds an light into the scene
     *
     * @param InLight: Light to add to the scene
     */
    void AddLight(CLight* InLight);

    /**
     * @brief: Function called when adding a component
     * 
     * @param NewComponent: New component just added to the scene
     */
    void OnAddedComponent(CComponent* NewComponent);

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
        for (CActor* Actor : Actors)
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
    FORCEINLINE const TArray<CActor*>& GetActors() const
    {
        return Actors;
    }

    /**
     * @brief: Retrieve all lights of the scene
     *
     * @return: Returns a reference to an array of all lights in the scene
     */
    FORCEINLINE const TArray<CLight*>& GetLights() const
    {
        return Lights;
    }

    /**
     * @brief: Retrieve all MeshDrawCommands of the scene
     *
     * @return: Returns a reference to an array of all MeshDrawCommands in the scene
     */
    FORCEINLINE const TArray<SMeshDrawCommand>& GetMeshDrawCommands() const
    {
        return MeshDrawCommands;
    }

    /**
     * @brief: Retrieve the camera of the scene
     * 
     * @return: Returns a pointer to the camera of the scene
     */
    FORCEINLINE CCamera* GetCamera() const
    {
        return CurrentCamera;
    }

private:
    void AddMeshComponent(class CMeshComponent* Component);

    TArray<CActor*> Actors;
    TArray<CLight*> Lights;
    TArray<SMeshDrawCommand> MeshDrawCommands;

    CCamera* CurrentCamera = nullptr;
};