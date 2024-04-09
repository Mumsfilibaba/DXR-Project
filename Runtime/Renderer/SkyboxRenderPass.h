#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/World/World.h"
#include "RendererUtilities/GPUTextureCompressor.h"

class FSkyboxRenderPass : public FRenderPass
{
public:
    FSkyboxRenderPass(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
        , SkyboxIndexCount(0)
        , SkyboxIndexFormat(EIndexFormat::Unknown)
    {
    }

    bool Initialize(FFrameResources& FrameResources);
    void Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

    void Release();

private:
    FGPUTextureCompressor        TextureCompressor;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          SkyboxVertexShader;
    FRHIPixelShaderRef           SkyboxPixelShader;
    FRHIBufferRef                SkyboxVertexBuffer;
    FRHIBufferRef                SkyboxIndexBuffer;
    uint32                       SkyboxIndexCount;
    EIndexFormat                 SkyboxIndexFormat;
    FRHISamplerStateRef          SkyboxSampler;
};
