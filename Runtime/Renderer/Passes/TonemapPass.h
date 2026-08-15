#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

enum class ETonemappingType : int32
{
    Unknown    = 0,
    ACES       = 1,
    Reinhard   = 2,
    Uncharted2 = 3,
};

struct FTonemapInfoHLSL
{
    ETonemappingType TonemappingType;
    int32            bOutputSRGB;
    float            ReinhardIntensity;
    float            Padding0;
};

MARK_AS_REALLOCATABLE(FTonemapInfoHLSL);

class FRenderGraphBuilder;

class FTonemapPass : public FRenderPass
{
public:
    FTonemapPass(FSceneRenderer* InRenderer);
    virtual ~FTonemapPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, FRHITexture* OutputTarget, bool bOutputSRGB);

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* OutputTarget, bool bOutputSRGB);

    FRHIGraphicsPipelineStateRef TonemapPSO;
    EFormat                      TonemapPSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          TonemapVertexShader;
    FRHIPixelShaderRef           TonemapShader;
    FRHIDepthStencilStateRef     TonemapDepthStencilState;
    FRHIRasterizerStateRef       TonemapRasterizerState;
    FRHIBlendStateRef            TonemapBlendState;
};
