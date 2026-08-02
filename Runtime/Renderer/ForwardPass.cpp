#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/MaterialBindless.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

static bool GForwardPassBindless = false;
static FAutoConsoleVariableRef CVarForwardPassBindless(
    "Renderer.ForwardPass.Bindless",
    "When true, the forward pass samples per-material textures (Albedo / Normal / Material / Height) and the material sampler via SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of register bindings. Full-frame SRVs / samplers (sky, integration LUT, shadow maps) remain non-bindless.",
    GForwardPassBindless);

FForwardPass::FForwardPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FForwardPass::~FForwardPass()
{
    PipelineStates.Clear();
}

bool FForwardPass::CompilePipelineState(FFrameResources& FrameResources, bool bBindless)
{
    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
        { "BINDLESS_FORWARD_PASS", bBindless ? "(1)" : "(0)" },
    };

    const EShaderModel TargetShaderModel = bBindless ? EShaderModel::SM_6_6 : EShaderModel::SM_6_2;

    TArray<uint8> ShaderCode;

    FGraphicsPipelineStateInstance NewInstance;

    FShaderCompileInfo CompileInfo("VSMain", TargetShaderModel, EShaderStage::Vertex, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    NewInstance.VertexShader = RHI::CreateVertexShader(ShaderCode);
    if (!NewInstance.VertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSMain", TargetShaderModel, EShaderStage::Pixel, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ForwardPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    NewInstance.PixelShader = RHI::CreatePixelShader(ShaderCode);
    if (!NewInstance.PixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = true;

    NewInstance.DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!NewInstance.DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    NewInstance.RasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!NewInstance.RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;
    BlendStateDesc.RenderTargets[0].bBlendEnable = true;
    BlendStateDesc.RenderTargets[0].SrcBlend = EBlendType::One;
    BlendStateDesc.RenderTargets[0].DstBlend = EBlendType::Zero;

    NewInstance.BlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!NewInstance.BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    NewInstance.InputLayout = FrameResources.MeshInputLayout;

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.VertexShader                                   = NewInstance.VertexShader.Get();
    PSODesc.PixelShader                                    = NewInstance.PixelShader.Get();
    PSODesc.InputLayout                                    = NewInstance.InputLayout.Get();
    PSODesc.DepthStencilState                              = NewInstance.DepthStencilState.Get();
    PSODesc.BlendState                                     = NewInstance.BlendState.Get();
    PSODesc.RasterizerState                                = NewInstance.RasterizerState.Get();
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RendererTextureFormats::SceneTargetFormat;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    NewInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!NewInstance.PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }

    const String DebugName = String::CreateFormatted("ForwardPass PipelineState%s", bBindless ? " [Bindless]" : "");
    NewInstance.PipelineState->SetDebugName(DebugName);

    PipelineStates.Add(MakeMaterialPSOKey(0, bBindless), Move(NewInstance));
    return true;
}

bool FForwardPass::Initialize(FFrameResources& FrameResources)
{
    if (!CompilePipelineState(FrameResources, false))
    {
        return false;
    }

    if (RHI::bSupportsBindless && !CompilePipelineState(FrameResources, true))
    {
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

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ShadowCascades.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::PixelShaderResource));

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIRenderTargetView* RenderTargetView = FrameResources.SceneTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = FrameResources.GBuffer[EGBufferIndex::Depth]->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);
    CommandList.BeginRenderPass(RenderPassDesc);

    const bool bBindless = RHI::bSupportsBindless && GForwardPassBindless && FrameResources.MaterialDataBufferSRV.IsValid();

    FGraphicsPipelineStateInstance* PipelineInstance = PipelineStates.Find(MakeMaterialPSOKey(0, bBindless));
    if (!PipelineInstance)
    {
        CommandList.EndRenderPass();
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ShadowCascades.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));
        DEBUG_BREAK();
        return;
    }

    FRHIVertexShaderRef VShader = PipelineInstance->VertexShader;
    FRHIPixelShaderRef  PShader = PipelineInstance->PixelShader;

    CommandList.SetGraphicsPipelineState(PipelineInstance->PipelineState.Get());

    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.CameraBuffer.Get(), 0);
    // TODO: Fix point-light count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(PShader.Get(), FrameResources.DirectionalLightDataBuffer.Get(), 5);

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
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

    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        if (!Material->ShouldRenderInForwardPass())
        {
            continue;
        }
        
        CommandList.SetShaderResourceView(PShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 9);

        if (bBindless)
        {
            // TODO: 
        }
        else
        {
            CommandList.SetShaderResourceView(PShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 5);
            CommandList.SetShaderResourceView(PShader.Get(), Material->NormalMap->GetShaderResourceView(), 6);
            CommandList.SetShaderResourceView(PShader.Get(), Material->MaterialMap->GetShaderResourceView(), 7);
            CommandList.SetShaderResourceView(PShader.Get(), Material->HeightMap->GetShaderResourceView(), 8);

            FRHISamplerState* SamplerState = Material->GetMaterialSampler();
            CommandList.SetSamplerState(PShader.Get(), SamplerState, 0);
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

            FRHIBuffer* VertexBuffers[] =
            {
                StaticMesh->Mesh->GetVertexBuffer(EVertexStream::Positions),
                StaticMesh->Mesh->GetVertexBuffer(EVertexStream::Normals),
                StaticMesh->Mesh->GetVertexBuffer(EVertexStream::TexCoords),
            };

            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 3), 0);
            CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

            StaticMesh->PerObjectBuffer.MaterialIndex = Material->GetBufferIndex();
            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);

            CommandList.SetConstantBuffer(VShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);
            CommandList.SetConstantBuffer(PShader.Get(), FrameResources.PerObjectBuffer.Get(), 6);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.ShadowCascades.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::NonPixelShaderResource));
}
