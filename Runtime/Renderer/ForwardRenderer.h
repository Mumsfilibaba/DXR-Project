#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

class RENDERER_API FForwardRenderer
{
public:
    FForwardRenderer()  = default;
    ~FForwardRenderer() = default;

    bool Init(FFrameResources& FrameResources);
    void Release();

    void Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

private:
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          VShader;
    FRHIPixelShaderRef           PShader;
};