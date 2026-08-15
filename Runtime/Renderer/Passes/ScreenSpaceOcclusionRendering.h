#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

class FScreenSpaceOcclusionPass : public FRenderPass
{
public:
    FScreenSpaceOcclusionPass(FSceneRenderer* InRenderer);
    virtual ~FScreenSpaceOcclusionPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, const FPassResources& PassResources);

    FRHIComputePipelineStateRef PipelineState;
    FRHIComputeShaderRef        SSAOShader;
    
    FRHIComputePipelineStateRef BlurHorizontalPSO;
    FRHIComputeShaderRef        BlurHorizontalShader;
    
    FRHIComputePipelineStateRef BlurVerticalPSO;
    FRHIComputeShaderRef        BlurVerticalShader;
};