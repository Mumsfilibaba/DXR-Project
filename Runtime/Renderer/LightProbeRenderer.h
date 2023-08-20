#pragma once
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"

class RENDERER_API FLightProbeRenderer
{
public:
    bool Init(FLightSetup& LightSetup, FFrameResources& FrameResources);

    void Release();

    void RenderSkyLightProbe(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& Resources);

private:
    bool CreateSkyLightResources(FLightSetup& LightSetup);

    FRHIComputePipelineStateRef IrradianceGenPSO;
    FRHIComputeShaderRef        IrradianceGenShader;
    FRHIComputePipelineStateRef SpecularIrradianceGenPSO;
    FRHIComputeShaderRef        SpecularIrradianceGenShader;
};