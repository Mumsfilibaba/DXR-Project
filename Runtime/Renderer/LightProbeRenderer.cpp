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
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Wrap;
    SamplerDesc.AddressV = ESamplerMode::Wrap;
    SamplerDesc.AddressW = ESamplerMode::Wrap;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.LightProbeSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!FrameResources.LightProbeSampler)
    {
        return false;
    }

    return true;
}
