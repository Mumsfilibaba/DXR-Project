#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "RendererUtilities/GPUTextureCompressor.h"

class FLightProbeRenderer : public FRenderPass
{
public:
    FLightProbeRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FLightSetup& LightSetup, FFrameResources& FrameResources);
    void Release();

    void RenderSkyLightProbe(FRHICommandList& CommandList, FLightSetup& LightSetup, const FFrameResources& Resources);

private:
    bool CreateSkyLightResources(FLightSetup& LightSetup);

    FGPUTextureCompressor       Compressor;

    FRHIComputePipelineStateRef IrradianceGenPSO;
    FRHIComputeShaderRef        IrradianceGenShader;
    FRHIComputePipelineStateRef SpecularIrradianceGenPSO;
    FRHIComputeShaderRef        SpecularIrradianceGenShader;
};
