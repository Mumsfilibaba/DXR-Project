#pragma once
#include "Core/Containers/Map.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FMaterial;
class FScene;
class FRenderGraphBuilder;

#if EDITOR_BUILD

class FEditorNoJitterDepthPass : public FRenderPass
{
public:
    explicit FEditorNoJitterDepthPass(FSceneRenderer* InRenderer);
    virtual ~FEditorNoJitterDepthPass();

    virtual void PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
};

#endif
