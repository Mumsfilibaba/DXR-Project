#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"

class FTonemapPass : public FRenderPass
{
public:
    FTonemapPass(FSceneRenderer* InRenderer);
    virtual ~FTonemapPass();

    bool Initialize(const FFrameResources& FrameResources);
    void Release();

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
    void Release();

    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;
};