#include "ForwardRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Resources/Material.h"

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

    VShader = CreateVertexShader(ShaderCode);
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

    PShader = CreatePixelShader(ShaderCode);
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

    TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
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

    TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
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

    TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
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
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    PipelineState = CreateGraphicsPipelineState(PSOProperties);
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
    VShader.Reset();
    PShader.Reset();
}

void ForwardRenderer::Render(CommandList& CmdList, const FrameResources& FrameResources, const LightSetup& LightSetup)
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin ForwardPass");

    TRACE_SCOPE("ForwardPass");

    const Float RenderWidth  = Float(FrameResources.FinalTarget->GetWidth());
    const Float RenderHeight = Float(FrameResources.FinalTarget->GetHeight());

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    RenderTargetView* FinalTargetRTV = FrameResources.FinalTarget->GetRenderTargetView();
    CmdList.SetRenderTargets(&FinalTargetRTV, 1, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    CmdList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix pointlight count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 1);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 3);

    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.IrradianceMap->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.DirLightShadowMaps->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.PointLightShadowMaps->GetShaderResourceView(), 4);

    CmdList.SetSamplerState(PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1);
    CmdList.SetSamplerState(PShader.Get(), FrameResources.IrradianceSampler.Get(), 2);
    CmdList.SetSamplerState(PShader.Get(), FrameResources.PointShadowSampler.Get(), 3);
    CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalShadowSampler.Get(), 4);

    struct TransformBuffer
    {
        XMFLOAT4X4 Transform;
        XMFLOAT4X4 TransformInv;
    } TransformPerObject;

    CmdList.SetGraphicsPipelineState(PipelineState.Get());
    for (const MeshDrawCommand& Command : FrameResources.ForwardVisibleCommands)
    {
        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
        CmdList.SetIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        ConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer(PShader.Get(), ConstantBuffer, 4);

        ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[0], 5);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[1], 6);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[2], 7);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[3], 8);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[4], 9);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[5], 10);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[6], 11);

        SamplerState* SamplerState = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState(PShader.Get(), SamplerState, 0);

        TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CmdList.Set32BitShaderConstants(VShader.Get(), &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End ForwardPass");
}
