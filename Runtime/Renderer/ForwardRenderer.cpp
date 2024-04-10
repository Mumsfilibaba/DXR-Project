#include "ForwardRenderer.h"
#include "Scene.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/ProxySceneComponent.h"

bool FForwardRenderer::Initialize(FFrameResources& FrameResources)
{
    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<uint8> ShaderCode;
    
    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
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

    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
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

void FForwardRenderer::Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup, FScene* Scene)
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin ForwardPass");

    TRACE_SCOPE("ForwardPass");

    CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    const float RenderWidth  = float(FrameResources.CurrentWidth);
    const float RenderHeight = float(FrameResources.CurrentHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix point-light count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(PShader.Get(), LightSetup.DirectionalLightDataBuffer.Get(), 5);

    const FProxyLightProbe& Skylight = LightSetup.Skylight;
    CommandList.SetShaderResourceView(PShader.Get(), Skylight.IrradianceMap->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(PShader.Get(), Skylight.SpecularIrradianceMap->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
    //TODO: Fix directional-light shadows
    //CmdList.SetShaderResourceView(PShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PShader.Get(), LightSetup.PointLightShadowMaps->GetShaderResourceView(), 4);

    CommandList.SetSamplerState(PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.IrradianceSampler.Get(), 2);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.PointLightShadowSampler.Get(), 3);
    //CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 4);

    struct STransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;


    for (const FProxySceneComponent* Component : Scene->VisiblePrimitives)
    {
        if (!Component->Material->ShouldRenderInForwardPass())
        {
            continue;
        }

        CommandList.SetVertexBuffers(MakeArrayView(&Component->VertexBuffer, 1), 0);
        CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

        FRHIBuffer* ConstantBuffer = Component->Material->GetMaterialBuffer();
        CommandList.SetConstantBuffer(PShader.Get(), ConstantBuffer, 6);

        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->AlbedoMap->GetShaderResourceView(), 5);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->NormalMap->GetShaderResourceView(), 6);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->RoughnessMap->GetShaderResourceView(), 7);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->MetallicMap->GetShaderResourceView(), 8);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->AOMap->GetShaderResourceView(), 9);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->AlphaMask->GetShaderResourceView(), 10);
        CommandList.SetShaderResourceView(PShader.Get(), Component->Material->HeightMap->GetShaderResourceView(), 11);

        FRHISamplerState* SamplerState = Component->Material->GetMaterialSampler();
        CommandList.SetSamplerState(PShader.Get(), SamplerState, 0);

        TransformPerObject.Transform    = Component->CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Component->CurrentActor->GetTransform().GetMatrixInverse();

        CommandList.Set32BitShaderConstants(VShader.Get(), &TransformPerObject, 32);

        CommandList.DrawIndexedInstanced(Component->NumIndices, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();

    CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End ForwardPass");
}
