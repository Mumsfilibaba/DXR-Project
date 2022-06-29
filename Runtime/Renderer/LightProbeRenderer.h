#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CLightProbeRenderer

class RENDERER_API CLightProbeRenderer
{
public:
    CLightProbeRenderer() = default;
    ~CLightProbeRenderer() = default;

    bool Init(SLightSetup& LightSetup, SFrameResources& FrameResources);

    void Release();

    void RenderSkyLightProbe(FRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& Resources);

private:
    bool CreateSkyLightResources(SLightSetup& LightSetup);

    TSharedRef<FRHIComputePipelineState> IrradianceGenPSO;
    TSharedRef<FRHIComputeShader>        IrradianceGenShader;
    TSharedRef<FRHIComputePipelineState> SpecularIrradianceGenPSO;
    TSharedRef<FRHIComputeShader>        SpecularIrradianceGenShader;
};