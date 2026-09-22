#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineState.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/Passes/CascadedShadowsRenderPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/VertexStreamCache.h"

extern bool GShadowsBindless;

static TAutoConsoleVariable<bool> CVarCSMEnableSinglePassRendering(
    "Renderer.CSM.EnableSinglePassRendering",
    "Enables instancing for cascade rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less "
    "overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableGeometryShaderInstancing(
    "Renderer.CSM.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableViewInstancing(
    "Renderer.CSM.EnableViewInstancing",
    "Enables view-instancing for cascade rendering, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableDepthClipping(
    "Renderer.CSM.EnableDepthClipping",
    "Enables depth-clipping for cascade rendering.",
    false,
    EConsoleVariableFlags::Default);

FCascadedShadowsRenderPass::FCascadedShadowsRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerCascadeBuffer(nullptr)
{
}

FCascadedShadowsRenderPass::~FCascadedShadowsRenderPass()
{
    MaterialPSOs.Clear();
    PerCascadeBuffer.Reset();
}

FGraphicsPipelineStateInstance* FCascadedShadowsRenderPass::CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& /* FrameResources */ )
{
    const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

    const int32 MaterialFlags        = static_cast<int32>(Features.Flags);
    const bool  bBindless            = RHI::bSupportsBindless && GShadowsBindless;
    const bool  bEnableDepthClipping = CVarCSMEnableDepthClipping.GetValue();

    const uint32 PipelineFlags =
        (Features.IsDoubleSided() ? PIPELINE_FLAG_DOUBLE_SIDED : 0u) |
        (bEnableDepthClipping ? PIPELINE_FLAG_DEPTH_CLIPPING : 0u);

    const FCascadeShadowVS::FPermutation Permutation = FCascadeShadowRules::Create(Features, bBindless, RenderPassType);
    const FGraphicsPipelineKey           PSOKey      = FGraphicsPipelineKey(Permutation.GetPermutationID(), PipelineFlags, Batch.Declaration.GetID());

    FGraphicsPipelineStateInstance* CachedDirectionalLightPSO = MaterialPSOs.Find(PSOKey);
    if (!CachedDirectionalLightPSO)
    {
        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = FShaderCache::Get().GetShader<FCascadeShadowVS>(Permutation);

        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            NewPipelineStateInstance.GeometryShader = FShaderCache::Get().GetShader<FCascadeShadowGS>(Permutation);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        const bool bWantPixelShader = Features.HasHeightMap() || Features.HasAlphaMask();
        if (bWantPixelShader)
        {
            NewPipelineStateInstance.PixelShader = FShaderCache::Get().GetShader<FCascadeShadowPS>(Permutation);
            if (!NewPipelineStateInstance.PixelShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        NewPipelineStateInstance.StreamBinding = FVertexStreamCache::Get().GetBinding(Batch.Declaration, Features.GetDepthOnlyAttributes());
        if (!NewPipelineStateInstance.StreamBinding)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = true;

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.bDepthClipEnable = bEnableDepthClipping;

        if (!RHI::bSupportsDynamicDepthBias)
        {
            RasterizerStateDesc.DepthBias            = 1.0f;
            RasterizerStateDesc.DepthBiasClamp       = 0.05f;
            RasterizerStateDesc.SlopeScaledDepthBias = 1.0f;
        }

        RasterizerStateDesc.CullMode = Features.IsDoubleSided() ? ECullMode::None : ECullMode::Back;

        FRHIBlendStateDesc BlendStateDesc;

        FRHIRasterizerStateRef   RasterizerState   = RHI::CreateRasterizerState(RasterizerStateDesc);
        FRHIBlendStateRef        BlendState        = RHI::CreateBlendState(BlendStateDesc);
        FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);

        if (!DepthStencilState || !RasterizerState || !BlendState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                   = BlendState.Get();
        PSODesc.DepthStencilState            = DepthStencilState.Get();
        PSODesc.bPrimitiveRestartEnable      = false;
        PSODesc.InputLayout                  = NewPipelineStateInstance.StreamBinding->InputLayout.Get();
        PSODesc.PrimitiveTopology            = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerState              = RasterizerState.Get();
        PSODesc.MultiSampleState.SampleCount = 1;
        PSODesc.MultiSampleState.SampleMask  = RHI_DEFAULT_SAMPLE_MASK;
        PSODesc.VertexShader                 = NewPipelineStateInstance.VertexShader.Get();
        PSODesc.GeometryShader               = NewPipelineStateInstance.GeometryShader.Get();
        PSODesc.PixelShader                  = NewPipelineStateInstance.PixelShader.Get();

        if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            PSODesc.ViewInstancingState.StartRenderTargetArrayIndex = 0;
            PSODesc.ViewInstancingState.NumArraySlices              = NUM_SHADOW_CASCADES;
            PSODesc.ViewInstancingState.bEnableViewInstancing       = true;
        }

        PSODesc.RasterizerOutputFormats.DepthStencilFormat = RendererTextureFormats::ShadowMapFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets   = 0;

        NewPipelineStateInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return nullptr;
        }
        else
        {
            const String DebugName = String::Printf("CSM PipelineState%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        FGraphicsPipelineStateInstance& NewInstance = MaterialPSOs.Add(PSOKey, Move(NewPipelineStateInstance));
        return &NewInstance;
    }
    else
    {
        return CachedDirectionalLightPSO;
    }
}

bool FCascadedShadowsRenderPass::Initialize(FFrameResources& Resources)
{
    const FRHIBufferDesc PerCascadeBufferDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FPerCascadeHLSL));
    PerCascadeBuffer = RHI::CreateBuffer(PerCascadeBufferDesc, ERHIResourceState::ConstantBuffer, nullptr);
    if (!PerCascadeBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PerCascadeBuffer->SetDebugName("Per Cascade Buffer");
    }

    return CreateResources(Resources);
}

