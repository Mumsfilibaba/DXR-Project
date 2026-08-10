#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Event.h"
#include "Engine/World/Actors/PlayerController.h"
#include "RendererCore/Interfaces/IScene.h"

class FActorFilter;
class FCameraComponent;
class FSceneComponent;

enum class EWorldRunState : uint8
{
    /** The world is being authored, game code is held back */
    Editing,

    /** The world is running, every tickable actor advances */
    Playing,

    /** The world is running but frozen, game code keeps its state without advancing */
    Paused,
};

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
     * @brief Start the game, calling Start on every startable actor and binding the player-controllers
     */
    void BeginPlay();

    /**
     * @brief End the game, calling EndPlay on every actor and dropping the input bindings made by BeginPlay
     */
    void EndPlay();

    /**
     * @brief Freeze or resume a running world, actors stop ticking without losing the state they built up
     *
     * @param bPaused True to freeze the world, false to resume it
     */
    void SetPaused(bool bPaused);

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
     * @brief Create a filter that actors can be placed in. Filters are presented in the scene-hierarchy in the 
     * order they were created. Names only have to be unique among the filters sharing a parent. Filters exist 
     * purely to organise the editor's scene-hierarchy. Outside an editor build no filter is created and this 
     * returns nullptr, which every other function here accepts, so scene-setup code needs no branching.
     *
     * @param InName Name of the filter, an existing filter is returned if the name is already taken in the parent
     * @param InParent Filter to nest the new filter inside, or nullptr to place it at the root of the hierarchy
     * @return Returns the filter, or nullptr outside an editor build
     */
    FActorFilter* CreateActorFilter(const String& InName, FActorFilter* InParent = nullptr);

    /**
     * @brief Look for a filter by name among the filters nested directly inside a parent
     *
     * @param InName Name of the filter to look for
     * @param InParent Filter to search inside, or nullptr to search the root of the hierarchy
     * @return Returns the filter with the given name, or nullptr if no such filter exists
     */
    FActorFilter* FindActorFilter(const String& InName, FActorFilter* InParent = nullptr) const;

    /**
     * @brief Move a filter, and everything nested inside it, into another filter. A filter cannot 
     * be moved into itself or into one of its own descendants, such a move is ignored.
     *
     * @param InFilter Filter to move
     * @param InParent Filter to nest it inside, or nullptr to move it to the root of the hierarchy
     */
    void SetActorFilterParent(FActorFilter* InFilter, FActorFilter* InParent);

    /**
     * @brief Destroy a filter, every actor and filter inside it moves up into the destroyed filter's parent
     *
     * @param InFilter Filter to destroy
     */
    void DestroyActorFilter(FActorFilter* InFilter);

    /**
     * @brief Set the filter that actors added from now on are placed in Only affects actors added
     * after this call, SetFilter remains the way to place a single actor.
     *
     * @param InFilter Filter to place new actors in, or nullptr to leave them at the root
     */
    void SetCurrentFilter(FActorFilter* InFilter);

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
     * @return Returns every filter in the world, in creation order, and an empty view outside an editor build
     */
    TArrayView<FActorFilter* const> GetActorFilters() const
    {
    #if EDITOR_BUILD
        return ActorFilters;
    #else
        return { };
    #endif
    }

    /**
     * @return Returns the filter new actors are currently placed in, or nullptr
     */
    FActorFilter* GetCurrentFilter() const
    {
    #if EDITOR_BUILD
        return CurrentFilter;
    #else
        return nullptr;
    #endif
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

    /**
     * @return Returns the current run-state of the world
     */
    EWorldRunState GetRunState() const
    {
        return RunState;
    }

    /**
     * @return Returns true while the world is running and advancing
     */
    bool IsPlaying() const
    {
        return RunState == EWorldRunState::Playing;
    }

    /**
     * @return Returns true while the world is being authored rather than run
     */
    bool IsEditing() const
    {
        return RunState == EWorldRunState::Editing;
    }

    /**
     * @return Returns true while the world is running but frozen
     */
    bool IsPaused() const
    {
        return RunState == EWorldRunState::Paused;
    }

private:
    IScene*                    Scene;
    FCameraComponent*          ActiveCamera;
    EWorldRunState             RunState;
    TArray<FActor*>            Actors;
#if EDITOR_BUILD
    TArray<FActorFilter*>      ActorFilters;
    FActorFilter*              CurrentFilter;
#endif
    TArray<FPlayerController*> PlayerControllers;
    FOnActorRemovedEvent       OnActorRemovedEvent;
};
