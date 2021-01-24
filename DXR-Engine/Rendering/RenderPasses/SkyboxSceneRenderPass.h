#pragma once
#include "SceneRenderPass.h"

#include "Rendering/MeshFactory.h"

class SkyboxSceneRenderPass final : public SceneRenderPass
{
public:
    SkyboxSceneRenderPass()  = default;
    ~SkyboxSceneRenderPass() = default;

    Bool Init(SharedRenderPassResources& FrameResource);

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResource) override final;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<VertexBuffer> SkyboxVertexBuffer;
    TSharedRef<IndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<SamplerState> SkyboxSampler;

    MeshData SkyboxMesh;
};