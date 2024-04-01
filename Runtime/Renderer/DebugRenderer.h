#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

class FDebugRenderer : public FRenderPass
{
public:
    FDebugRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FFrameResources& Resources);
    
    void Release();

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources);
    
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FRendererScene* Scene);

private:
    FRHIBufferRef                AABBVertexBuffer;
    FRHIBufferRef                AABBIndexBuffer;
    uint32                       AABBIndexCount = 0;

    FRHIGraphicsPipelineStateRef AABBDebugPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;

    FRHIGraphicsPipelineStateRef LightDebugPSO;
    FRHIVertexShaderRef          LightDebugVS;
    FRHIPixelShaderRef           LightDebugPS;

    FRHIBufferRef                DbgSphereVertexBuffer;
    FRHIBufferRef                DbgSphereIndexBuffer;
    uint32                       DbgSphereIndexCount = 0;
};