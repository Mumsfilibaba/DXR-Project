#include "SkyboxRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"
#include "Rendering/TextureFactory.h"

#include "Debug/Profiler.h"

Bool SkyboxRenderPass::Init(FrameResources& FrameResources)
{
    SkyboxMesh = MeshFactory::CreateSphere(1);

    ResourceData VertexData = ResourceData(SkyboxMesh.Vertices.Data(), SkyboxMesh.Vertices.SizeInBytes());
    SkyboxVertexBuffer = RenderLayer::CreateVertexBuffer<Vertex>(SkyboxMesh.Vertices.Size(), BufferFlag_Upload, EResourceState::VertexAndConstantBuffer, &VertexData);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    ResourceData IndexData = ResourceData(SkyboxMesh.Indices.Data(), SkyboxMesh.Indices.SizeInBytes());
    SkyboxIndexBuffer = RenderLayer::CreateIndexBuffer(
        EIndexFormat::UInt32, 
        SkyboxMesh.Indices.Size(), 
        BufferFlag_Upload, 
        EResourceState::VertexAndConstantBuffer, 
        &IndexData);
    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetName("Skybox IndexBuffer");
    }

    // Create Texture Cube
    const std::string PanoramaSourceFilename = "../Assets/Textures/arches.hdr";
    TSharedRef<Texture2D> Panorama = TextureFactory::LoadFromFile(PanoramaSourceFilename, 0, EFormat::R32G32B32A32_Float);
    if (!Panorama)
    {
        return false;
    }
    else
    {
        Panorama->SetName(PanoramaSourceFilename);
    }

    FrameResources.Skybox = TextureFactory::CreateTextureCubeFromPanorma(Panorama.Get(), 1024, TextureFactoryFlag_GenerateMips, EFormat::R16G16B16A16_Float);
    if (!FrameResources.Skybox)
    {
        return false;
    }
    else
    {
        FrameResources.Skybox->SetName("Skybox");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Wrap;
    CreateInfo.AddressV = ESamplerMode::Wrap;
    CreateInfo.AddressW = ESamplerMode::Wrap;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipLinear;
    CreateInfo.MinLOD   = 0.0f;
    CreateInfo.MaxLOD   = 0.0f;

    SkyboxSampler = RenderLayer::CreateSamplerState(CreateInfo);
    if (!SkyboxSampler)
    {
        return false;
    }

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Skybox.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Skybox VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Skybox.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("Skybox PixelShader");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Skybox RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable      = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Skybox BlendState");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.DepthEnable    = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Skybox DepthStencilState");
    }

    GraphicsPipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.InputLayoutState                       = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.BlendState                             = BlendState.Get();
    PipelineStateInfo.DepthStencilState                      = DepthStencilState.Get();
    PipelineStateInfo.RasterizerState                        = RasterizerState.Get();
    PipelineStateInfo.ShaderState.VertexShader               = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader                = PShader.Get();
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PipelineStateInfo.PipelineFormats.NumRenderTargets       = 1;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("SkyboxPSO PipelineState");
    }

    return true;
}

void SkyboxRenderPass::Render(CommandList& CmdList, const FrameResources& FrameResources, const Scene& Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

    TRACE_SCOPE("Render Skybox");

    const Float RenderWidth  = Float(FrameResources.FinalTarget->GetWidth());
    const Float RenderHeight = Float(FrameResources.FinalTarget->GetHeight());

    RenderTargetView* RenderTarget[] = { FrameResources.FinalTarget->GetRenderTargetView() };
    CmdList.BindRenderTargets(RenderTarget, 1, nullptr);

    CmdList.BindViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.BindScissorRect(RenderWidth, RenderHeight, 0, 0);

    CmdList.TransitionTexture(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite);
    CmdList.TransitionTexture(FrameResources.FinalTarget.Get(), EResourceState::UnorderedAccess, EResourceState::RenderTarget);

    CmdList.BindRenderTargets(RenderTarget, 1, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.BindVertexBuffers(&SkyboxVertexBuffer, 1, 0);
    CmdList.BindIndexBuffer(SkyboxIndexBuffer.Get());
    CmdList.BindGraphicsPipelineState(PipelineState.Get());

    struct SimpleCameraBuffer
    {
        XMFLOAT4X4 Matrix;
    } SimpleCamera;
    SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

    CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &SimpleCamera, 16);

    ShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.BindShaderResourceViews(EShaderStage::Pixel, &SkyboxSRV, 1, 0);

    CmdList.BindSamplerStates(EShaderStage::Pixel, &SkyboxSampler, 1, 0);

    CmdList.DrawIndexedInstanced(static_cast<UInt32>(SkyboxMesh.Indices.Size()), 1, 0, 0, 0);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");
}

void SkyboxRenderPass::Release()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
}