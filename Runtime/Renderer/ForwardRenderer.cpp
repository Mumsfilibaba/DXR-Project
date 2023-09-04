#include "ForwardRenderer.h"
#include "MeshDrawCommand.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/Scene/Actors/Actor.h"
#include "Core/Misc/FrameProfiler.h"

bool FForwardRenderer::Init(FFrameResources& FrameResources)
{
    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<uint8> ShaderCode;
    
    FRHIShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex, Defines);
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

    CompileInfo = FRHIShaderCompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines);
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
    DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilInitializer.bDepthEnable      = true;
    DepthStencilInitializer.bDepthWriteEnable = true;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInitializer;
    RasterizerStateInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;
    BlendStateInitializer.RenderTargets[0] = FRenderTargetBlendDesc(true, EBlendType::One, EBlendType::Zero);

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
    PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;

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

void FForwardRenderer::Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup)
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin ForwardPass");

    TRACE_SCOPE("ForwardPass");

    CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    const float RenderWidth  = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());

    FRHIViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FRHIScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix point-light count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 3);

    const FProxyLightProbe& Skylight = LightSetup.Skylight;
    CommandList.SetShaderResourceView(PShader.Get(), Skylight.IrradianceMap->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(PShader.Get(), Skylight.SpecularIrradianceMap->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
    //TODO: Fix directional-light shadows
    //CmdList.SetShaderResourceView(PShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PShader.Get(), LightSetup.PointLightShadowMaps->GetShaderResourceView(), 3);

    CommandList.SetSamplerState(PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.IrradianceSampler.Get(), 2);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.PointLightShadowSampler.Get(), 3);
    //CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 4);

    struct STransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;

    CommandList.SetGraphicsPipelineState(PipelineState.Get());
    for (const auto CommandIndex : FrameResources.ForwardVisibleCommands)
    {
        const FMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];

        CommandList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
        CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CommandList);
        }

        FRHIBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
        CommandList.SetConstantBuffer(PShader.Get(), ConstantBuffer, 4);

        FRHIShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[0], 4);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[1], 5);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[2], 6);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[3], 7);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[4], 8);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[5], 9);
        CommandList.SetShaderResourceView(PShader.Get(), ShaderResourceViews[6], 10);

        FRHISamplerState* SamplerState = Command.Material->GetMaterialSampler();
        CommandList.SetSamplerState(PShader.Get(), SamplerState, 0);

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CommandList.Set32BitShaderConstants(VShader.Get(), &TransformPerObject, 32);

        CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
    }

    CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End ForwardPass");
}
