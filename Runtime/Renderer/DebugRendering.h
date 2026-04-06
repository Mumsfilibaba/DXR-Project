#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

class FDebugRenderer : public FRenderPass
{
public:
    FDebugRenderer(FSceneRenderer* InRenderer);
    virtual ~FDebugRenderer();

    bool Initialize(FFrameResources& Resources);

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderCascadeSplitFrustums(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);

private:
    struct FCascadeLineVertex
    {
        FVector3 Position;
        FVector4 Color;
    };

    static constexpr uint32 NumCascadeFrustumPlanes = 6;

    void UpdateCascadeSplitReadback();
    void QueueCascadeSplitReadback(FRHICommandList& CommandList, FFrameResources& Resources);
    bool BuildCascadeFrustumVertices(const FFrameResources& Resources, const FScene* Scene, TArray<FCascadeLineVertex>& OutVertices) const;

    // Geometry Data
    FRHIBufferRef                AABBVertexBuffer;
    FRHIBufferRef                AABBIndexBuffer_Wireframe;
    FRHIBufferRef                AABBIndexBuffer_Solid;
    uint32                       AABBIndexCount_Wireframe;
    uint32                       AABBIndexCount_Solid;

    FRHIBufferRef                SphereVertexBuffer;
    FRHIBufferRef                SphereIndexBuffer;
    uint32                       SphereIndexCount;

    // Wireframe AABBs
    FRHIGraphicsPipelineStateRef AABB_NoDepth_PSO;
    FRHIGraphicsPipelineStateRef AABB_Depth_PSO;
    FRHIVertexShaderRef          AABB_VS;
    FRHIPixelShaderRef           AABB_PS;

    // Solid AABBs
    FRHIGraphicsPipelineStateRef AABBSolid_PSO;
    FRHIVertexShaderRef          AABBSolid_VS;
    FRHIPixelShaderRef           AABBSolid_PS;

    // PointLights
    FRHIGraphicsPipelineStateRef LightDebug_PSO;
    FRHIVertexShaderRef          LightDebug_VS;
    FRHIPixelShaderRef           LightDebug_PS;

    // LightProbes
    FRHIGraphicsPipelineStateRef ProbeDebug_PSO;
    FRHIVertexShaderRef          ProbeDebug_VS;
    FRHIPixelShaderRef           ProbeDebug_PS;

    // Cascade Split Frustum Debug
    FRHIGraphicsPipelineStateRef CascadeFrustumDebug_PSO;
    FRHIVertexShaderRef          CascadeFrustumDebug_VS;
    FRHIPixelShaderRef           CascadeFrustumDebug_PS;
    FRHIBufferRef                CascadeSplitReadbackBuffer;
    FRHIFenceRef                 CascadeSplitReadbackFence;
    bool                         bCascadeSplitReadbackInFlight;
    bool                         bHasCascadeSplitDistances;
    uint64                       CascadeSplitReadbackSize;
    uint64                       CascadeSplitStrideBytes;
    FVector4                     CachedCascadeFrustumPlanes[NUM_SHADOW_CASCADES][NumCascadeFrustumPlanes];
    float                        CachedCascadeSplitStart[NUM_SHADOW_CASCADES];
    float                        CachedCascadeSplitEnd[NUM_SHADOW_CASCADES];
};
