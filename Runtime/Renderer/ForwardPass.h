#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"

class FForwardPass : public FRenderPass
{
public:
    FForwardPass(FSceneRenderer* InRenderer);
    virtual ~FForwardPass();

    bool Initialize(FFrameResources& FrameResources);
    void Release();

    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          VShader;
    FRHIPixelShaderRef           PShader;
};