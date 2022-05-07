#include "SkyboxRenderPass.h"

#include "Core/Debug/Debug.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShadowMapRenderer

bool CSkyboxRenderPass::Init(SFrameResources& FrameResources)
{
    SkyboxMesh = CMeshFactory::CreateSphere(1);

    CRHIBufferDataInitializer VertexData(SkyboxMesh.Vertices.Data(), SkyboxMesh.Vertices.SizeInBytes());

    CRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, SkyboxMesh.Vertices.Size(), sizeof(SVertex), EResourceAccess::VertexAndConstantBuffer, &VertexData);
    SkyboxVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    CRHIBufferDataInitializer IndexData(SkyboxMesh.Indices.Data(), SkyboxMesh.Indices.SizeInBytes());

    CRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint32, SkyboxMesh.Indices.Size(), EResourceAccess::IndexBuffer, &IndexData);
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
    const String PanoramaSourceFilename = ENGINE_LOCATION"/Assets/Textures/arches.hdr";
    TSharedRef<CRHITexture2D> Panorama = CTextureFactory::LoadFromFile(PanoramaSourceFilename, 0, EFormat::R32G32B32A32_Float);
    if (!Panorama)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Panorama->SetName(PanoramaSourceFilename);
    }

    FrameResources.Skybox = CTextureFactory::CreateTextureCubeFromPanorma(Panorama.Get(), 1024, TextureFactoryFlag_GenerateMips, EFormat::R16G16B16A16_Float);
    if (!FrameResources.Skybox)
    {
        return false;
    }
    else
    {
        FrameResources.Skybox->SetName("Skybox");
    }

    CRHISamplerStateInitializer Initializer;
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
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Skybox.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    SkyboxVertexShader = RHICreateVertexShader(ShaderCode);
    if (!SkyboxVertexShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Skybox.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    SkyboxPixelShader = RHICreatePixelShader(ShaderCode);
    if (!SkyboxPixelShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIBlendStateInitializer BlendStateInitializer;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIDepthStencilStateInitializer DepthStencilStateInitializer;
    DepthStencilStateInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilStateInitializer.bDepthEnable   = true;
    DepthStencilStateInitializer.DepthWriteMask = EDepthWriteMask::All;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIGraphicsPipelineStateInitializer PipelineStateInitializer;
    PipelineStateInitializer.VertexInputLayout                      = FrameResources.StdInputLayout.Get();
    PipelineStateInitializer.BlendState                             = BlendState.Get();
    PipelineStateInitializer.DepthStencilState                      = DepthStencilState.Get();
    PipelineStateInitializer.RasterizerState                        = RasterizerState.Get();
    PipelineStateInitializer.ShaderState.VertexShader               = SkyboxVertexShader.Get();
    PipelineStateInitializer.ShaderState.PixelShader                = SkyboxPixelShader.Get();
    PipelineStateInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PipelineStateInitializer.PipelineFormats.NumRenderTargets       = 1;
    PipelineStateInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

    PipelineState = RHICreateGraphicsPipelineState(PipelineStateInitializer);
    if (!PipelineState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("SkyboxPSO PipelineState");
    }

    return true;
}

void CSkyboxRenderPass::Render(CRHICommandList& CmdList, const SFrameResources& FrameResources, const CScene& Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

    GPU_TRACE_SCOPE(CmdList, "Skybox");

    TRACE_SCOPE("Render Skybox");

    const float RenderWidth  = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    CRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0] = CRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = CRHIDepthStencilView(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EAttachmentLoadAction::Load);

    CmdList.BeginRenderPass(RenderPass);

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetVertexBuffers(&SkyboxVertexBuffer, 1, 0);
    CmdList.SetIndexBuffer(SkyboxIndexBuffer.Get());
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    struct SSimpleCameraBuffer
    {
        CMatrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

    CmdList.Set32BitShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, 16);

    CRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CmdList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CmdList.DrawIndexedInstanced(static_cast<uint32>(SkyboxMesh.Indices.Size()), 1, 0, 0, 0);

    CmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");
}

void CSkyboxRenderPass::Release()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
    SkyboxVertexShader.Reset();
    SkyboxPixelShader.Reset();
}
