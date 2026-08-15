#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Material.h"
#include "Renderer/Passes/Editor/EditorSelectionIDPass.h"
#include "Renderer/Shaders/PrePassShaders.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/VertexStreamCache.h"
#include "Core/Misc/ConsoleManager.h"

#if EDITOR_BUILD

extern bool GEditorSelectionUseUnjitteredCamera;

static bool GEditorSelectionAllowTranslucent = true;
static FAutoConsoleVariableRef CVarEditorSelectionAllowTranslucent(
    "Renderer.Editor.Selection.AllowTranslucent",
    "Let clicks and box-selects land on translucent surfaces. Disable to pick straight through them, which is easier when a large pane of "
    "glass covers what you are trying to select.",
    GEditorSelectionAllowTranslucent,
    EConsoleVariableFlags::Default);

struct FSelectionIDShaderRules
{
    using FPermutation = TShaderPermutation<FMaterialPermutation, FUnjitteredCamera, FRequiredAttributes>;

    static_assert(FPermutation::PermutationCount == 512, "EditorSelectionID permutation space grew unexpectedly");

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features)
    {
        FPermutation Permutation;
        Permutation.Set<FMaterialPermutation>(Features.CreatePermutation());
        Permutation.Set<FUnjitteredCamera>(GEditorSelectionUseUnjitteredCamera);
        return RemapPermutation(Permutation);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        return RemapDepthOnlyAttributes(RemapMaterialPermutation(Permutation));
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        return RemapPermutation(Permutation) == Permutation;
    }
};

class FSelectionIDVS : public FSelectionIDShaderRules
{
    DECLARE_SHADER_TYPE(FSelectionIDVS, EShaderStage::Vertex);
};

class FSelectionIDPS : public FSelectionIDShaderRules
{
    DECLARE_SHADER_TYPE(FSelectionIDPS, EShaderStage::Pixel);
};

IMPLEMENT_SHADER_TYPE(FSelectionIDVS, "Shaders/EditorSelectionID.hlsl", "VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FSelectionIDPS, "Shaders/EditorSelectionID.hlsl", "PSMain", EShaderModel::SM_6_2);

NODISCARD static FGraphicsPipelineKey CreateSelectionIDPSOKey(const FMaterialFeatures& Features, const FVertexDeclaration& Declaration)
{
    const FSelectionIDShaderRules::FPermutation Permutation = FSelectionIDShaderRules::Create(Features);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FEditorSelectionIDPass::FEditorSelectionIDPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FEditorSelectionIDPass::~FEditorSelectionIDPass()
{
    MaterialPSOs.Clear();
}

void FEditorSelectionIDPass::PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Material, Declaration);

    const FGraphicsPipelineKey         PSOKey      = CreateSelectionIDPSOKey(Features, Declaration);
    const FSelectionIDVS::FPermutation Permutation = FSelectionIDVS::FPermutation(PSOKey.PermutationID);

    FGraphicsPipelineStateInstance* CachedPSO = MaterialPSOs.Find(PSOKey);
    if (CachedPSO)
    {
        return;
    }

    FGraphicsPipelineStateInstance NewPipelineInstance;
    NewPipelineInstance.VertexShader = FShaderCache::Get().GetShader<FSelectionIDVS>(Permutation);
    if (!NewPipelineInstance.VertexShader)
    {
        DEBUG_BREAK();
        return;
    }

    NewPipelineInstance.PixelShader = FShaderCache::Get().GetShader<FSelectionIDPS>(Permutation);
    if (!NewPipelineInstance.PixelShader)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

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
    PSODesc.InputLayout                                    = NewPipelineInstance.StreamBinding->InputLayout.Get();
    PSODesc.BlendState                                     = BlendState.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.VertexShader                                   = NewPipelineInstance.VertexShader.Get();
    PSODesc.PixelShader                                    = NewPipelineInstance.PixelShader.Get();
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RendererTextureFormats::ObjectIDFormat;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

    NewPipelineInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!NewPipelineInstance.PipelineState)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        const String DebugName = String::Printf("Editor SelectionID PSO %d", static_cast<int32>(Features.Flags));
        NewPipelineInstance.PipelineState->SetDebugName(DebugName);
    }

    MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
}

bool FEditorSelectionIDPass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FEditorSelectionIDPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage =
        ETextureUsageFlags::RenderTarget |
        ETextureUsageFlags::ShaderResourceTexture |
        ETextureUsageFlags::CopySource;

    const FClearValue ClearValue(RendererTextureFormats::ObjectIDFormat, 0.0f, 0.0f, 0.0f, 0.0f);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::ObjectIDFormat, Width, Height, 1, 1, Usage, ClearValue);
    FrameResources.EditorObjectID_NoJitter = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);

    if (!FrameResources.EditorObjectID_NoJitter)
    {
        return false;
    }

    FrameResources.EditorObjectID_NoJitter->SetDebugName("Editor ObjectID NoJitter");
    return true;
}

void FEditorSelectionIDPass::Record(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (!FrameResources.EditorNoJitterDepth || !FrameResources.EditorObjectID_NoJitter)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Editor SelectionID");
    TRACE_SCOPE("Editor SelectionID");
    GPU_TRACE_SCOPE(CommandList, "Editor SelectionID");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const auto DrawBatches = [&](bool bTranslucentPhase)
    {
        for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
        {
            FMaterial* Material = Batch.Material;
            CHECK(Material != nullptr);

            if (Material->IsTranslucent() != bTranslucentPhase)
            {
                continue;
            }

            const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

            const FGraphicsPipelineKey PSOKey = CreateSelectionIDPSOKey(Features, Batch.Declaration);

            FGraphicsPipelineStateInstance* PipelineInstance = MaterialPSOs.Find(PSOKey);
            if (!PipelineInstance)
            {
                DEBUG_BREAK();
                continue;
            }

            FRHIGraphicsPipelineState* PipelineState = PipelineInstance->PipelineState.Get();
            CHECK(PipelineState != nullptr);
            CommandList.SetGraphicsPipelineState(PipelineState);

            CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

            BindMaterialTextures(CommandList, PipelineInstance->PixelShader.Get(), Features, *Material, 0);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 7);

            if (Features.HasHeightMap())
            {
                CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
            }

            for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
            {
                FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

                StaticMesh->Mesh->SetVertexBuffers(CommandList, *PipelineInstance->StreamBinding);
                CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

                StaticMesh->PerObjectBuffer.MaterialIndex = Material->GetBufferIndex();

                CommandList.UpdateBuffer(FrameResources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);
                CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.PerObjectBuffer.Get(), 1);

                if (FRHIPixelShader* PixelShader = PipelineInstance->PixelShader.Get())
                {
                    CommandList.SetConstantBuffer(PixelShader, FrameResources.PerObjectBuffer.Get(), 1);
                }

                CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
            }
        }
    };

    DrawBatches(false);

    if (GEditorSelectionAllowTranslucent)
    {
        DrawBatches(true);
    }
}

void FEditorSelectionIDPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("EditorSelectionID", ERenderGraphPassFlags::Raster, true,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetRenderTarget(0, Context.EditorObjectID_NoJitter, EAttachmentLoadAction::Clear);
            PassBuilder.SetDepthStencil(Context.EditorNoJitterDepth, EAttachmentLoadAction::Load, EAttachmentStoreAction::Store, FDepthStencilValue(1.0f, 0), true);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene);
        });
}

#endif
