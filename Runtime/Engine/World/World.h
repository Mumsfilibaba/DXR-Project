#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/Array.h"
#include "Core/Delegates/Event.h"
#include "Engine/World/Actors/PlayerController.h"
#include "RendererCore/Interfaces/IScene.h"

class FCameraComponent;
class FSceneComponent;

class ENGINE_API FWorld
{
public:
    DECLARE_EVENT(FOnActorRemovedEvent, FWorld, FActor*);


    /**
     * @brief Default constructor
     */
    FWorld();

    /**
     * @brief Destructor
     */
    ~FWorld();

    /**
     * @brief Create a new actor and add it to the world 
     * 
     * @return Returns the newly created actor
     */
    FActor* CreateActor();

    /**
     * @brief Creates, initializes, and registers an actor owned by this world
     *
     * @tparam ActorType Actor class to create
     * @tparam ArgumentTypes Types forwarded to the actor-specific Initialize overload
     * @param Arguments Arguments forwarded to Initialize before world registration
     * @return Returns the world-owned actor, or nullptr if creation failed
     */
    template<typename ActorType, typename... ArgumentTypes>
    ActorType* SpawnActor(ArgumentTypes&&... Arguments)
    {
        ActorType* NewActor = NewObject<ActorType>();
        if (NewActor)
        {
            if constexpr (sizeof...(ArgumentTypes) == 0)
            {
                static_cast<FActor*>(NewActor)->Initialize();
            }
            else
            {
                NewActor->Initialize(Forward<ArgumentTypes>(Arguments)...);
            }

            AddActor(NewActor);
        }

        return NewActor;
    }

    /**
     * @brief Start game 
     */
    void Start();

     /**
      * @brief Ticks all actors in the world, should be called once per frame
      * 
      * @param DeltaTime The time between this and the last tick in seconds
      */
    void Tick(float DeltaTime);

    /**
     * @brief Adds an actor into the world 
     * 
     * @param InActor Actor to add to the world
     */
    void AddActor(FActor* InActor);

    /**
     * @brief Removes and deletes an actor owned by the world
     *
     * @param InActor Actor to remove
     */
    void RemoveActor(FActor* InActor);

    /**
     * @brief Sets the camera component used for rendering the world
     *
     * @param InCamera Camera component to make active, or nullptr to clear
     */
    void SetActiveCamera(FCameraComponent* InCamera);

    /**
     * @brief Adds a player-controller into the world
     * 
     * @param InPlayerController PlayerController to add to the world
     */
    void AddPlayerController(FPlayerController* InPlayerController);

    /**
     * @brief Function called when adding a new SceneComponent
     * 
     * @param SceneComponent New SceneComponent just added to the world
     */
    void AddSceneComponent(FSceneComponent* SceneComponent);

    /**
     * @brief Function called when removing a SceneComponent
     * 
     * @param SceneComponent SceneComponent being removed from the world
     */
    void RemoveSceneComponent(FSceneComponent* SceneComponent);

    /**
      * @brief Sets the scene representation in the renderer
      * 
      * @param InScene Interface to the renderer scene representation
      */
    void SetSceneInterface(IScene* InScene);

    /**
     * @brief Clears the renderer scene interface after the renderer scene is destroyed
     */
    void ClearSceneInterface();

    /**
     * @return Returns the event broadcast immediately before an actor is deleted
     */
    FOnActorRemovedEvent& GetOnActorRemovedEvent()
    {
        return OnActorRemovedEvent;
    }

    /**
     * @return Returns the Renderer representation of this scene 
     */
    IScene* GetSceneInterface() const
    {
        return Scene;
    }

    /**
     * @return Returns a reference to an array of all actors in the world
     */
    const TArray<FActor*>& GetActors() const
    {
        return Actors;
    }

    /**
     * @return Returns a reference to an array of all actors in the world
     */
    const TArray<FPlayerController*>& GetPlayerControllers() const
    {
        return PlayerControllers;
    }

    /**
     * @return Returns a pointer to the first PlayerController in the world
     */
    FPlayerController* GetFirstPlayerController() const
    {
        if (!PlayerControllers.IsEmpty())
        {
            return PlayerControllers.First();
        }

        return nullptr;
    }

    /**
     * @return Returns the active camera component
     */
    FCameraComponent* GetActiveCamera() const
    {
        return ActiveCamera;
    }

private:
    IScene*                    Scene;
    FCameraComponent*          ActiveCamera;
    TArray<FActor*>            Actors;
    TArray<FPlayerController*> PlayerControllers;
    FOnActorRemovedEvent       OnActorRemovedEvent;
};
