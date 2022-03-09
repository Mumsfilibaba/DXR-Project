#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShadowMapRenderer

class RENDERER_API CSkyboxRenderPass final
{
public:
    CSkyboxRenderPass() = default;
    ~CSkyboxRenderPass() = default;

    bool Init(SFrameResources& FrameResources);

    void Render(CRHICommandList& CmdList, const SFrameResources& FrameResources, const CScene& Scene);

    void Release();

private:
    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
    TSharedRef<CRHIVertexShader> SkyboxVertexShader;
    TSharedRef<CRHIPixelShader>  SkyboxPixelShader;
    TSharedRef<CRHIBuffer> SkyboxVertexBuffer;
    TSharedRef<CRHIBuffer>  SkyboxIndexBuffer;
    TSharedRef<CRHISamplerState> SkyboxSampler;

    SMeshData SkyboxMesh;
};