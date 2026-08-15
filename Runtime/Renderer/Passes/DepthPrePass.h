#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

#include "Renderer/Settings/RenderFeatureSettings.h"

extern bool GPrePassBindless;

class FDepthPrePass : public FRenderPass
{
public:
    FDepthPrePass(FSceneRenderer* InRenderer);
    virtual ~FDepthPrePass();

    virtual void PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, const FPassResources& PassResources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
};
