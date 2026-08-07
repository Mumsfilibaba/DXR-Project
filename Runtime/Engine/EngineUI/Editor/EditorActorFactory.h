#pragma once
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Engine/EngineModule.h"

class FActor;
class FWorld;

enum class EEditorPrimitiveType : uint8
{
    Cube,
    Sphere,
    Plane,
    Cylinder,
    Cone,
    Torus,
    Pyramid,
    Teapot,
    Count,
};

enum class EEditorLightType : uint8
{
    Point,
    Spot,
    Directional,
    Sky,
};

struct ENGINE_API EditorActorFactory
{
    /**
     * @brief Spawn a MeshFactory primitive resting on top of the given location
     *
     * @param World World to spawn into
     * @param Type Primitive to build the mesh from
     * @param Location Point on the surface the mesh should sit on
     * @return Returns the new actor, or nullptr if it could not be created
     */
    static FActor* SpawnPrimitive(FWorld* World, EEditorPrimitiveType Type, const Vector3& Location);

    /**
     * @brief Spawn a light actor at the given location
     *
     * @param World World to spawn into
     * @param Type Kind of light to create
     * @param Location Position of the light
     * @return Returns the new actor, or nullptr if it could not be created
     */
    static FActor* SpawnLight(FWorld* World, EEditorLightType Type, const Vector3& Location);

    /**
     * @brief Spawn a camera actor at the given location
     *
     * @param World World to spawn into
     * @param Location Position of the camera
     * @return Returns the new actor, or nullptr if it could not be created
     */
    static FActor* SpawnCamera(FWorld* World, const Vector3& Location);

    /**
     * @brief Draw the Mesh, Light and Camera entries that place a new actor
     *
     * Shared by the viewport and Scene Hierarchy context menus so both offer the same set of actors. The caller
     * supplies the surrounding menu, and is expected to be inside an open popup.
     *
     * @param World World to spawn into
     * @param Location Where a picked entry should place its actor
     * @return Returns the actor spawned this frame, or nullptr if no entry was picked
     */
    static FActor* DrawPlaceActorMenu(FWorld* World, const Vector3& Location);

    /**
     * @brief Check whether a light of the given type can be spawned
     *
     * A spot light is never drawn, because the renderer has no spot light support at all. A sky light is rejected
     * by the scene without a cube map, and the editor has no way to author one, so a new sky light can only
     * borrow the cube map from one that already exists.
     *
     * @param World World to search for an existing sky light
     * @param Type Kind of light the caller wants to create
     * @return Returns true if spawning the light would produce something the renderer can draw
     */
    static bool CanSpawnLight(FWorld* World, EEditorLightType Type);

    /**
     * @brief Build an actor name that no actor in the world is using yet
     *
     * @param World World to check for collisions
     * @param BaseName Name to use as-is when it is free, and as the stem of "BaseName (N)" when it is not
     * @return Returns the unique name
     */
    static String MakeUniqueActorName(FWorld* World, const CHAR* BaseName);

    /** @brief Drop the cached primitive meshes, which must happen before the RHI is torn down. */
    static void ReleaseCachedMeshes();

    /** @brief Display name of a primitive, also used as the base name of the spawned actor. */
    static const CHAR* GetPrimitiveName(EEditorPrimitiveType Type);

    /** @brief Display name of a light type, also used as the base name of the spawned actor. */
    static const CHAR* GetLightName(EEditorLightType Type);
};
