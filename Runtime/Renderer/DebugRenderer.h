#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDebugRenderer

class FDebugRenderer
{
public:
    FDebugRenderer()  = default;
    ~FDebugRenderer() = default;

    bool Init(FFrameResources& Resources);
    void Release();

    void RenderObjectAABBs(FRHICommandList& CmdList, FFrameResources& Resources);

private:
    FRHIVertexBufferRef          AABBVertexBuffer;
    FRHIIndexBufferRef           AABBIndexBuffer;
    FRHIGraphicsPipelineStateRef AABBDebugPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;
};