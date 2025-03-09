#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Renderer/LightProbeRenderer.h"

FLightProbeRenderer::FLightProbeRenderer(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FLightProbeRenderer::~FLightProbeRenderer()
{
}

bool FLightProbeRenderer::Initialize(FFrameResources& FrameResources)
{
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Wrap;
    SamplerInfo.AddressV = ESamplerMode::Wrap;
    SamplerInfo.AddressW = ESamplerMode::Wrap;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.LightProbeSampler = RHICreateSamplerState(SamplerInfo);
    if (!FrameResources.LightProbeSampler)
    {
        return false;
    }

    return true;
}
