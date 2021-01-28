#include "ForwardSceneRenderPass.h"

#include "RenderLayer/ShaderCompiler.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/Viewport.h"

Bool ForwardSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    TArray<ShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ForwardPass.hlsl",
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
        VShader->SetName("ForwardPass VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ForwardPass.hlsl",
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
        PShader->SetName("ForwardPass PixelShader");
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
        DepthStencilState->SetName("ForwardPass DepthStencilState");
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
        RasterizerState->SetName("ForwardPass RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = true;

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("ForwardPass BlendState");
    }

    GraphicsPipelineStateCreateInfo PSOProperties = { };
    PSOProperties.ShaderState.VertexShader  = VShader.Get();
    PSOProperties.ShaderState.PixelShader   = PShader.Get();
    PSOProperties.InputLayoutState          = FrameResources.StdInputLayout.Get();
    PSOProperties.DepthStencilState         = DepthStencilState.Get();
    PSOProperties.BlendState                = BlendState.Get();
    PSOProperties.RasterizerState           = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = FrameResources.MainWindowViewport->GetColorFormat();
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
    PSOProperties.PrimitiveTopologyType = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("Forward PipelineState");
    }

    return true;
}

void ForwardSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
}
