#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightProbeRenderer

class RENDERER_API FLightProbeRenderer
{
public:
    FLightProbeRenderer() = default;
    ~FLightProbeRenderer() = default;

    bool Init(SLightSetup& LightSetup, SFrameResources& FrameResources);

    void Release();

    void RenderSkyLightProbe(FRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& Resources);

private:
    bool CreateSkyLightResources(SLightSetup& LightSetup);

    FRHIComputePipelineStateRef IrradianceGenPSO;
    FRHIComputeShaderRef        IrradianceGenShader;
    FRHIComputePipelineStateRef SpecularIrradianceGenPSO;
    FRHIComputeShaderRef        SpecularIrradianceGenShader;
};