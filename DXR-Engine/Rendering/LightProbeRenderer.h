#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

class LightProbeRenderer
{
public:
    LightProbeRenderer() = default;
    ~LightProbeRenderer() = default;

    bool Init( LightSetup& LightSetup, FrameResources& FrameResources );

    void Release();

    void RenderSkyLightProbe( CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& Resources );

private:
    bool CreateSkyLightResources( LightSetup& LightSetup );

    TRef<ComputePipelineState> IrradianceGenPSO;
    TRef<ComputeShader>        IrradianceGenShader;
    TRef<ComputePipelineState> SpecularIrradianceGenPSO;
    TRef<ComputeShader>        SpecularIrradianceGenShader;
};