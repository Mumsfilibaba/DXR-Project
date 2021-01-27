#include "DirectionalLightShadowSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

static const EFormat ShadowMapFormat = EFormat::Format_D32_Float;

struct DirectionalLightPerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool DirectionalLightShadowSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    FrameResources.DirectionalLightBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightProperties>(
        nullptr,
        FrameResources.MaxDirectionalLights,
        BufferUsage_Default,
        EResourceState::ResourceState_VertexAndConstantBuffer);
    if (!FrameResources.DirectionalLightBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.DirectionalLightBuffer->SetName("DirectionalLight Buffer");
    }

    PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightPerShadowMap>(
        nullptr, 1,
        BufferUsage_Default,
        EResourceState::ResourceState_VertexAndConstantBuffer);
    if (!PerShadowMapBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetName("PerShadowMap Buffer");
    }

    FrameResources.DirLightShadowMaps = RenderLayer::CreateTexture2D(
        nullptr,
        ShadowMapFormat,
        TextureUsage_ShadowMap,
        4096, 4096,
        1, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.DirLightShadowMaps)
    {
        FrameResources.DirLightShadowMaps->SetName("Directional Light ShadowMaps");

        FrameResources.DirLightShadowMapDSV = RenderLayer::CreateDepthStencilView(
            FrameResources.DirLightShadowMaps.Get(),
            ShadowMapFormat,
            0);
        if (!FrameResources.DirLightShadowMapDSV)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("DirectionalLight DepthStencilView");
        }

#if !ENABLE_VSM
        FrameResources.DirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.DirLightShadowMaps.Get(),
            EFormat::Format_R32_Float,
            0, 1);
        if (!FrameResources.DirLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("DirectionalLight ShaderResourceView");
        }
#endif
    }
    else
    {
        return false;
    }

#if ENABLE_VSM
    VSMDirLightShadowMaps = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R32G32_Float,
        TextureUsage_RenderTarget,
        Renderer::GetGlobalLightSettings().ShadowMapWidth,
        Renderer::GetGlobalLightSettings().ShadowMapHeight,
        1, 1,
        ClearValue(ColorClearValue(1.0f, 1.0f, 1.0f, 1.0f)));
    if (VSMDirLightShadowMaps)
    {
        VSMDirLightShadowMaps->SetName("Directional Light VSM");

        VSMDirLightShadowMapRTV = RenderLayer::CreateRenderTargetView(
            VSMDirLightShadowMaps.Get(),
            EFormat::Format_R32G32_Float,
            0);
        if (!VSMDirLightShadowMapRTV)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("VSM DirectionalLight DepthStencilView");
        }

        VSMDirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            VSMDirLightShadowMaps.Get(),
            EFormat::Format_R32G32_Float,
            0, 1);
        if (!VSMDirLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("VSM DirectionalLight ShaderResourceView");
        }
    }
    else
    {
        return false;
    }
#endif

    TArray<UInt8> ShaderCode;
#if ENABLE_VSM
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "VSM_VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VSShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VSShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VSShader->SetName("ShadowMap VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "VSM_PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PSShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PSShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PSShader->SetName("ShadowMap PixelShader");
    }
#else
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "Main",
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
        VShader->SetName("ShadowMap VertexShader");
    }
#endif

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
        DepthStencilState->SetName("Shadow DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Shadow RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Shadow BlendState");
    }

    GraphicsPipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.BlendState               = BlendState.Get();
    PipelineStateInfo.DepthStencilState        = DepthStencilState.Get();
    PipelineStateInfo.IBStripCutValue          = EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
    PipelineStateInfo.InputLayoutState         = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.PrimitiveTopologyType    = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PipelineStateInfo.RasterizerState          = RasterizerState.Get();
    PipelineStateInfo.SampleCount              = 1;
    PipelineStateInfo.SampleQuality            = 0;
    PipelineStateInfo.SampleMask               = 0xffffffff;
    PipelineStateInfo.ShaderState.VertexShader = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader  = nullptr;
    PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat = ShadowMapFormat;

#if ENABLE_VSM
    VSMShadowMapPSO = MakeShared<D3D12GraphicsPipelineState>(Device.Get());
    if (!VSMShadowMapPSO->Initialize(PSOProperties))
    {
        return false;
    }
#else
    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("ShadowMap PipelineState");
    }
#endif

    return true;
}

void DirectionalLightShadowSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResources)
{
}
