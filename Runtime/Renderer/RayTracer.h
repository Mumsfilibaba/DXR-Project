#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"
#include "RendererCore/RayTracing/AccelerationStructureCacheBackend.h"
#include "RendererCore/RayTracing/AccelerationStructureCompactionHelper.h"
#include "RendererCore/RayTracing/AccelerationStructureSerializationHelper.h"
#include "Core/Containers/UniquePtr.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

class FRayTracer : public FRenderPass
{
public:
    FRayTracer(FSceneRenderer* InRenderer);
    ~FRayTracer();

    bool Initialize(FFrameResources& Resources);
    void Release();

    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    bool NeedsReflectionReconfigure() const;

    void PreRender(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderPrimaryRayDebug(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void ReleaseRayTracingGeometry(FScene* Scene);

    void InvalidateReflectionHistory()
    {
        bReflectionHistoryValid = false;
    }

private:
    FRHIRayTracingPipelineStateRef LocalPipeline;
    FRHIRayGenShaderRef            RayGenShader;
    FRHIRayMissShaderRef           RayMissShader;
    FRHIRayClosestHitShaderRef     RayClosestHitShader;
    FRHIRayTracingPipelineStateRef BindlessPipeline;
    FRHIRayGenShaderRef            RayGenShaderBindless;
    FRHIRayMissShaderRef           RayMissShaderBindless;
    FRHIRayClosestHitShaderRef     RayClosestHitShaderBindless;
    FRHIComputeShaderRef           InlineReflectionsShader;
    FRHIComputePipelineStateRef    InlineReflectionsPipeline;
    FRHIComputeShaderRef           PrimaryRayDebugShader;
    FRHIComputePipelineStateRef    PrimaryRayDebugPipeline;
    FRHIRayGenShaderRef            RayGenShaderSER;
    FRHIRayTracingPipelineStateRef SERPipeline;
    uint32                         CurrentSERHitGroupCapacity;
    uint32                         CurrentHitGroupCapacity;
    uint32                         CurrentBindlessHitGroupCapacity;
    uint32                         GeometryTableCapacity;

    FAccelerationStructureCache                        ASCache;
    TUniquePtr<FDiskAccelerationStructureCacheBackend> ASCacheBackend;
    FAccelerationStructureCompactionHelper             CompactionHelper;
    FAccelerationStructureSerializationHelper          SerializationHelper;
    TArray<FRHIRayTracingAccelerationStructure*>       CompactionRequested;
    TArray<FRHIRayTracingAccelerationStructure*>       SerializationRequested;

    FRHIComputeShaderRef           ReflectionTemporalShader;
    FRHIComputePipelineStateRef    ReflectionTemporalPipeline;
    FRHIComputeShaderRef           ReflectionAtrousShader;
    FRHIComputePipelineStateRef    ReflectionAtrousPipeline;
    FRHIComputeShaderRef           ReflectionUpsampleShader;
    FRHIComputePipelineStateRef    ReflectionUpsamplePipeline;
    uint32                         ReflectionHistoryIndex;
    bool                           bReflectionHistoryValid;
    bool                           bDenoiserHalfRes;
    uint32                         DenoiserFullWidth;
    uint32                         DenoiserFullHeight;

    void DenoiseReflections(FRHICommandList& CommandList, FFrameResources& Resources);
    void BuildSceneAccelerationData(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bNeedBindlessData);
};