#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDebugRenderer

class FDebugRenderer
{
public:
    FDebugRenderer()  = default;
    ~FDebugRenderer() = default;

    bool Init(FFrameResources& Resources);
    void Release();

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, const FScene& Scene);

private:
    FRHIVertexBufferRef          AABBVertexBuffer;
    FRHIIndexBufferRef           AABBIndexBuffer;
    FRHIGraphicsPipelineStateRef AABBDebugPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;

    FRHIGraphicsPipelineStateRef LightDebugPSO;
    FRHIVertexShaderRef          LightDebugVS;
    FRHIPixelShaderRef           LightDebugPS;
    FRHIVertexBufferRef          DbgSphereVertexBuffer;
    FRHIIndexBufferRef           DbgSphereIndexBuffer;
};