#include "DeferredSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"
#include "RenderLayer/Viewport.h"

Bool DeferredSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RenderTarget;

    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.AlbedoFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("FrameResources.GBuffer Albedo");

        FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
            FrameResources.AlbedoFormat, 0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.AlbedoFormat, 0);
        if (!FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Normal
    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.NormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName("FrameResources.GBuffer Normal");

        FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
            FrameResources.NormalFormat, 
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
            FrameResources.NormalFormat, 
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Material Properties
    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.MaterialFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("FrameResources.GBuffer Material");

        FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
            FrameResources.MaterialFormat, 
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
            FrameResources.MaterialFormat, 
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // DepthStencil
    const UInt32 UsageDS = TextureUsage_Default | TextureUsage_DSV | TextureUsage_SRV;
    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R32_Typeless,
        UsageDS,
        Width,
        Height,
        1, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.GBuffer[GBUFFER_DEPTH_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName("FrameResources.GBuffer DepthStencil");

        FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            EFormat::Format_R32_Float,
            0,
            1);
        if (!FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX])
        {
            return false;
        }

        FrameResources.GBufferDSV = RenderLayer::CreateDepthStencilView(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            FrameResources.DepthBufferFormat,
            0);
        if (!FrameResources.GBufferDSV)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // View Normal
    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.ViewNormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName("FrameResources.GBuffer View Normal");

        FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), 
            FrameResources.ViewNormalFormat, 
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), 
            FrameResources.ViewNormalFormat, 
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Final Image
    FrameResources.FinalTarget = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.FinalTargetFormat,
        TextureUsage_Default | TextureUsage_RenderTarget | TextureUsage_UAV,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetName("Final Target");

        FrameResources.FinalTargetSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0, 1);
        if (!FrameResources.FinalTargetSRV)
        {
            return false;
        }

        FrameResources.FinalTargetRTV = RenderLayer::CreateRenderTargetView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0);
        if (!FrameResources.FinalTargetRTV)
        {
            return false;
        }

        FrameResources.FinalTargetUAV = RenderLayer::CreateUnorderedAccessView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0);
        if (!FrameResources.FinalTargetUAV)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressV = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressW = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipPoint;

        FrameResources.GBufferSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!FrameResources.GBufferSampler)
        {
            return false;
        }
    }

    TArray<ShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/GeometryPass.hlsl",
        "VSMain",
        &Defines,
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
        VShader->SetName("GeometryPass VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/GeometryPass.hlsl",
        "PSMain",
        &Defines,
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
        PShader->SetName("GeometryPass PixelShader");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
    DepthStencilStateInfo.DepthEnable    = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

    TSharedRef<DepthStencilState> GeometryDepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
    if (!GeometryDepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        GeometryDepthStencilState->SetName("GeometryPass DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

    TSharedRef<RasterizerState> GeometryRasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!GeometryRasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        GeometryRasterizerState->SetName("GeometryPass RasterizerState");
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
        BlendState->SetName("GeometryPass BlendState");
    }

    GraphicsPipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.InputLayoutState          = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.BlendState                = BlendState.Get();
    PipelineStateInfo.DepthStencilState         = GeometryDepthStencilState.Get();
    PipelineStateInfo.RasterizerState           = GeometryRasterizerState.Get();
    PipelineStateInfo.ShaderState.VertexShader  = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader   = PShader.Get();
    PipelineStateInfo.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::Format_R8G8B8A8_Unorm;
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::Format_R8G8B8A8_Unorm;
    PipelineStateInfo.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
    PipelineStateInfo.PipelineFormats.NumRenderTargets = 4;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("GeometryPass PipelineState");
    }

    return true;
}

void DeferredSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResources)
{
}
