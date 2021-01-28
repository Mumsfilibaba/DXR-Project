#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

class LightProbeRenderer
{
public:
    LightProbeRenderer()  = default;
    ~LightProbeRenderer() = default;

    Bool Init(
        SceneLightSetup& LightSetup,
        FrameResources& FrameResources);
    
    void Release();

    void RenderSkyLightProbe(
        CommandList& CmdList, 
        const SceneLightSetup& LightSetup,
        const FrameResources& Resources);

private:
    Bool CreateSkyLightResources(SceneLightSetup& LightSetup);

    TSharedRef<ComputePipelineState> IrradianceGenPSO;
    TSharedRef<ComputePipelineState> SpecularIrradianceGenPSO;
};