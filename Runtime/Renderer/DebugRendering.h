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
    void RenderOcclusionVolumes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);

private:
    // Object AABBS
    FRHIBufferRef                AABBVertexBuffer;
    FRHIBufferRef                AABBIndexBuffer;
    uint32                       AABBIndexCount;

    FRHIGraphicsPipelineStateRef AABBPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;

    // Occlusion Volumes
    FRHIGraphicsPipelineStateRef OcclusionVolumePSO;
    FRHIVertexShaderRef          OcclusionVolumeVS;
    FRHIPixelShaderRef           OcclusionVolumePS;

    // PointLights
    FRHIGraphicsPipelineStateRef LightDebugPSO;
    FRHIVertexShaderRef          LightDebugVS;
    FRHIPixelShaderRef           LightDebugPS;

    FRHIBufferRef                DbgSphereVertexBuffer;
    FRHIBufferRef                DbgSphereIndexBuffer;
    uint32                       DbgSphereIndexCount;
};