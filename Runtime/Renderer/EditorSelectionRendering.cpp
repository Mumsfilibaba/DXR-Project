#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Material.h"
#include "Renderer/DeferredRendering.h"
#include "Renderer/EditorSelectionRendering.h"
#include "Renderer/MaterialBindless.h"
#include "Renderer/PrePassShaders.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/VertexStreamCache.h"
#include "Core/Misc/ConsoleManager.h"

#if EDITOR_BUILD

static bool GEditorSelectionUseUnjitteredCamera = true;
static FAutoConsoleVariableRef CVarEditorSelectionUseUnjitteredCamera(
    "Renderer.Editor.Selection.UseUnjitteredCamera",
    "Use unjittered camera matrices for editor selection buffers (depth/ObjectID). Disable to better match TAA-jittered shading at the cost of more outline jitter.",
    GEditorSelectionUseUnjitteredCamera,
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

NODISCARD static FGraphicsPipelineKey MakeNoJitterDepthPSOKey(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration)
{
    const FPrePassShaderRules::FPermutation Permutation = FPrePassShaderRules::Create(Features, bBindless, GEditorSelectionUseUnjitteredCamera);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

NODISCARD static FGraphicsPipelineKey MakeSelectionIDPSOKey(const FMaterialFeatures& Features, const FVertexDeclaration& Declaration)
{
    const FSelectionIDShaderRules::FPermutation Permutation = FSelectionIDShaderRules::Create(Features);
    return FGraphicsPipelineKey(Permutation.GetPermutationID(), 0, Declaration.GetID());
}

FEditorNoJitterDepthPass::FEditorNoJitterDepthPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FEditorNoJitterDepthPass::~FEditorNoJitterDepthPass()
{
    MaterialPSOs.Clear();
}

void FEditorNoJitterDepthPass::PreparePipelineState(FMaterial* Material, const FVertexDeclaration& Declaration, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Material, Declaration);

    const int32 MaterialFlags = static_cast<int32>(Features.Flags);
    const bool  bBindless     = RHI::bSupportsBindless && GPrePassBindless;

    const FGraphicsPipelineKey     PSOKey      = MakeNoJitterDepthPSOKey(Features, bBindless, Declaration);
    const FPrePassVS::FPermutation Permutation = FPrePassVS::FPermutation(PSOKey.PermutationID);

    FGraphicsPipelineStateInstance* CachedPSO = MaterialPSOs.Find(PSOKey);
    if (CachedPSO)
    {
        return;
    }

    FGraphicsPipelineStateInstance NewPipelineInstance;
    NewPipelineInstance.VertexShader = FShaderCache::Get().GetShader<FPrePassVS>(Permutation);

    if (!NewPipelineInstance.VertexShader)
    {
        DEBUG_BREAK();
        return;
    }

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
    PSODesc.InputLayout                                = NewPipelineInstance.StreamBinding->InputLayout.Get();
    PSODesc.BlendState                                 = BlendState.Get();
    PSODesc.DepthStencilState                          = DepthStencilState.Get();
    PSODesc.RasterizerState                            = RasterizerState.Get();
    PSODesc.VertexShader                               = NewPipelineInstance.VertexShader.Get();
    PSODesc.PixelShader                                = NewPipelineInstance.PixelShader.Get();
    PSODesc.RasterizerOutputFormats.DepthStencilFormat = RendererTextureFormats::DepthBufferFormat;

    NewPipelineInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!NewPipelineInstance.PipelineState)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        const String DebugName = String::CreateFormatted("Editor NoJitter Depth PSO%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
        NewPipelineInstance.PipelineState->SetDebugName(DebugName);
    }

    MaterialPSOs.Add(PSOKey, Move(NewPipelineInstance));
}

bool FEditorNoJitterDepthPass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FEditorNoJitterDepthPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage           = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource;
    const FClearValue        DepthClearValue = FClearValue(RendererTextureFormats::DepthBufferFormat, 1.0f, 0);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    FrameResources.EditorNoJitterDepth = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);

    if (!FrameResources.EditorNoJitterDepth)
    {
        return false;
    }

    FrameResources.EditorNoJitterDepth->SetDebugName("Editor NoJitter Depth");
    return true;
}

void FEditorNoJitterDepthPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (!FrameResources.EditorNoJitterDepth)
    {
        return;
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorNoJitterDepth.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite));

    RHI_EVENT_SCOPE(CommandList, "Editor NoJitter Depth");
    TRACE_SCOPE("Editor NoJitter Depth");
    GPU_TRACE_SCOPE(CommandList, "Editor NoJitter Depth");

    FRHIDepthStencilView* DepthStencilView = FrameResources.EditorNoJitterDepth->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Clear);

    CommandList.BeginRenderPass(RenderPassDesc);

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

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

        const FGraphicsPipelineKey PSOKey = MakeNoJitterDepthPSOKey(Features, bBindless, Batch.Declaration);

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

        // The parallax march reads the camera position in the pixel shader, so it needs the camera buffer as well.
        if (Features.HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        }

        if (Features.HasAlphaMask() || Features.HasHeightMap())
        {
            if (Features.HasHeightMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 2);
            }

            CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

            if (Features.HasAlphaMask())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            }

            if (Features.HasHeightMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
            }
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

    CommandList.EndRenderPass();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorNoJitterDepth.Get(), ERHIResourceState::DepthWrite, ERHIResourceState::PixelShaderResource));
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

    const FGraphicsPipelineKey         PSOKey      = MakeSelectionIDPSOKey(Features, Declaration);
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
        const String DebugName = String::CreateFormatted("Editor SelectionID PSO %d", static_cast<int32>(Features.Flags));
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

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopySource;
    const FClearValue        ClearValue(RendererTextureFormats::ObjectIDFormat, 0.0f, 0.0f, 0.0f, 0.0f);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::ObjectIDFormat, Width, Height, 1, 1, Usage, ClearValue);
    FrameResources.EditorObjectID_NoJitter = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!FrameResources.EditorObjectID_NoJitter)
    {
        return false;
    }

    FrameResources.EditorObjectID_NoJitter->SetDebugName("Editor ObjectID NoJitter");
    return true;
}

void FEditorSelectionIDPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (!FrameResources.EditorNoJitterDepth || !FrameResources.EditorObjectID_NoJitter)
    {
        return;
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorNoJitterDepth.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite)); 
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorObjectID_NoJitter.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::RenderTarget));

    RHI_EVENT_SCOPE(CommandList, "Editor SelectionID");
    TRACE_SCOPE("Editor SelectionID");
    GPU_TRACE_SCOPE(CommandList, "Editor SelectionID");

    FRHIRenderTargetView* RenderTargetView = FrameResources.EditorObjectID_NoJitter->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = FrameResources.EditorNoJitterDepth->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 0.0f));
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    for (const FMeshBatch& Batch : Scene->GetCameraView().GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        CHECK(Material != nullptr);

        if (!Material->ShouldRenderInPrePass())
        {
            continue;
        }

        const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

        const FGraphicsPipelineKey PSOKey = MakeSelectionIDPSOKey(Features, Batch.Declaration);

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

        CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

        if (Features.HasAlphaMask())
        {
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
        }

        if (Features.HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), FrameResources.MaterialDataBufferSRV.Get(), 2);
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

    CommandList.EndRenderPass();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorObjectID_NoJitter.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::PixelShaderResource)); 
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.EditorNoJitterDepth.Get(), ERHIResourceState::DepthWrite, ERHIResourceState::PixelShaderResource)); 
}

#endif
