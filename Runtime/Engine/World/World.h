#pragma once
#include "Camera.h"
#include "Actors/PlayerController.h"
#include "Lights/Light.h"
#include "Reflections/LightProbe.h"
#include "Core/Time/Timespan.h"
#include "Core/Containers/Array.h"
#include "RendererCore/Interfaces/IScene.h"

// TODO: Remove
#include "Engine/World/Components/ProxySceneComponent.h"

class FSceneComponent;

class ENGINE_API FWorld
{
public:

    /**
     * @brief - Default constructor
     */
    FWorld();

    /**
     * @brief - Destructor
     */
    ~FWorld();

    /**
     * @brief  - Create a new actor and add it to the world 
     * @return - Returns the newly created actor
     */
    FActor* CreateActor();

    /**
     * @brief - Start game 
     */
    void Start();

     /**
      * @brief           - Ticks all actors in the world, should be called once per frame
      * @param DeltaTime - The time between this and the last tick
      */
    void Tick(FTimespan DeltaTime);

    /**
     * @brief          - Adds a camera into the world 
     * @param InCamera - Camera to add to the world
     */
    void AddCamera(FCamera* InCamera);

    /**
     * @brief         - Adds an actor into the world 
     * @param InActor - Actor to add to the world
     */
    void AddActor(FActor* InActor);

    /**
     * @brief                    - Adds a player-controller into the world
     * @param InPlayerController - PlayerController to add to the world
     */
    void AddPlayerController(FPlayerController* InPlayerController);

    /**
     * @brief         - Adds an light into the world
     * @param InLight - Light to add to the world
     */
    void AddLight(FLight* InLight);

    /**
     * @brief              - Adds a light-probe into the world
     * @param InLightProbe - LightProbe to add to the world
     */
    void AddLightProbe(FLightProbe* InLightProbe);

    /**
     * @brief                   - Function called when adding a new RendererComponent
     * @param RendererComponent - New RendererComponent just added to the world
     */
    void AddRendererComponent(FSceneComponent* RendererComponent);

    /**
      * @brief         - Sets the scene representation in the renderer
      * @param InScene - Interface to the renderer scene representation
      */
    void SetSceneInterface(IScene* InScene);

    /**
     * @return - Returns the Renderer representation of this scene 
     */
    IScene* GetSceneInterface() const
    {
        return Scene;
    }

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
     * @return - Returns a reference to an array of all actors in the world
     */
    const TArray<FActor*>& GetActors() const
    {
        return Actors;
    }

    /**
     * @return - Returns a reference to an array of all actors in the world
     */
    const TArray<FPlayerController*>& GetPlayerControllers() const
    {
        return PlayerControllers;
    }

    /**
     * @return - Returns a pointer to the first PlayerController in the world
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
     * @return - Returns a reference to an array of all lights in the world
     */
    const TArray<FLight*>& GetLights() const
    {
        return Lights;
    }

    /**
     * @return - Returns a reference to an array of all light-probes in the world
     */
    const TArray<FLightProbe*>& GetLightProbes() const
    {
        return LightProbes;
    }

    /**
     * @return - Returns a pointer to the camera of the world
     */
    FCamera* GetCamera() const
    {
        return CurrentCamera;
    }

private:
    FCamera* CurrentCamera;
    IScene*  Scene;

    // These actors are the "permanent" storage of actors
    TArray<FActor*>            Actors;
    TArray<FPlayerController*> PlayerControllers;
    TArray<FLight*>            Lights;
    TArray<FLightProbe*>       LightProbes;
};
