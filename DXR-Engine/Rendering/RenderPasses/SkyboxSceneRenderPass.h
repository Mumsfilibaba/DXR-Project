#pragma once
#include "SceneRenderPass.h"

#include "Rendering/MeshFactory.h"

class SkyboxSceneRenderPass final : public SceneRenderPass
{
public:
    SkyboxSceneRenderPass()  = default;
    ~SkyboxSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResource,
        const Scene& Scene) override final;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<VertexBuffer> SkyboxVertexBuffer;
    TSharedRef<IndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<SamplerState> SkyboxSampler;

    MeshData SkyboxMesh;
};