#pragma once
#include "Core/Containers/Map.h"
#include "Renderer/FrameResources.h"
#include "Renderer/RenderPass.h"

class FMaterial;
class FScene;

#if EDITOR_BUILD

class FEditorNoJitterDepthPass : public FRenderPass
{
public:
    explicit FEditorNoJitterDepthPass(FSceneRenderer* InRenderer);
    virtual ~FEditorNoJitterDepthPass();

    virtual void PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
};

class FEditorSelectionIDPass : public FRenderPass
{
public:
    explicit FEditorSelectionIDPass(FSceneRenderer* InRenderer);
    virtual ~FEditorSelectionIDPass();

    virtual void PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
};

#endif
