#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "RendererCore/Interfaces/ISceneObject.h"

class FPointLight;
class FDirectionalLight;
class FSkyLight;

struct FScenePointLight : public ISceneObject
{
    struct FShadowData
    {
        FMatrix4 Matrix;
        FVector3 Position;
        float    NearPlane;
        float    FarPlane;
    };

    FScenePointLight(FPointLight* InPointLight);
    ~FScenePointLight();

    virtual void Tick() override final { }

    // Pointer to the light in the world
    FPointLight* PointLight;

    // Shadow generation information
    FFrustum    Frustums[RHI_NUM_CUBE_FACES];
    FShadowData ShadowData[RHI_NUM_CUBE_FACES];
    
    // Store data for each face
    TArray<FMeshBatch>            MeshBatches[RHI_NUM_CUBE_FACES];
    TArray<FProxySceneComponent*> Primitives[RHI_NUM_CUBE_FACES];

    // Store data for a single pass cube-map
    TArray<FMeshBatch>            SinglePassMeshBatch;
    TArray<FProxySceneComponent*> SinglePassPrimitives;
};

struct FSceneDirectionalLight : public ISceneObject
{
    FSceneDirectionalLight(FDirectionalLight* InDirectionalLight);
    ~FSceneDirectionalLight();

    virtual void Tick() override final { }

    // Pointer to the light in the world
    FDirectionalLight* DirectionalLight;

    // Store data for rendering all cascades
    TArray<FMeshBatch>            MeshBatches;
    TArray<FProxySceneComponent*> Primitives;
};

struct FSceneSkyLight : public ISceneObject
{
    FSceneSkyLight(FSkyLight* InSkyLight);
    ~FSceneSkyLight();

    virtual void Tick() override final { }

    // Filters the source into the necessary cube-maps
    void FilterStaticCubeMaps();

    // Pointer to the light in the world
    FSkyLight* SkyLight;

    // Source cube-map
    FRHITextureRef SourceCubeMap;

    // Cube-maps
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;
};