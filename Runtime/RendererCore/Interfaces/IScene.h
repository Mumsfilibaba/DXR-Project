#pragma once
#include "Core/CoreTypes.h"

class FCamera;
class FLight;
class FStaticMeshComponent;
class FSkyboxComponent;
class FLightProbe;
class FActor;

struct IScene
{
    virtual ~IScene() = default;

    /** @brief Update the scene for the current frame. */
    virtual void Tick() = 0;

    /**
     * @brief Add a renderer-side proxy for a camera.
     * @param InCamera The camera to add to the scene.
     */
    virtual void AddCamera(FCamera* InCamera) = 0;

    /**
     * @brief Add a light to the scene.
     * @param InLight The light to add to the scene.
     */
    virtual void AddLight(FLight* InLight) = 0;

    /**
     * @brief Add a light-probe to the scene.
     * @param InLightProbe The light-probe to add to the scene.
     */
    virtual void AddLightProbe(FLightProbe* InLightProbe) = 0;

    /**
     * @brief Add a skybox to the scene.
     * @param InSkyboxComponent The skybox component to add to the scene.
     */
    virtual void AddSkybox(FSkyboxComponent* InSkyboxComponent) = 0;

    /**
     * @brief Add a static mesh to the scene.
     * @param InMeshComponent The static-mesh component to add to the scene.
     */
    virtual void AddStaticMesh(FStaticMeshComponent* InMeshComponent) = 0;

    /**
     * @brief Remove a previously-added light from the scene.
     * @param InLight The light to remove from the scene.
     */
    virtual void RemoveLight(FLight* InLight) = 0;

    /**
     * @brief Remove a previously-added light-probe from the scene.
     * @param InLightProbe The light-probe to remove from the scene.
     */
    virtual void RemoveLightProbe(FLightProbe* InLightProbe) = 0;

    /**
     * @brief Remove a previously-added static mesh from the scene.
     * @param InMeshComponent The static-mesh component to remove from the scene.
     */
    virtual void RemoveStaticMesh(FStaticMeshComponent* InMeshComponent) = 0;

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
};
