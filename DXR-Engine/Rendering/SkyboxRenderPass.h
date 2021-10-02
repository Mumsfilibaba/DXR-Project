#pragma once
#include "FrameResources.h"

#include "Assets/MeshFactory.h"

#include "RHICore/RHICommandList.h"

#include "Scene/Scene.h"

class CSkyboxRenderPass final
{
public:
    CSkyboxRenderPass() = default;
    ~CSkyboxRenderPass() = default;

    bool Init( SFrameResources& FrameResources );

    void Render( CRHICommandList& CmdList, const SFrameResources& FrameResources, const CScene& Scene );

    void Release();

private:
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIVertexShader> SkyboxVertexShader;
    TSharedRef<CRHIPixelShader>  SkyboxPixelShader;
    TSharedRef<CRHIVertexBuffer> SkyboxVertexBuffer;
    TSharedRef<CRHIIndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<CRHISamplerState> SkyboxSampler;

    SMeshData SkyboxMesh;
};