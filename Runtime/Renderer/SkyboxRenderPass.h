#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShadowMapRenderer

class RENDERER_API FSkyboxRenderPass final
{
public:
    FSkyboxRenderPass() = default;
    ~FSkyboxRenderPass() = default;

    bool Init(FFrameResources& FrameResources);

    void Render(FRHICommandList& CmdList, const FFrameResources& FrameResources, const FScene& Scene);

    void Release();

private:
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          SkyboxVertexShader;
    FRHIPixelShaderRef           SkyboxPixelShader;
    TSharedRef<FRHIVertexBuffer> SkyboxVertexBuffer;
    TSharedRef<FRHIIndexBuffer>  SkyboxIndexBuffer;
    FRHISamplerStateRef          SkyboxSampler;

    FMeshData                    SkyboxMesh;
};