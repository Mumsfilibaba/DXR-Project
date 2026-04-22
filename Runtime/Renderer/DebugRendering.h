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

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);
    void RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);

private:
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
    FRHIGraphicsPipelineStateRef AABB_NoDepth_PSO_BB;
    FRHIGraphicsPipelineStateRef AABB_Depth_PSO;
    FRHIVertexShaderRef          AABB_VS;
    FRHIPixelShaderRef           AABB_PS;

    // Solid AABBs
    FRHIGraphicsPipelineStateRef AABBSolid_PSO;
    FRHIGraphicsPipelineStateRef AABBSolid_PSO_BB;
    FRHIVertexShaderRef          AABBSolid_VS;
    FRHIPixelShaderRef           AABBSolid_PS;

    // PointLights
    FRHIGraphicsPipelineStateRef LightDebug_PSO;
    FRHIGraphicsPipelineStateRef LightDebug_PSO_BB;
    FRHIVertexShaderRef          LightDebug_VS;
    FRHIPixelShaderRef           LightDebug_PS;

    // LightProbes
    FRHIGraphicsPipelineStateRef ProbeDebug_PSO;
    FRHIGraphicsPipelineStateRef ProbeDebug_PSO_BB;
    FRHIVertexShaderRef          ProbeDebug_VS;
    FRHIPixelShaderRef           ProbeDebug_PS;
};