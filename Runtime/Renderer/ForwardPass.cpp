#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FForwardPass::FForwardPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FForwardPass::~FForwardPass()
{
    PipelineState.Reset();
    VShader.Reset();
    PShader.Reset();
}

bool FForwardPass::Initialize(FFrameResources& FrameResources)
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

    VShader = FRHI::Get()->CreateVertexShader(ShaderCode);
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

    PShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = true;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;
    BlendStateDesc.RenderTargets[0].bBlendEnable = true;
    BlendStateDesc.RenderTargets[0].SrcBlend = EBlendType::One;
    BlendStateDesc.RenderTargets[0].DstBlend = EBlendType::Zero;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.VertexShader                                   = VShader.Get();
    PSODesc.PixelShader                                    = PShader.Get();
    PSODesc.InputLayout                                    = FrameResources.MeshInputLayout.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.BlendState                                     = BlendState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::SceneTargetFormat;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FForwardPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    // Forward Pass
    RHI_EVENT_SCOPE(CommandList, "ForwardPass");

    TRACE_SCOPE("ForwardPass");

    GPU_TRACE_SCOPE(CommandList, "Forward Pass");

    CommandList.TransitionTextureState(FrameResources.ShadowCascades.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIRenderTargetView* RenderTargetView = FrameResources.SceneTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = FrameResources.GBuffer[GBufferIndex_Depth]->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix point-light count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.DirectionalLightDataBuffer.Get(), 5);

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->SkyLight)
        {
            CommandList.SetShaderResourceView(PShader.Get(), SkyLight->DiffuseCubeMap->GetShaderResourceView(), 0);
            CommandList.SetShaderResourceView(PShader.Get(), SkyLight->SpecularCubeMap->GetShaderResourceView(), 1);
        }
    }

    CommandList.SetShaderResourceView(PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2);
    //TODO: Fix directional-light shadows
    //CmdList.SetShaderResourceView(PShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PShader.Get(), FrameResources.PointLightShadowMaps->GetShaderResourceView(), 4);

    CommandList.SetSamplerState(PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.LightProbeSampler.Get(), 2);
    CommandList.SetSamplerState(PShader.Get(), FrameResources.PointLightShadowSampler.Get(), 3);
    //CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 4);

    for (const FMeshBatch& Batch : Scene->CameraView.GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        if (!Material->ShouldRenderInForwardPass())
        {
            continue;
        }
        
        FRHIBuffer* ConstantBuffer = Material->GetMaterialBuffer();
        CommandList.SetConstantBuffer(PShader.Get(), ConstantBuffer, 6);
        
        CommandList.SetShaderResourceView(PShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 5);
        CommandList.SetShaderResourceView(PShader.Get(), Material->NormalMap->GetShaderResourceView(), 6);
        CommandList.SetShaderResourceView(PShader.Get(), Material->MaterialMap->GetShaderResourceView(), 7);
        CommandList.SetShaderResourceView(PShader.Get(), Material->HeightMap->GetShaderResourceView(), 8);
        
        FRHISamplerState* SamplerState = Material->GetMaterialSampler();
        CommandList.SetSamplerState(PShader.Get(), SamplerState, 0);

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

            FRHIBuffer* VertexBuffers[] =
            {
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Normals),
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
            };

            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 3), 0);
            CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

            CommandList.UpdateBuffer(FrameResources.TransformBuffer.Get(), FBufferRegion(0, sizeof(FTransformBufferHLSL)), &StaticMesh->GetTransformShaderData());
            CommandList.SetConstantBuffer(VShader.Get(), FrameResources.TransformBuffer.Get(), 1);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    CommandList.TransitionTextureState(FrameResources.ShadowCascades.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
}
