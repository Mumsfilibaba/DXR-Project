#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineState.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Renderer/Passes/PointLightRenderPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/VertexStreamCache.h"

bool GShadowsBindless = false;
static FAutoConsoleVariableRef CVarShadowsBindless(
    "Renderer.Shadows.Bindless",
    "When true, point-light cube shadows and cascaded directional shadows sample the alpha-mask / parallax-height textures and material "
    "sampler through SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of conventional "
    "register bindings.",
    GShadowsBindless);

static TAutoConsoleVariable<bool> CVarPointLightsEnableSinglePassRendering(
    "Renderer.PointLights.EnableSinglePassRendering",
    "Enables instancing for cube-map rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less "
    "overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightsEnableGeometryShaderInstancing(
    "Renderer.PointLights.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cube-map drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

FPointLightRenderPass::FPointLightRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerShadowMapBuffer(nullptr)
    , SinglePassShadowMapBuffer(nullptr)
{
}

FPointLightRenderPass::~FPointLightRenderPass()
{
    MaterialPSOs.Clear();
    PerShadowMapBuffer.Reset();
    SinglePassShadowMapBuffer.Reset();
}

FGraphicsPipelineStateInstance* FPointLightRenderPass::CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& /* FrameResources */)
{
    const FMaterialFeatures Features(Batch.EffectiveMaterialFlags);

    const int32 MaterialFlags = static_cast<int32>(Features.Flags);
    const bool  bBindless     = RHI::bSupportsBindless && GShadowsBindless;

    const FPointLightShadowVS::FPermutation Permutation = FPointLightShadowRules::Create(Features, bBindless, RenderPassType);

    const uint32               PipelineFlags = Features.IsDoubleSided() ? PIPELINE_FLAG_DOUBLE_SIDED : 0u;
    const FGraphicsPipelineKey PSOKey        = FGraphicsPipelineKey(Permutation.GetPermutationID(), PipelineFlags, Batch.Declaration.GetID());

    FGraphicsPipelineStateInstance* CachedPointLightPSO = MaterialPSOs.Find(PSOKey);
    if (!CachedPointLightPSO)
    {
        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = FShaderCache::Get().GetShader<FPointLightShadowVS>(Permutation);

        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            NewPipelineStateInstance.GeometryShader = FShaderCache::Get().GetShader<FPointLightShadowGS>(Permutation);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        NewPipelineStateInstance.PixelShader = FShaderCache::Get().GetShader<FPointLightShadowPS>(Permutation);
        if (!NewPipelineStateInstance.PixelShader)
        {
            DEBUG_BREAK();
            return nullptr;
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
        PSODesc.BlendState                                 = BlendState.Get();
        PSODesc.DepthStencilState                          = DepthStencilState.Get();
        PSODesc.bPrimitiveRestartEnable                    = false;
        PSODesc.InputLayout                                = NewPipelineStateInstance.StreamBinding->InputLayout.Get();
        PSODesc.PrimitiveTopology                          = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerState                            = RasterizerState.Get();
        PSODesc.MultiSampleState.SampleCount               = 1;
        PSODesc.MultiSampleState.SampleMask                = RHI_DEFAULT_SAMPLE_MASK;
        PSODesc.VertexShader                               = NewPipelineStateInstance.VertexShader.Get();
        PSODesc.GeometryShader                             = NewPipelineStateInstance.GeometryShader.Get();
        PSODesc.PixelShader                                = NewPipelineStateInstance.PixelShader.Get();
        PSODesc.RasterizerOutputFormats.NumRenderTargets   = 0;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat = RendererTextureFormats::ShadowMapFormat;

        NewPipelineStateInstance.PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return nullptr;
        }
        else
        {
            const String DebugName = String::Printf("Point ShadowMap PipelineState%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        FGraphicsPipelineStateInstance& NewInstance = MaterialPSOs.Add(PSOKey, Move(NewPipelineStateInstance));
        return &NewInstance;
    }
    else
    {
        return CachedPointLightPSO;
    }
}

bool FPointLightRenderPass::Initialize(FFrameResources& Resources)
{
    const FRHIBufferDesc PerShadowMapBufferDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FPerShadowMapHLSL));
    PerShadowMapBuffer = RHI::CreateBuffer(PerShadowMapBufferDesc, ERHIResourceState::ConstantBuffer, nullptr);

    if (!PerShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetDebugName("Per ShadowMap Buffer");
    }

    const FRHIBufferDesc SinglePassShadowMapBufferDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FSinglePassPointLightBufferHLSL));
    SinglePassShadowMapBuffer = RHI::CreateBuffer(SinglePassShadowMapBufferDesc, ERHIResourceState::ConstantBuffer, nullptr);

    if (!SinglePassShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SinglePassShadowMapBuffer->SetDebugName("Single-Pass ShadowMap Buffer");
    }

    return CreateResources(Resources);
}

