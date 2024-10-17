#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
#include "Renderer/RendererUtilities/GPUTextureCompressor.h"

class FSkyboxRenderPass : public FRenderPass
{
public:
    FSkyboxRenderPass(FSceneRenderer* InRenderer);
    virtual ~FSkyboxRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

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
