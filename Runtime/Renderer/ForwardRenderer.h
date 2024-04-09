#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"

class FForwardRenderer : public FRenderPass
{
public:
    FForwardRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FFrameResources& FrameResources);
    void Release();

    void Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup, FScene* Scene);

private:
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          VShader;
    FRHIPixelShaderRef           PShader;
};