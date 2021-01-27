#include "PointLightShadowSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

static const EFormat ShadowMapFormat = EFormat::Format_D32_Float;
static const UInt16  ShadowMapSize   = 1024;

struct PointLightPerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool PointLightShadowSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    FrameResources.PointLightBuffer = RenderLayer::CreateConstantBuffer<PointLightProperties>(
        nullptr,
        FrameResources.MaxPointLights,
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!FrameResources.PointLightBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.PointLightBuffer->SetName("PointLight Buffer");
    }

    PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<PointLightPerShadowMap>(
        nullptr, 1,
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!PerShadowMapBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetName("PerShadowMap Buffer");
    }

    FrameResources.PointLightShadowMaps = RenderLayer::CreateTextureCubeArray(
        nullptr,
        ShadowMapFormat,
        TextureUsage_ShadowMap,
        ShadowMapSize,
        1, FrameResources.MaxPointLightShadows, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.PointLightShadowMaps)
    {
        FrameResources.PointLightShadowMaps->SetName("PointLight ShadowMaps");

        FrameResources.PointLightShadowMapDSVs.Resize(FrameResources.MaxPointLightShadows);
        for (UInt32 i = 0; i < FrameResources.MaxPointLightShadows; i++)
        {
            for (UInt32 Face = 0; Face < 6; Face++)
            {
                TStaticArray<TSharedRef<DepthStencilView>, 6>& DepthCube = FrameResources.PointLightShadowMapDSVs[i];
                DepthCube[Face] = RenderLayer::CreateDepthStencilView(
                    FrameResources.PointLightShadowMaps.Get(),
                    ShadowMapFormat,
                    0, i, Face);
                if (!DepthCube[Face])
                {
                    Debug::DebugBreak();
                    return false;
                }
            }
        }

        FrameResources.PointLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.PointLightShadowMaps.Get(),
            EFormat::Format_R32_Float,
            0, 1, 0, FrameResources.MaxPointLightShadows);
        if (!FrameResources.PointLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
    }
    else
    {
        return false;
    }

    // Linear Shadow Maps
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
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
        VShader->SetName("Linear ShadowMap VertexShader");
    }

#if !ENABLE_VSM
    TSharedRef<PixelShader> PShader;
#endif
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("Linear ShadowMap PixelShader");
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
    PipelineStateInfo.BlendState            = BlendState.Get();
    PipelineStateInfo.DepthStencilState     = DepthStencilState.Get();
    PipelineStateInfo.IBStripCutValue       = EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
    PipelineStateInfo.InputLayoutState      = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.PrimitiveTopologyType = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PipelineStateInfo.RasterizerState       = RasterizerState.Get();
    PipelineStateInfo.SampleCount           = 1;
    PipelineStateInfo.SampleQuality         = 0;
    PipelineStateInfo.SampleMask            = 0xffffffff;
    PipelineStateInfo.ShaderState.VertexShader = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader  = PShader.Get();
    PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat = ShadowMapFormat;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("Linear ShadowMap PipelineState");
    }

    return true;
}

void PointLightShadowSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResources)
{
}
