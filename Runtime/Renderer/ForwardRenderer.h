#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CDeferredRenderer

class RENDERER_API CForwardRenderer
{
public:
    CForwardRenderer() = default;
    ~CForwardRenderer() = default;

    bool Init(SFrameResources& FrameResources);
    void Release();

    void Render(CRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup);

private:
    CRHIGraphicsPipelineStateRef PipelineState;
    CRHIVertexShaderRef          VShader;
    CRHIPixelShaderRef           PShader;
};