bool FCascadedShadowsRenderPass::CreateResources(FFrameResources& Resources)
{
    const ETextureUsageFlags Flags =
        ETextureUsageFlags::DepthStencil |
        ETextureUsageFlags::ShaderResourceTexture |
        ETextureUsageFlags::NoDefaultDSV;

    const FClearValue DepthClearValue(RendererTextureFormats::ShadowMapFormat, 1.0f, 0);
    FRHITextureDesc CascadeDesc = FRHITextureDesc::CreateTexture2DArray(RendererTextureFormats::ShadowMapFormat, Resources.CascadeSize,
        Resources.CascadeSize, NUM_SHADOW_CASCADES, 1, 1, Flags, DepthClearValue);
    Resources.ShadowCascades = RHI::CreateTexture(CascadeDesc, ERHIResourceState::NonPixelShaderResource);

    if (Resources.ShadowCascades)
    {
        const String DebugName = String::Printf("Shadow Map Cascades");
        Resources.ShadowCascades->SetDebugName(DebugName);
    }
    else
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FCascadedShadowsRenderPass::Record(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const auto GetRenderMapRenderPassType = []() -> ECascadeRenderPassType
    {
        constexpr uint32 MinViewInstanceCount = 4;

        const bool bUseVSInstancing =
            RHI::bSupportRenderTargetArrayIndexFromVertexShader &&
            CVarCSMEnableSinglePassRendering.GetValue();

        const bool bUseGSInstancing =
            !bUseVSInstancing &&
            RHI::bSupportsGeometryShaders &&
            CVarCSMEnableGeometryShaderInstancing.GetValue();

        const bool bUseViewInstancing =
            !bUseGSInstancing &&
            RHI::bSupportsViewInstancing &&
            RHI::MaxViewInstanceCount >= MinViewInstanceCount &&
            CVarCSMEnableViewInstancing.GetValue();

        if (bUseVSInstancing)
        {
            return ECascadeRenderPassType::SinglePass;
        }
        else if (bUseGSInstancing)
        {
            return ECascadeRenderPassType::GeometryShaderSinglePass;
        }
        else if (bUseViewInstancing)
        {
            return ECascadeRenderPassType::ViewInstancingSinglePass;
        }
        else
        {
            return ECascadeRenderPassType::MultiPass;
        }
    };

    RHI_EVENT_SCOPE(CommandList, "Render DirectionalLight ShadowMaps");

    TRACE_SCOPE("Render DirectionalLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

    const ECascadeRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (Scene->GetDirectionalLight())
    {
        if (RenderPassType == ECascadeRenderPassType::SinglePass)
        {
            RecordInternal<ECascadeRenderPassType::SinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            RecordInternal<ECascadeRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            RecordInternal<ECascadeRenderPassType::ViewInstancingSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::MultiPass)
        {
            RecordInternal<ECascadeRenderPassType::MultiPass>(CommandList, Resources, Scene);
        }
    }
}

template<ECascadeRenderPassType RenderPassType>
void FCascadedShadowsRenderPass::RecordInternal(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const bool bBindless =
        RHI::bSupportsBindless &&
        GShadowsBindless &&
        Resources.MaterialDataBufferSRV.IsValid();

    constexpr bool bIsSinglePass =
        RenderPassType == ECascadeRenderPassType::SinglePass ||
        RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass ||
        RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass;

    FSceneDirectionalLight* SceneDirectionalLight = Scene->GetDirectionalLight();
    if constexpr (bIsSinglePass)
    {
        FRHIDepthStencilView* DepthStencilView = Resources.ShadowCascadesCombinedDSV.Get();
        if (!DepthStencilView)
        {
            LOG_ERROR("CascadedShadows single-pass render requires ShadowCascadesCombinedDSV, but it was not resolved");
            return;
        }

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView);

        // Setup view-instancing
        if constexpr (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            RenderPassDesc.ViewInstancingState.StartRenderTargetArrayIndex = 0;
            RenderPassDesc.ViewInstancingState.NumArraySlices              = NUM_SHADOW_CASCADES;
            RenderPassDesc.ViewInstancingState.bEnableViewInstancing       = true;
        }

        CommandList.BeginRenderPass(RenderPassDesc);

        if (RHI::bSupportsDynamicDepthBias)
        {
            CommandList.SetDepthBias(1.0f, 0.05f, 1.0f);
        }

        const float CascadeSize = static_cast<float>(Resources.CascadeSize);
        FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.SetViewport(ViewportRegion);

        FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
        CommandList.SetScissorRect(ScissorRegion);

        for (const FMeshBatch& Batch : SceneDirectionalLight->ShadowView.GetMeshBatches())
        {
            FMaterial* Material = Batch.Material;
            const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);
            FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Batch, Resources);
            if (!Instance)
            {
                DEBUG_BREAK();
            }

            FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
            CHECK(PipelineState != nullptr);

            CommandList.SetGraphicsPipelineState(PipelineState);

            // If we are using geometry-shaders bind the necessary buffer to the geometry-shader otherwise to the vertex-shader
            if constexpr (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
            {
                CommandList.SetShaderResourceView(Instance->GeometryShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
            }
            else
            {
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
            }

            // If this material require a pixel-shader, bind necessary pixel-shader resources
            if (Instance->PixelShader)
            {
                if (bBindless)
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                }
                else
                {
                    BindMaterialTextures(CommandList, Instance->PixelShader.Get(), Features, *Material, 0);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                }

                if (Features.HasHeightMap())
                {
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 3);
                }
            }

            // Draw all the objects
            for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
            {
                FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                StaticMesh->Mesh->SetVertexBuffers(CommandList, *Instance->StreamBinding);

                CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

                StaticMesh->PerObjectBuffer.MaterialIndex = Material->GetBufferIndex();

                CommandList.UpdateBuffer(Resources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);
                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), Resources.PerObjectBuffer.Get(), 1);

                if (FRHIPixelShader* PixelShader = Instance->PixelShader.Get())
                {
                    CommandList.SetConstantBuffer(PixelShader, Resources.PerObjectBuffer.Get(), 1);
                }

                if constexpr (RenderPassType == ECascadeRenderPassType::SinglePass)
                {
                    constexpr uint32 SinglePassInstanceCount = 4;
                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                }
                else
                {
                    constexpr uint32 SinglePassInstanceCount = 1;
                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                }
            }
        }

        CommandList.EndRenderPass();
    }
    else
    {
        for (uint32 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            FPerCascadeHLSL PerCascadeData;
            PerCascadeData.CascadeIndex = static_cast<int32>(Index);

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(PerCascadeBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
            CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascadeHLSL)), &PerCascadeData);
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(PerCascadeBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));

            FRHIDepthStencilView* DepthStencilView = Resources.ShadowCascadePerCascadeDSVs[Index].Get();
            if (!DepthStencilView)
            {
                LOG_ERROR("CascadedShadows multi-pass render requires ShadowCascadePerCascadeDSVs[%u], but it was not resolved", Index);
                continue;
            }

            FRHIBeginRenderPassDesc RenderPassDesc;
            RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView);

            CommandList.BeginRenderPass(RenderPassDesc);

            if (RHI::bSupportsDynamicDepthBias)
            {
                CommandList.SetDepthBias(1.0f, 0.05f, 1.0f);
            }

            const float CascadeSize = static_cast<float>(Resources.CascadeSize);
            FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : SceneDirectionalLight->ShadowView.GetMeshBatches())
            {
                FMaterial* Material = Batch.Material;
                const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

                FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Batch, Resources);
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);
                CommandList.SetGraphicsPipelineState(PipelineState);

                // Bind pixel-shader resources if there are any
                if (Instance->PixelShader)
                {
                    if (bBindless)
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                    }
                    else
                    {
                        BindMaterialTextures(CommandList, Instance->PixelShader.Get(), Features, *Material, 0);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                    }

                    if (Features.HasHeightMap())
                    {
                        CommandList.SetConstantBuffer(Instance->PixelShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 3);
                    }
                }

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerCascadeBuffer.Get(), 0);
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
                {
                    FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                    StaticMesh->Mesh->SetVertexBuffers(CommandList, *Instance->StreamBinding);

                    CommandList.SetIndexBuffer(StaticMesh->IndexBuffer, StaticMesh->IndexFormat);

                    StaticMesh->PerObjectBuffer.MaterialIndex = Material->GetBufferIndex();

                    CommandList.UpdateBuffer(Resources.PerObjectBuffer.Get(), FBufferRegion(0, sizeof(FPerObjectHLSL)), &StaticMesh->PerObjectBuffer);
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), Resources.PerObjectBuffer.Get(), 1);

                    if (FRHIPixelShader* PixelShader = Instance->PixelShader.Get())
                    {
                        CommandList.SetConstantBuffer(PixelShader, Resources.PerObjectBuffer.Get(), 1);
                    }

                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
    }
}

void FCascadedShadowsRenderPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    FSceneDirectionalLight* DirectionalLight = Context.Scene ? Context.Scene->GetDirectionalLight() : nullptr;
    const bool bEnableSunShadows = GSunShadowsEnabled && (!DirectionalLight || DirectionalLight->bCastShadows);
    const bool bEnablePass       = GShadowsEnabled && bEnableSunShadows;

    GraphBuilder.AddPass("CascadedShadows", ERenderGraphPassFlags::None, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            if (Context.ShadowCascades)
            {
                PassBuilder.WriteTexture(Context.ShadowCascades, ERHIResourceState::DepthWrite);
            }

            if (Context.ShadowCascadesCombinedDSV)
            {
                PassBuilder.UseDepthStencilView(Context.ShadowCascadesCombinedDSV, ERHIResourceState::DepthWrite, true);
            }

            for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
            {
                if (Context.ShadowCascadeDSVs[CascadeIndex])
                {
                    PassBuilder.UseDepthStencilView(Context.ShadowCascadeDSVs[CascadeIndex], ERHIResourceState::DepthWrite, true);
                }
            }

            if (Context.CascadeMatrixBufferSRV)
            {
                PassBuilder.Read(Context.CascadeMatrixBufferSRV);
            }

            if (Context.PerCascadeBuffer)
            {
                PassBuilder.ReadBuffer(Context.PerCascadeBuffer, ERHIResourceState::ConstantBuffer);
            }
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncCascadedShadowViews(PassResources, Context, *Context.FrameResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene);
        });
}
