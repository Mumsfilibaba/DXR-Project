#include "ForwardRenderer.h"
#include "MeshDrawCommand.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/Scene/Actor.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FForwardRenderer

bool FForwardRenderer::Init(FFrameResources& FrameResources)
{
    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<uint8> ShaderCode;
    
    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex, Defines.CreateView());
    if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines.CreateView());
    if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    PShader = RHICreatePixelShader(ShaderCode);
    if (!PShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilInitializer.bDepthEnable   = true;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::All;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer( { FRenderTargetBlendDesc(true, EBlendType::One, EBlendType::Zero) }, false , false);

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.ShaderState.VertexShader               = VShader.Get();
    PSOInitializer.ShaderState.PixelShader                = PShader.Get();
    PSOInitializer.VertexInputLayout                      = FrameResources.MeshInputLayout.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
    PSOInitializer.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FForwardRenderer::Release()
{
    PipelineState.Reset();
    VShader.Reset();
    PShader.Reset();
}

void FForwardRenderer::Render(FRHICommandList& CmdList, const FFrameResources& FrameResources, const FLightSetup& LightSetup)
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin ForwardPass");

    TRACE_SCOPE("ForwardPass");

    CmdList.TransitionTexture(
        LightSetup.ShadowMapCascades[0].Get(), 
        EResourceAccess::NonPixelShaderResource, 
        EResourceAccess::PixelShaderResource);

    const float RenderWidth  = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    FRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EAttachmentLoadAction::Load);
    CmdList.BeginRenderPass(RenderPass);

    CmdList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix point-light count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 1);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(PShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 3);

    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.IrradianceMap->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
    //TODO: Fix directional-light shadows
    //CmdList.SetShaderResourceView(PShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView(PShader.Get(), LightSetup.PointLightShadowMaps->GetShaderResourceView(), 3);

    CmdList.SetSamplerState(PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1);
    CmdList.SetSamplerState(PShader.Get(), FrameResources.IrradianceSampler.Get(), 2);
    CmdList.SetSamplerState(PShader.Get(), FrameResources.PointLightShadowSampler.Get(), 3);
    //CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 4);

    struct STransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;

    CmdList.SetGraphicsPipelineState(PipelineState.Get());
    for (const auto CommandIndex : FrameResources.ForwardVisibleCommands)
    {
        const FMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];

        CmdList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
        CmdList.SetIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        FRHIConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer(PShader.Get(), ConstantBuffer, 4);

        FRHIShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[0], 4);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[1], 5);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[2], 6);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[3], 7);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[4], 8);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[5], 9);
        CmdList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[6], 10);

        FRHISamplerState* SamplerState = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState(PShader.Get(), SamplerState, 0);

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CmdList.Set32BitShaderConstants(VShader.Get(), &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
    }

    CmdList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

    CmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End ForwardPass");
}
