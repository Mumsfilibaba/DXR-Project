#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
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

    bool Initialize(FFrameResources& FrameResources);
    void Release();

    void RenderSkyLightProbe(FRHICommandList& CommandList, FFrameResources& Resources);

private:
    bool CreateSkyLightResources(FFrameResources& Resources);

    FGPUTextureCompressor       Compressor;

    FRHIComputePipelineStateRef IrradianceGenPSO;
    FRHIComputeShaderRef        IrradianceGenShader;
    FRHIComputePipelineStateRef SpecularIrradianceGenPSO;
    FRHIComputeShaderRef        SpecularIrradianceGenShader;
};
