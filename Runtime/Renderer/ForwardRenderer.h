#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDeferredRenderer

class RENDERER_API CForwardRenderer
{
public:
    CForwardRenderer() = default;
    ~CForwardRenderer() = default;

    bool Init(SFrameResources& FrameResources);
    void Release();

    void Render(FRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup);

private:
    TSharedRef<FRHIGraphicsPipelineState> PipelineState;
    TSharedRef<FRHIVertexShader>          VShader;
    TSharedRef<FRHIPixelShader>           PShader;
};