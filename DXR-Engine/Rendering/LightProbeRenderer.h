#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHICore/RHICommandList.h"

class CLightProbeRenderer
{
public:
    CLightProbeRenderer() = default;
    ~CLightProbeRenderer() = default;

    bool Init( SLightSetup& LightSetup, SFrameResources& FrameResources );

    void Release();

    void RenderSkyLightProbe( CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& Resources );

private:
    bool CreateSkyLightResources( SLightSetup& LightSetup );

    TSharedRef<CRHIComputePipelineState> IrradianceGenPSO;
    TSharedRef<CRHIComputeShader>        IrradianceGenShader;
    TSharedRef<CRHIComputePipelineState> SpecularIrradianceGenPSO;
    TSharedRef<CRHIComputeShader>        SpecularIrradianceGenShader;
};