#include "ForwardRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"

#include "Scene/Actor.h"

#include "Debug/Profiler.h"

Bool ForwardRenderer::Init(FrameResources& FrameResources)
{
    TArray<ShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ForwardPass.hlsl", "VSMain", &Defines, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ForwardPass.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
        DepthStencilState->SetName("ForwardPass DepthStencilState");
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
        RasterizerState->SetName("ForwardPass RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable      = false;
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

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.InputLayoutState                       = FrameResources.StdInputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = FrameResources.MainWindowViewport->GetColorFormat();
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

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

void ForwardRenderer::Release()
{
    PipelineState.Reset();
}

void ForwardRenderer::Render(CommandList& CmdList, const FrameResources& FrameResources, const SceneLightSetup& LightSetup)
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin ForwardPass");

    TRACE_SCOPE("ForwardPass");

    const Float RenderWidth  = Float(FrameResources.FinalTarget->GetWidth());
    const Float RenderHeight = Float(FrameResources.FinalTarget->GetHeight());

    CmdList.BindViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.BindScissorRect(RenderWidth, RenderHeight, 0, 0);

    CmdList.BindRenderTargets(&FrameResources.BackBufferRTV, 1, FrameResources.GBufferDSV.Get());

    ConstantBuffer* ConstantBuffers[] =
    {
        FrameResources.CameraBuffer.Get(),
        LightSetup.PointLightBuffer.Get(),
        LightSetup.DirectionalLightBuffer.Get(),
    };

    CmdList.BindConstantBuffers(EShaderStage::Pixel, ConstantBuffers, 3, 0);

    {
        ShaderResourceView* ShaderResourceViews[] =
        {
            LightSetup.IrradianceMapSRV.Get(),
            LightSetup.SpecularIrradianceMapSRV.Get(),
            FrameResources.IntegrationLUTSRV.Get(),
            LightSetup.DirLightShadowMapSRV.Get(),
            LightSetup.PointLightShadowMapSRV.Get(),
        };

        CmdList.BindShaderResourceViews(EShaderStage::Pixel, ShaderResourceViews, 5, 0);
    }

    {
        SamplerState* SamplerStates[] =
        {
            FrameResources.IntegrationLUTSampler.Get(),
            FrameResources.IrradianceSampler.Get(),
            FrameResources.ShadowMapCompSampler.Get(),
            FrameResources.ShadowMapSampler.Get()
        };

        CmdList.BindSamplerStates(EShaderStage::Pixel, SamplerStates, 4, 1);
    }

    struct TransformBuffer
    {
        XMFLOAT4X4 Transform;
        XMFLOAT4X4 TransformInv;
    } TransformPerObject;

    CmdList.BindGraphicsPipelineState(PipelineState.Get());
    for (const MeshDrawCommand& Command : FrameResources.ForwardVisibleCommands)
    {
        CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
        CmdList.BindIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        ConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
        CmdList.BindConstantBuffers(EShaderStage::Pixel, &ConstantBuffer, 1, 3);

        ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.BindShaderResourceViews(EShaderStage::Pixel, ShaderResourceViews, 7, 5);

        SamplerState* SamplerState = Command.Material->GetMaterialSampler();
        CmdList.BindSamplerStates(EShaderStage::Pixel, &SamplerState, 1, 0);

        TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End ForwardPass");
}
