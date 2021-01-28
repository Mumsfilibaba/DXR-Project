#include "SkyboxRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"
#include "Rendering/TextureFactory.h"

#include "Debug/Profiler.h"

Bool SkyboxRenderPass::Init(FrameResources& FrameResources)
{
    SkyboxMesh = MeshFactory::CreateSphere(1);

    ResourceData VertexData = ResourceData(SkyboxMesh.Vertices.Data());
    SkyboxVertexBuffer = RenderLayer::CreateVertexBuffer<Vertex>(
        &VertexData,
        SkyboxMesh.Vertices.Size(),
        BufferUsage_Dynamic);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    ResourceData IndexData = ResourceData(SkyboxMesh.Indices.Data());
    SkyboxIndexBuffer = RenderLayer::CreateIndexBuffer(
        &IndexData,
        SkyboxMesh.Indices.SizeInBytes(),
        EIndexFormat::IndexFormat_UInt32,
        BufferUsage_Dynamic);
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
    SampledTexture2D Panorama = TextureFactory::LoadSampledTextureFromFile(
        PanoramaSourceFilename,
        0,
        EFormat::Format_R32G32B32A32_Float);
    if (!Panorama)
    {
        return false;
    }
    else
    {
        Panorama.SetName(PanoramaSourceFilename);
    }

    FrameResources.Skybox = TextureFactory::CreateTextureCubeFromPanorma(
        Panorama,
        1024,
        TextureFactoryFlag_GenerateMips,
        EFormat::Format_R16G16B16A16_Float);
    if (!FrameResources.Skybox)
    {
        return false;
    }
    else
    {
        FrameResources.Skybox->SetName("Skybox");
    }

    FrameResources.SkyboxSRV = RenderLayer::CreateShaderResourceView(
        FrameResources.Skybox.Get(),
        EFormat::Format_R16G16B16A16_Float,
        0,
        FrameResources.Skybox->GetMipLevels());
    if (!FrameResources.SkyboxSRV)
    {
        return false;
    }
    else
    {
        FrameResources.SkyboxSRV->SetName("Skybox SRV");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressV = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressW = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipLinear;
    CreateInfo.MinLOD   = 0.0f;
    CreateInfo.MaxLOD   = 0.0f;

    SkyboxSampler = RenderLayer::CreateSamplerState(CreateInfo);
    if (!SkyboxSampler)
    {
        return false;
    }

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Skybox.hlsl",
        "VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
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

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Skybox.hlsl",
        "PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
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
    RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

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
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
    DepthStencilStateInfo.DepthEnable    = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

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

void SkyboxRenderPass::Render(
    CommandList& CmdList,
    const FrameResources& FrameResources,
    const Scene& Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

    {
        TRACE_SCOPE("Render Skybox");

        const Float RenderWidth  = Float(FrameResources.FinalTarget->GetWidth());
        const Float RenderHeight = Float(FrameResources.FinalTarget->GetHeight());

        RenderTargetView* RenderTarget[] = { FrameResources.FinalTargetRTV.Get() };
        CmdList.BindRenderTargets(RenderTarget, 1, nullptr);

        CmdList.BindViewport(
            RenderWidth,
            RenderHeight,
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            RenderWidth,
            RenderHeight,
            0, 0);

        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_DepthWrite);

        CmdList.TransitionTexture(
            FrameResources.FinalTarget.Get(),
            EResourceState::ResourceState_UnorderedAccess,
            EResourceState::ResourceState_RenderTarget);

        CmdList.BindRenderTargets(RenderTarget, 1, FrameResources.GBufferDSV.Get());

        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
        CmdList.BindVertexBuffers(&SkyboxVertexBuffer, 1, 0);
        CmdList.BindIndexBuffer(SkyboxIndexBuffer.Get());
        CmdList.BindGraphicsPipelineState(PipelineState.Get());

        struct SimpleCameraBuffer
        {
            XMFLOAT4X4 Matrix;
        } SimpleCamera;
        SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Vertex,
            &SimpleCamera, 16);

        CmdList.BindShaderResourceViews(
            EShaderStage::ShaderStage_Pixel,
            &FrameResources.SkyboxSRV,
            1, 0);

        CmdList.BindSamplerStates(
            EShaderStage::ShaderStage_Pixel,
            &SkyboxSampler,
            1, 0);

        CmdList.DrawIndexedInstanced(
            static_cast<UInt32>(SkyboxMesh.Indices.Size()),
            1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");
}

void SkyboxRenderPass::Release()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
}
