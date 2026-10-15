#pragma once
#include "Core/Containers/Set.h"
#include "RHI/RHICommandList.h"
#include "RendererCore/RayTracing/AccelerationStructureCompactionHelper.h"
#include "RendererCore/RayTracing/AccelerationStructureSerializationHelper.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Passes/RayTracing/RayTracingStructureCache.h"
#include "Renderer/Graph/FrameResources.h"

class FMesh;
class FScene;

class FRayTracingSceneBuilder : public FRenderPass
{
public:
    FRayTracingSceneBuilder(FSceneRenderer* InRenderer);
    ~FRayTracingSceneBuilder();

    bool Initialize();
    void Release();

    void BuildSceneAccelerationData(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bNeedBindlessData);
    void ReleaseRayTracingResources(FScene* Scene);

private:
    struct FPendingCacheWrite
    {
        FRHIRayTracingAccelerationStructure* Structure = nullptr;
        FAccelerationStructureCache*         Cache     = nullptr;
        FString                              Key;
    };

    void RequestCompactionAndCacheWrite(FRHICommandList& CommandList, const FMesh& Mesh, FRHIGeometryAccelerationStructure* Geometry);
    void TickCompactionAndCacheWrites(FRHICommandList& CommandList);

    uint32                                     GeometryTableCapacity;
    FRayTracingStructureCache                  StructureCache;
    FAccelerationStructureCompactionHelper     CompactionHelper;
    FAccelerationStructureSerializationHelper  SerializationHelper;
    TSet<FRHIRayTracingAccelerationStructure*> HandledStructures;
    TArray<FPendingCacheWrite>                 AwaitingCompaction;
};
