#pragma once
#include "Core/Containers/UniquePtr.h"
#include "RHI/RHICommandList.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"
#include "RendererCore/RayTracing/AccelerationStructureCacheBackend.h"
#include "RendererCore/RayTracing/AccelerationStructureCompactionHelper.h"
#include "RendererCore/RayTracing/AccelerationStructureSerializationHelper.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"

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
    uint32                                             GeometryTableCapacity;
    FAccelerationStructureCache                        ASCache;
    TUniquePtr<FDiskAccelerationStructureCacheBackend> ASCacheBackend;
    FAccelerationStructureCompactionHelper             CompactionHelper;
    FAccelerationStructureSerializationHelper          SerializationHelper;
    TArray<FRHIRayTracingAccelerationStructure*>       CompactionRequested;
    TArray<FRHIRayTracingAccelerationStructure*>       SerializationRequested;
};
