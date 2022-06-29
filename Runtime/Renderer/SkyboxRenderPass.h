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
    TSharedRef<FRHIGraphicsPipelineState> PipelineState;
    TSharedRef<FRHIVertexShader> SkyboxVertexShader;
    TSharedRef<FRHIPixelShader>  SkyboxPixelShader;
    TSharedRef<CRHIVertexBuffer> SkyboxVertexBuffer;
    TSharedRef<FRHIIndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<FRHISamplerState> SkyboxSampler;

    SMeshData SkyboxMesh;
};