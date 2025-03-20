#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

enum class ETonemappingType : int32
{
    Unknown    = 0,
    ACES       = 1,
    Reinhard   = 2,
    Uncharted2 = 3,
};

struct FTonemapInfoHLSL
{
    // 0-16
    ETonemappingType TonemappingType;
    float            ReinhardIntensity;
    float            Padding0;
    float            Padding1;
};

MARK_AS_REALLOCATABLE(FTonemapInfoHLSL);

class FTonemapPass : public FRenderPass
{
public:
    FTonemapPass(FSceneRenderer* InRenderer);
    virtual ~FTonemapPass();

    bool Initialize(const FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIGraphicsPipelineStateRef TonemapPSO;
    FRHIPixelShaderRef           TonemapShader;
};

class FFXAAPass : public FRenderPass
{
public:
    FFXAAPass(FSceneRenderer* InRenderer);
    virtual ~FFXAAPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;
};