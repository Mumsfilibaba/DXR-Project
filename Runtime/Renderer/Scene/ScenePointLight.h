#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"

class FPointLight;

struct FScenePointLight : public FSceneObject
{
    struct FShadowData
    {
        FMatrix4 Matrix;
        FVector3 Position;
        float    NearPlane;
        float    FarPlane;
    };

    FScenePointLight(FScene* InScene, FPointLight* InPointLight);
    ~FScenePointLight();

    virtual void Tick() override final { }

    // Pointer to the light in the world
    FPointLight* PointLight;

    // Shadow generation information
    FFrustum    Frustums[RHI_NUM_CUBE_FACES];
    FShadowData ShadowData[RHI_NUM_CUBE_FACES];
    
    // Store data for each face
    TArray<FMeshBatch>        MeshBatches[RHI_NUM_CUBE_FACES];
    TArray<FSceneStaticMesh*> StaticMeshes[RHI_NUM_CUBE_FACES];

    // Store data for a single pass cube-map
    TArray<FMeshBatch>        SinglePassMeshBatch;
    TArray<FSceneStaticMesh*> SinglePassStaticMeshes;
};