bool FPointLightRenderPass::CreateResources(FFrameResources& Resources)
{
    const FClearValue DepthClearValue(RendererTextureFormats::ShadowMapFormat, 1.0f, 0);

    const ETextureUsageFlags Flags =
        ETextureUsageFlags::DepthStencil |
        ETextureUsageFlags::ShaderResourceTexture |
        ETextureUsageFlags::NoDefaultDSV;

    FRHITextureDesc PointLightDesc = FRHITextureDesc::CreateTextureCubeArray(RendererTextureFormats::ShadowMapFormat, Resources.PointLightShadowSize, Resources.MaxPointLightShadows, 1, 1, Flags, DepthClearValue);
    Resources.PointLightShadowMaps = RHI::CreateTexture(PointLightDesc, ERHIResourceState::NonPixelShaderResource);

    if (Resources.PointLightShadowMaps)
    {
        Resources.PointLightShadowMaps->SetDebugName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    return true;
}

void FPointLightRenderPass::Record(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const auto GetRenderMapRenderPassType = []() -> ECubeMapRenderPassType
    {
        const bool bUseVSInstancing =
            RHI::bSupportRenderTargetArrayIndexFromVertexShader &&
            CVarPointLightsEnableSinglePassRendering.GetValue();

        const bool bUseGSInstancing =
            !bUseVSInstancing &&
            RHI::bSupportsGeometryShaders &&
            CVarPointLightsEnableGeometryShaderInstancing.GetValue();

        if (bUseVSInstancing)
        {
            return ECubeMapRenderPassType::SinglePass;
        }
        else if (bUseGSInstancing)
        {
            return ECubeMapRenderPassType::GeometryShaderSinglePass;
        }
        else
        {
            return ECubeMapRenderPassType::MultiPass;
        }
    };

    RHI_EVENT_SCOPE(CommandList, "Render PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    const ECubeMapRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (RenderPassType == ECubeMapRenderPassType::SinglePass)
    {
        RecordInternal<ECubeMapRenderPassType::SinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
    {
        RecordInternal<ECubeMapRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::MultiPass)
    {
        RecordInternal<ECubeMapRenderPassType::MultiPass>(CommandList, Resources, Scene);
    }
}

template<ECubeMapRenderPassType RenderPassType>
void FPointLightRenderPass::RecordInternal(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const bool bBindless =
        RHI::bSupportsBindless &&
        GShadowsBindless &&
        Resources.MaterialDataBufferSRV.IsValid();

    const int32 MaxShadowCasters = static_cast<int32>(Resources.MaxPointLightShadows);

    TArray<FScenePointLight*> ShadowCasters;
    ShadowCasters.Reserve(MaxShadowCasters);

    for (FScenePointLight* ScenePointLight : Scene->GetPointLights())
    {
        if (ShadowCasters.Size() >= MaxShadowCasters)
        {
            break;
        }

        if (ScenePointLight && ScenePointLight->bCastShadows)
        {
            ShadowCasters.Add(ScenePointLight);
        }
    }

    const int32 NumPointLights = ShadowCasters.Size();

    constexpr bool bIsSinglePass = RenderPassType == ECubeMapRenderPassType::SinglePass || RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass;
    if constexpr (bIsSinglePass)
    {
        // Per light data
        FSinglePassPointLightBufferHLSL SinglePassPointLightBuffer;

        for (int32 LightIndex = 0; LightIndex < NumPointLights; ++LightIndex)
        {
            FScenePointLight* ScenePointLight = ShadowCasters[LightIndex];
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                const FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
                SinglePassPointLightBuffer.LightPosition               = Data.Position;
                SinglePassPointLightBuffer.LightFarPlane               = Data.FarPlane;
                SinglePassPointLightBuffer.LightProjections[FaceIndex] = Data.ViewProjMatrix;
            }

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(SinglePassShadowMapBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
            CommandList.UpdateBuffer(SinglePassShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FSinglePassPointLightBufferHLSL)), &SinglePassPointLightBuffer);
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(SinglePassShadowMapBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));

            FRHIDepthStencilView* DepthStencilView = Resources.PointLightShadowMapDSVs[LightIndex].Get();
            if (!DepthStencilView)
            {
                LOG_ERROR("PointLightShadows single-pass render requires PointLightShadowMapDSVs[%d], but it was not resolved", LightIndex);
                continue;
            }

            FRHIBeginRenderPassDesc RenderPassDesc;
            RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FDepthStencilValue(1.0f, 0));

            CommandList.BeginRenderPass(RenderPassDesc);

            const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
            FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : ScenePointLight->SinglePassShadowView.GetMeshBatches())
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

                if constexpr (RenderPassType == ECubeMapRenderPassType::SinglePass)
                {
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                }
                else
                {
                    CommandList.SetConstantBuffer(Instance->GeometryShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                }

                if (Instance->PixelShader)
                {
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), SinglePassShadowMapBuffer.Get(), 0);

                    if (bBindless)
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                    }
                    else
                    {
                        BindMaterialTextures(CommandList, Instance->PixelShader.Get(), Features, *Material, 0);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                    }
                }

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

                    if constexpr (RenderPassType == ECubeMapRenderPassType::SinglePass)
                    {
                        // One instance per face
                        constexpr uint32 SinglePassInstanceCount = 6;
                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                    }
                    else
                    {
                        // When using a geometry shader we just have a single instance
                        constexpr uint32 SinglePassInstanceCount = 1;
                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                    }
                }
            }

            CommandList.EndRenderPass();
        }
    }
    else
    {
        FPerShadowMapHLSL PerShadowMapData;

        for (int32 LightIndex = 0; LightIndex < NumPointLights; ++LightIndex)
        {
            FScenePointLight* ScenePointLight = ShadowCasters[LightIndex];
            for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
            {
                FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
                PerShadowMapData.Matrix   = Data.ViewProjMatrix;
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(PerShadowMapBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
                CommandList.UpdateBuffer(PerShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FPerShadowMapHLSL)), &PerShadowMapData);
                CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(PerShadowMapBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));

                const uint32 ArrayIndex = (LightIndex * RHI_NUM_CUBE_FACES) + FaceIndex;
                FRHIDepthStencilView* DepthStencilView = Resources.PointLightShadowMapFaceDSVs[ArrayIndex].Get();
                if (!DepthStencilView)
                {
                    LOG_ERROR("PointLightShadows multi-pass render requires PointLightShadowMapFaceDSVs[%u], but it was not resolved", ArrayIndex);
                    continue;
                }

                FRHIBeginRenderPassDesc RenderPassDesc;
                RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FDepthStencilValue(1.0f, 0));

                CommandList.BeginRenderPass(RenderPassDesc);

                const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
                FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                const TArray<FMeshBatch>& MeshBatches = ScenePointLight->ShadowView[FaceIndex].GetMeshBatches();
                for (const FMeshBatch& Batch : MeshBatches)
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
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerShadowMapBuffer.Get(), 0);

                    if (Instance->PixelShader)
                    {
                        CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                        if (bBindless)
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                        }
                        else
                        {
                            BindMaterialTextures(CommandList, Instance->PixelShader.Get(), Features, *Material, 0);
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 7);
                        }
                    }

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
}

void FPointLightRenderPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("PointLightShadows", ERenderGraphPassFlags::None, GShadowsEnabled && GPointLightShadowsEnabled,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            if (Context.PointLightShadowMaps)
            {
                PassBuilder.WriteTexture(Context.PointLightShadowMaps, ERHIResourceState::DepthWrite);
            }

            for (FRenderGraphDepthStencilView* FaceDepthStencilView : Context.PointLightShadowMapFaceDSVs)
            {
                if (FaceDepthStencilView)
                {
                    PassBuilder.UseDepthStencilView(FaceDepthStencilView, ERHIResourceState::DepthWrite, true);
                }
            }

            for (FRenderGraphDepthStencilView* LightDepthStencilView : Context.PointLightShadowMapDSVs)
            {
                if (LightDepthStencilView)
                {
                    PassBuilder.UseDepthStencilView(LightDepthStencilView, ERHIResourceState::DepthWrite, true);
                }
            }

            if (Context.PerShadowMapBuffer)
            {
                PassBuilder.ReadBuffer(Context.PerShadowMapBuffer, ERHIResourceState::ConstantBuffer);
            }

            if (Context.SinglePassShadowMapBuffer)
            {
                PassBuilder.ReadBuffer(Context.SinglePassShadowMapBuffer, ERHIResourceState::ConstantBuffer);
            }
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncPointLightShadowViews(PassResources, Context, *Context.FrameResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene);
        });
}
