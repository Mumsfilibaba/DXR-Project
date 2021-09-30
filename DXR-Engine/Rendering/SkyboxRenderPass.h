#pragma once
#include "FrameResources.h"

#include "Assets/MeshFactory.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class SkyboxRenderPass final
{
public:
    SkyboxRenderPass() = default;
    ~SkyboxRenderPass() = default;

    bool Init( FrameResources& FrameResources );

    void Render( CommandList& CmdList, const FrameResources& FrameResources, const CScene& Scene );

    void Release();

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<VertexShader> SkyboxVertexShader;
    TSharedRef<PixelShader>  SkyboxPixelShader;
    TSharedRef<VertexBuffer> SkyboxVertexBuffer;
    TSharedRef<IndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<SamplerState> SkyboxSampler;

    SMeshData SkyboxMesh;
};