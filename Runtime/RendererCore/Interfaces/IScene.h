#pragma once
#include "Core/CoreTypes.h"

class FActor;
class FSceneComponent;

struct IScene
{
    virtual ~IScene() = default;

    /** @brief Update the scene for the current frame. */
    virtual void Tick() = 0;

    /**
     * @brief Registers a scene component as a renderer source on the main thread.
     * @param InComponent Scene component to register.
     */
    virtual void AddSceneComponent(FSceneComponent* InComponent) = 0;

    /**
     * @brief Unregisters a scene component as a renderer source on the main thread.
     * @param InComponent Scene component to unregister.
     */
    virtual void RemoveSceneComponent(FSceneComponent* InComponent) = 0;

    /**
     * @brief Resolve an ObjectID (from the selection/picking buffer) back to an Actor.
     * @param ObjectID The ObjectID to resolve.
     * @return The Actor associated with the ObjectID, or nullptr if none.
     */
    virtual FActor* GetActorByObjectID(uint32 ObjectID) const = 0;

    /**
     * @brief Allocate (or fetch) the stable ObjectID for an Actor. Called on the main thread, e.g. when
     * marshalling the editor selection into a frame render packet.
     * @param Actor The Actor to allocate or fetch an ObjectID for.
     * @return The stable ObjectID for the Actor.
     */
    virtual uint32 GetOrCreateObjectID(FActor* Actor) = 0;

    /**
     * @brief Remove the stable ObjectID for an Actor.
     * @param Actor The Actor whose ObjectID should be removed.
     */
    virtual void RemoveActorObjectID(FActor* Actor) = 0;
};
