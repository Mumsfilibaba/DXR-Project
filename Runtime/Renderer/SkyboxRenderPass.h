#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Scene/Scene.h"

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
    TSharedRef<CRHIVertexBuffer> SkyboxVertexBuffer;
    TSharedRef<CRHIIndexBuffer>  SkyboxIndexBuffer;
    TSharedRef<CRHISamplerState> SkyboxSampler;

    SMeshData SkyboxMesh;
};