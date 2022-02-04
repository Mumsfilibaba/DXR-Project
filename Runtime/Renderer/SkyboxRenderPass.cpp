#include "SkyboxRenderPass.h"

#include "Core/Debug/Debug.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShadowMapRenderer

bool CSkyboxRenderPass::Init(SFrameResources& FrameResources)
{
    SkyboxMesh = CMeshFactory::CreateSphere(1);

    SRHIResourceData VertexData = SRHIResourceData(SkyboxMesh.Vertices.Data(), SkyboxMesh.Vertices.SizeInBytes());
    SkyboxVertexBuffer = RHICreateVertexBuffer<SVertex>(SkyboxMesh.Vertices.Size(), BufferFlag_Dynamic, ERHIResourceState::VertexAndConstantBuffer, &VertexData);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    SRHIResourceData IndexData = SRHIResourceData(SkyboxMesh.Indices.Data(), SkyboxMesh.Indices.SizeInBytes());
    SkyboxIndexBuffer = RHICreateIndexBuffer(ERHIIndexFormat::uint32, SkyboxMesh.Indices.Size(), BufferFlag_Dynamic, ERHIResourceState::VertexAndConstantBuffer, &IndexData);
    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetName("Skybox IndexBuffer");
    }

    // Create Texture Cube
    const CString PanoramaSourceFilename = ENGINE_LOCATION"/Assets/Textures/arches.hdr";
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

    SRHISamplerStateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Wrap;
    CreateInfo.AddressV = ESamplerMode::Wrap;
    CreateInfo.AddressW = ESamplerMode::Wrap;
    CreateInfo.Filter = ESamplerFilter::MinMagMipLinear;
    CreateInfo.MinLOD = 0.0f;
    CreateInfo.MaxLOD = 0.0f;

    SkyboxSampler = RHICreateSamplerState(CreateInfo);
    if (!SkyboxSampler)
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Skybox.hlsl", "VSMain", nullptr, ERHIShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        SkyboxVertexShader->SetName("Skybox VertexShader");
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Skybox.hlsl", "PSMain", nullptr, ERHIShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
    else
    {
        SkyboxPixelShader->SetName("Skybox PixelShader");
    }

    SRHIRasterizerStateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Skybox RasterizerState");
    }

    SRHIBlendStateInfo BlendStateInfo;
    BlendStateInfo.bIndependentBlendEnable = false;
    BlendStateInfo.RenderTarget[0].bBlendEnable = false;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Skybox BlendState");
    }

    SRHIDepthStencilStateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.bDepthEnable = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Skybox DepthStencilState");
    }

    SRHIGraphicsPipelineStateInfo PipelineStateInfo;
    PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.BlendState = BlendState.Get();
    PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
    PipelineStateInfo.RasterizerState = RasterizerState.Get();
    PipelineStateInfo.ShaderState.VertexShader = SkyboxVertexShader.Get();
    PipelineStateInfo.ShaderState.PixelShader = SkyboxPixelShader.Get();
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PipelineStateInfo.PipelineFormats.NumRenderTargets = 1;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

    PipelineState = RHICreateGraphicsPipelineState(PipelineStateInfo);
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

    const float RenderWidth = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    CRHIRenderTargetView* RenderTarget[] = { FrameResources.FinalTarget->GetRenderTargetView() };
    CmdList.SetRenderTargets(RenderTarget, 1, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetVertexBuffers(&SkyboxVertexBuffer, 1, 0);
    CmdList.SetIndexBuffer(SkyboxIndexBuffer.Get());
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    struct SimpleCameraBuffer
    {
        CMatrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

    CmdList.Set32BitShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, 16);

    CRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CmdList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CmdList.DrawIndexedInstanced(static_cast<uint32>(SkyboxMesh.Indices.Size()), 1, 0, 0, 0);

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
