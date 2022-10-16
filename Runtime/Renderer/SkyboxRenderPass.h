#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Scene/Scene.h"

class RENDERER_API FSkyboxRenderPass final
{
public:
    FSkyboxRenderPass()  = default;
    ~FSkyboxRenderPass() = default;

    bool Init(FFrameResources& FrameResources);

    void Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FScene& Scene);

    void Release();

private:
    FMeshData                    SkyboxMesh;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          SkyboxVertexShader;
    FRHIPixelShaderRef           SkyboxPixelShader;
    FRHIBufferRef                SkyboxVertexBuffer;
    FRHIBufferRef                SkyboxIndexBuffer;
    uint32                       SkyboxIndexCount;
    EIndexFormat                 SkyboxIndexFormat;
    FRHISamplerStateRef          SkyboxSampler;
};