#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"

class FDirectionalLight;

struct FSceneDirectionalLight : public FSceneObject
{
    FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight);
    ~FSceneDirectionalLight();

    virtual void Tick() override final;

    // Pointer to the light in the world
    FDirectionalLight* DirectionalLight;

    // Store data for rendering all cascades
    TArray<FMeshBatch>        MeshBatches;
    TArray<FSceneStaticMesh*> StaticMeshes;

    FVector3 Direction;
    FVector3 Rotation;
    FVector3 UpVector;
    FVector3 LookAt;
    FVector3 Position;
    FMatrix4 ShadowMatrix;
    float    Size;
};
