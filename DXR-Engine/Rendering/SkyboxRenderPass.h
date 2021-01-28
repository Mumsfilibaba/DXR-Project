#pragma once
#include "FrameResources.h"

#include "Rendering/MeshFactory.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class SkyboxRenderPass final
{
public:
    SkyboxRenderPass()  = default;
    ~SkyboxRenderPass() = default;

    Bool Init(FrameResources& FrameResources);

    void Render(
        CommandList& CmdList, 
        const FrameResources& FrameResources,
        const Scene& Scene);

    void Release();

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<VertexBuffer> SkyboxVertexBuffer;
    TSharedRef<IndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<SamplerState> SkyboxSampler;

    MeshData SkyboxMesh;
};