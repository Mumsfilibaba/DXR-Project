#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

class FForwardPass : public FRenderPass
{
public:
    FForwardPass(FSceneRenderer* InRenderer);
    virtual ~FForwardPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);
    FGraphicsPipelineStateInstance* CompilePipelineState(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> PipelineStates;
};
