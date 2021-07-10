#pragma once
#include "FrameResources.h"

#include "Rendering/Resources/MeshFactory.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class SkyboxRenderPass final
{
public:
    SkyboxRenderPass() = default;
    ~SkyboxRenderPass() = default;

    bool Init( FrameResources& FrameResources );

    void Render( CommandList& CmdList, const FrameResources& FrameResources, const Scene& Scene );

    void Release();

private:
    TRef<GraphicsPipelineState> PipelineState;
    TRef<VertexShader> SkyboxVertexShader;
    TRef<PixelShader>  SkyboxPixelShader;
    TRef<VertexBuffer> SkyboxVertexBuffer;
    TRef<IndexBuffer>  SkyboxIndexBuffer;
    TRef<SamplerState> SkyboxSampler;

    MeshData SkyboxMesh;
};