#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FRenderGraphBuilder;

class FFXAAPass : public FRenderPass
{
public:
    FFXAAPass(FSceneRenderer* InRenderer);
    virtual ~FFXAAPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources);

    FRHIGraphicsPipelineStateRef FXAAPSO;
    EFormat                      FXAAPSOFormat = EFormat::Unknown;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    EFormat                      FXAADebugPSOFormat = EFormat::Unknown;
    FRHIPixelShaderRef           FXAADebugShader;
    FRHIVertexShaderRef          FXAAVertexShader;
    FRHIDepthStencilStateRef     FXAADepthStencilState;
    FRHIRasterizerStateRef       FXAARasterizerState;
    FRHIBlendStateRef            FXAABlendState;
};
