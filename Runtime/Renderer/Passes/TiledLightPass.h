#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

class FTiledLightPass : public FRenderPass
{
public:
    FTiledLightPass(FSceneRenderer* InRenderer);
    virtual ~FTiledLightPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

    FRHIComputePipelineStateRef TiledLightPassPSO;
    FRHIComputeShaderRef        TiledLightShader;
    FRHIComputePipelineStateRef TiledLightPassPSO_TileDebug;
    FRHIComputeShaderRef        TiledLightShader_TileDebug;
    FRHIComputePipelineStateRef TiledLightPassPSO_CascadeDebug;
    FRHIComputeShaderRef        TiledLightShader_CascadeDebug;
};
