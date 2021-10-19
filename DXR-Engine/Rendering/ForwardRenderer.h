#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "CoreRHI/RHICommandList.h"

class CORE_API CForwardRenderer
{
public:
    CForwardRenderer() = default;
    ~CForwardRenderer() = default;

    bool Init( SFrameResources& FrameResources );
    void Release();

    void Render( CRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup );

private:
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIVertexShader>          VShader;
    TSharedRef<CRHIPixelShader>           PShader;
};