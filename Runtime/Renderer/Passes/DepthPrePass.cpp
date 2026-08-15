#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Renderer/Passes/DepthPrePass.h"
#include "Renderer/Shaders/PrePassShaders.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/VertexStreamCache.h"

bool GPrePassBindless = false;
static FAutoConsoleVariableRef CVarPrePassBindless(
    "Renderer.PrePass.Bindless",
    "When true, the depth pre-pass samples Albedo (alpha mask) and Height (parallax) via SM 6.6 ResourceDescriptorHeap[] / "
    "SamplerDescriptorHeap[] and a per-material indices buffer instead of register bindings.",
    GPrePassBindless);

NODISCARD static FGraphicsPipelineKey CreatePrePassPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FPrePassShaderRules::FPermutation Permutation = FPrePassShaderRules::Create(Features, bBindless, false);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FDepthPrePass::FDepthPrePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FDepthPrePass::~FDepthPrePass()
{
    MaterialPSOs.Clear();
}

void FDepthPrePass::PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Material, Declaration);

    const int32 MaterialFlags = static_cast<int32>(Features.Flags);
    const bool  bBindless     = RHI::bSupportsBindless && GPrePassBindless;

    const FGraphicsPipelineKey     PSOKey      = CreatePrePassPSOKey(Features, bBindless, Declaration);
    const FPrePassVS::FPermutation Permutation = FPrePassVS::FPermutation(PSOKey.PermutationID);

    FGraphicsPipelineStateInstance* CachedPrePassPSO = MaterialPSOs.Find(PSOKey);
    if (!CachedPrePassPSO)
    {
        FGraphicsPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = FShaderCache::Get().GetShader<FPrePassVS>(Permutation);

        if (!NewPipelineInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        // Nothing can discard without a height or alpha map, so the depth-only case needs no pixel shader.
        const bool bWantPixelShader = Features.HasHeightMap() || Features.HasAlphaMask();
        if (bWantPixelShader)
        {
            NewPipelineInstance.PixelShader = FShaderCache::Get().GetShader<FPrePassPS>(Permutation);
            if (!NewPipelineInstance.PixelShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = true;

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

        FRHIBlendStateDesc BlendStateDesc;

        FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerStateDesc);
        FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(BlendStateDesc);
        FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);

        if (!DepthStencilState || !RasterizerState || !BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.StreamBinding = FVertexStreamCache::Get().GetBinding(Declaration, Features.GetDepthOnlyAttributes());
        if (!NewPipelineInstance.StreamBinding)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                   = NewPipelineInstance.StreamBinding->InputLayout.Get();
        PSODesc.BlendState                                    = BlendState.Get();
        PSODesc.DepthStencilState                             = DepthStencilState.Get();
        PSODesc.RasterizerState                               = RasterizerState.Get();
        PSODesc.VertexShader                                  = NewPipelineInstance.VertexShader.Get();
        PSODesc.PixelShader                                   = NewPipelineInstance.PixelShader.Get();
        PSODesc.RasterizerOutputFormats.DepthStencilFormat    = RendererTextureFormats::DepthBufferFormat;
        PSODesc.MultiSampleState.bProgrammableSamplePositions = RHI::bSupportsProgrammableSamplePositions;

        NewPipelineInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const String DebugName = String::Printf("PrePass PipelineState%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
    }
}

bool FDepthPrePass::Initialize(FFrameResources& FrameResources)
{
    UNREFERENCED_VARIABLE(FrameResources);
    return true;
}

void FDepthPrePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    FRenderGraphTexture* Depth = Context.GBufferDepth;

    GraphBuilder.AddPass("DepthPrepass", ERenderGraphPassFlags::Raster, GPrePassEnabled,
        [Depth](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetDepthStencil(Depth, EAttachmentLoadAction::Clear);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            Record(PassCommandList, Context.CreatePassResources(PassResources), Context.Scene);
        });

    GraphBuilder.AddPass("ClearDepth", ERenderGraphPassFlags::Raster, !GPrePassEnabled,
        [Depth](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetDepthStencil(Depth, EAttachmentLoadAction::Clear);
        },
        [](FRHICommandList&, const FRenderGraphPassResources&)
        {
        });
}

void FDepthPrePass::Record(FRHICommandList& CommandList, const FPassResources& PassResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "Depth Pre-Pass");

    TRACE_SCOPE("Depth Pre-Pass");

    GPU_TRACE_SCOPE(CommandList, "Depth Pre-Pass");

    const float RenderWidth  = float(PassResources.RenderWidth);
    const float RenderHeight = float(PassResources.RenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const FFrameResources& FrameResources = *PassResources.FrameResources;

    const bool bBindless = RHI::bSupportsBindless && GPrePassBindless && FrameResources.MaterialDataBufferSRV.IsValid();
    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        CHECK(Material != nullptr);

        if (!Material->ShouldRenderInPrePass())
        {
            continue;
        }

        const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

        const FGraphicsPipelineKey PSOKey = CreatePrePassPSOKey(Features, bBindless, Batch.Declaration);

        FGraphicsPipelineStateInstance* PipelineInstance = MaterialPSOs.Find(PSOKey);
        if (!PipelineInstance)
        {
            DEBUG_BREAK();
            continue;
        }

        FRHIGraphicsPipelineState* PipelineState = PipelineInstance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);

        CommandList.SetGraphicsPipelineState(PipelineState);
        CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (Features.HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        }

        if (Features.HasAlphaMask() || Features.HasHeightMap())
        {
            if (!bBindless)
            {
                BindMaterialTextures(CommandList, PipelineInstance->PixelShader.Get(), Features, *Material, 0);
            }

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 7);
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
            StaticMesh->Mesh->SetVertexBuffers(CommandList, *PipelineInstance->StreamBinding);

            CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

            const int32 MaxMaterialIndex = Math::Max<int32>(int32(FrameResources.MaterialData.Size()) - 1, 0);
            StaticMesh->PerObjectBuffer.MaterialIndex = uint32(Math::Clamp<int32>(Material->GetBufferIndex(), 0, MaxMaterialIndex));

            CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);
            CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.PerObjectBuffer.Get(), 1);

            if (FRHIPixelShader* PixelShader = PipelineInstance->PixelShader.Get())
            {
                CommandList.SetConstantBuffer(PixelShader, FrameResources.PerObjectBuffer.Get(), 1);
            }

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }
}
