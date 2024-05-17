#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"

class FDebugRenderer : public FRenderPass
{
public:
    FDebugRenderer(FSceneRenderer* InRenderer);
    virtual ~FDebugRenderer();

    bool Initialize(FFrameResources& Resources);

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);

private:
    FRHIBufferRef                AABBVertexBuffer;
    FRHIBufferRef                AABBIndexBuffer;
    uint32                       AABBIndexCount;

    FRHIGraphicsPipelineStateRef AABBDebugPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;

    FRHIGraphicsPipelineStateRef LightDebugPSO;
    FRHIVertexShaderRef          LightDebugVS;
    FRHIPixelShaderRef           LightDebugPS;

    FRHIBufferRef                DbgSphereVertexBuffer;
    FRHIBufferRef                DbgSphereIndexBuffer;
    uint32                       DbgSphereIndexCount;
};