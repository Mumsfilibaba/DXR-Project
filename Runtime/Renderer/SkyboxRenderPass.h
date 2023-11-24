#pragma once
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Scene/Scene.h"
#include "RendererUtilities/GPUTextureCompressor.h"

class RENDERER_API FSkyboxRenderPass final
{
public:
    bool Initialize(FFrameResources& FrameResources);

    void Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FScene& Scene);

    void Release();

private:
    FGPUTextureCompressor        TextureCompressor;

    FMeshData                    SkyboxMesh;
    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          SkyboxVertexShader;
    FRHIPixelShaderRef           SkyboxPixelShader;
    FRHIBufferRef                SkyboxVertexBuffer;
    FRHIBufferRef                SkyboxIndexBuffer;
    uint32                       SkyboxIndexCount  = 0;
    EIndexFormat                 SkyboxIndexFormat = EIndexFormat::Unknown;
    FRHISamplerStateRef          SkyboxSampler;
};