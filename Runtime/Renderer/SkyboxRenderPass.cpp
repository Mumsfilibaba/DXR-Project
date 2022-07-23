#include "SkyboxRenderPass.h"

#include "Core/Debug/Debug.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShadowMapRenderer

bool FSkyboxRenderPass::Init(FFrameResources& FrameResources)
{
    SkyboxMesh = FMeshFactory::CreateSphere(1);

    FRHIBufferDataInitializer VertexData(SkyboxMesh.Vertices.Data(), SkyboxMesh.Vertices.SizeInBytes());

    FRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, SkyboxMesh.Vertices.Size(), sizeof(FVertex), EResourceAccess::VertexAndConstantBuffer, &VertexData);
    SkyboxVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    FRHIBufferDataInitializer IndexData(SkyboxMesh.Indices.Data(), SkyboxMesh.Indices.SizeInBytes());

    FRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint32, SkyboxMesh.Indices.Size(), EResourceAccess::IndexBuffer, &IndexData);
    SkyboxIndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetName("Skybox IndexBuffer");
    }

    // Create Texture Cube
    const FString PanoramaSourceFilename = ENGINE_LOCATION"/Assets/Textures/arches.hdr";
    FRHITexture2DRef Panorama = FTextureFactory::LoadFromFile(PanoramaSourceFilename, 0, EFormat::R32G32B32A32_Float);
    if (!Panorama)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Panorama->SetName(PanoramaSourceFilename);
    }

    FrameResources.Skybox = FTextureFactory::CreateTextureCubeFromPanorma(Panorama.Get(), 1024, TextureFactoryFlag_GenerateMips, EFormat::R16G16B16A16_Float);
    if (!FrameResources.Skybox)
    {
        return false;
    }
    else
    {
        FrameResources.Skybox->SetName("Skybox");
    }

    FRHISamplerStateInitializer Initializer;
    Initializer.AddressU = ESamplerMode::Wrap;
    Initializer.AddressV = ESamplerMode::Wrap;
    Initializer.AddressW = ESamplerMode::Wrap;
    Initializer.Filter   = ESamplerFilter::MinMagMipLinear;
    Initializer.MinLOD   = 0.0f;
    Initializer.MaxLOD   = 0.0f;

    SkyboxSampler = RHICreateSamplerState(Initializer);
    if (!SkyboxSampler)
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    SkyboxVertexShader = RHICreateVertexShader(ShaderCode);
    if (!SkyboxVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    SkyboxPixelShader = RHICreatePixelShader(ShaderCode);
    if (!SkyboxPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
    DepthStencilStateInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilStateInitializer.bDepthEnable   = true;
    DepthStencilStateInitializer.DepthWriteMask = EDepthWriteMask::All;

    TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.VertexInputLayout                      = FrameResources.StdInputLayout.Get();
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = SkyboxVertexShader.Get();
    PSOInitializer.ShaderState.PixelShader                = SkyboxPixelShader.Get();
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

    PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetName("SkyboxPSO PipelineState");
    }

    return true;
}

void FSkyboxRenderPass::Render(FRHICommandList& CmdList, const FFrameResources& FrameResources, const FScene& Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

    GPU_TRACE_SCOPE(CmdList, "Skybox");

    TRACE_SCOPE("Render Skybox");

    const float RenderWidth  = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());


    FRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EAttachmentLoadAction::Load);

    CmdList.BeginRenderPass(RenderPass);

    // NOTE: For now, MetalRHI require a renderpass to be started for these two to be valid
    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetVertexBuffers(&SkyboxVertexBuffer, 1, 0);
    CmdList.SetIndexBuffer(SkyboxIndexBuffer.Get());
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    struct SSimpleCameraBuffer
    {
        FMatrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

    CmdList.Set32BitShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, 16);

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CmdList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CmdList.DrawIndexedInstanced(static_cast<uint32>(SkyboxMesh.Indices.Size()), 1, 0, 0, 0);

    CmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");
}

void FSkyboxRenderPass::Release()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
    SkyboxVertexShader.Reset();
    SkyboxPixelShader.Reset();
}
