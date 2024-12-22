#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
#include "Renderer/RendererUtilities/GPUTextureCompressor.h"

class FLightProbeRenderer : public FRenderPass
{
public:
    FLightProbeRenderer(FSceneRenderer* InRenderer);
    virtual ~FLightProbeRenderer();

    bool Initialize(FFrameResources& FrameResources);
    void RenderSkyLightProbe(FRHICommandList& CommandList, FFrameResources& Resources);

private:
    bool CreateSkyLightResources(FFrameResources& Resources);

    FGPUTextureCompressor       Compressor;
    FRHIComputePipelineStateRef IrradianceGenPSO;
    FRHIComputeShaderRef        IrradianceGenShader;
    FRHIComputePipelineStateRef SpecularIrradianceGenPSO;
    FRHIComputeShaderRef        SpecularIrradianceGenShader;
};
