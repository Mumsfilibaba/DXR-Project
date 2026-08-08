#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineState.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Renderer/MaterialBindless.h"
#include "Renderer/ShadowRendering.h"
#include "Renderer/ShadowSettings.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "RendererCore/VertexStreamCache.h"

static bool GShadowsBindless = false;
static FAutoConsoleVariableRef CVarShadowsBindless(
    "Renderer.Shadows.Bindless",
    "When true, point-light cube shadows and cascaded directional shadows sample the alpha-mask / parallax-height textures and material sampler through SM 6.6 ResourceDescriptorHeap[] / SamplerDescriptorHeap[] and a per-material indices buffer instead of conventional register bindings.",
    GShadowsBindless);

static TAutoConsoleVariable<bool> CVarPointLightsEnableSinglePassRendering(
    "Renderer.PointLights.EnableSinglePassRendering",
    "Enables instancing for cube-map rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightsEnableGeometryShaderInstancing(
    "Renderer.PointLights.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cube-map drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

bool GCSMDebugCascades = false;
static FAutoConsoleVariableRef CVarCSMDebugCascades(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    GCSMDebugCascades,
    EConsoleVariableFlags::Default);

bool GCSMStableCascades = true;
static FAutoConsoleVariableRef CVarCSMStableCascades(
    "Renderer.CSM.StableCascades",
    "Set to true to enable stable cascades when generating shadow cascade matrices",
    GCSMStableCascades,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableSinglePassRendering(
    "Renderer.CSM.EnableSinglePassRendering",
    "Enables instancing for cascade rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less overhead on the CPU",
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

static TAutoConsoleVariable<int32> CVarCSMFilterMode(
    "Renderer.CSM.FilterMode",
    "Select mode when filer Cascaded Shadow Maps. 0: Percentage Closer Filtering (PCF) 1: Percentage Closer Soft Shadows (PCSS)",
    0,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterFunction(
    "Renderer.CSM.FilterFunction",
    "Select function to use to filer Cascaded Shadow Maps. 0: Grid 1: Poisson Disk 2: Vogel Disk",
    1,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterSize(
    "Renderer.CSM.FilterSize",
    "Size of the filter for the Cascaded Shadow Maps",
    256,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMMaxFilterSize(
    "Renderer.CSM.MaxFilterSize",
    "Maximum size of the filter for the Cascaded Shadow Maps",
    512,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMNumPoissonDiscSamples(
    "Renderer.CSM.NumPoissonDiscSamples",
    "Number Poisson Samples to use when sampling the Cascaded Shadow Maps using a Poisson Disc",
    32,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMRotateSamples(
    "Renderer.CSM.RotateSamples",
    "Rotate Poisson samples before using them to sample the Cascades",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMBlendCascades(
    "Renderer.CSM.BlendCascades",
    "Blend between cascades",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMSelectCascadeFromProjection(
    "Renderer.CSM.SelectCascadeFromProjection",
    "Select what cascade to use based on projection",
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

    // Double-sidedness only picks a cull mode, so it keys the pipeline rather than the shader
    const uint32 PipelineFlags = Features.IsDoubleSided() ? PIPELINE_FLAG_DOUBLE_SIDED : 0u;

    const FGraphicsPipelineKey PSOKey(Permutation.GetPermutationID(), PipelineFlags, Batch.Declaration.GetID());

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
            const String DebugName = String::CreateFormatted("Point ShadowMap PipelineState%s %d",
                bBindless ? " [Bindless]" : "",
                MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        // Return the new instance
        FGraphicsPipelineStateInstance& NewInstance = MaterialPSOs.Add(PSOKey, Move(NewPipelineStateInstance));
        return &NewInstance;
    }
    else
    {
        // Return the existing instance
        return CachedPointLightPSO;
    }
}

bool FPointLightRenderPass::Initialize(FFrameResources& Resources)
{
    FRHIBufferDesc PerShadowMapBufferDesc;
    PerShadowMapBufferDesc.Stride = sizeof(FPerShadowMapHLSL);
    PerShadowMapBufferDesc.Size   = sizeof(FPerShadowMapHLSL);
    PerShadowMapBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

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

	FRHIBufferDesc SinglePassShadowMapBufferDesc;
    SinglePassShadowMapBufferDesc.Stride = sizeof(FSinglePassPointLightBufferHLSL);
    SinglePassShadowMapBufferDesc.Size   = sizeof(FSinglePassPointLightBufferHLSL);
    SinglePassShadowMapBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

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

    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::NoDefaultDSV;
    FRHITextureDesc PointLightDesc = FRHITextureDesc::CreateTextureCubeArray(RendererTextureFormats::ShadowMapFormat, Resources.PointLightShadowSize, Resources.MaxPointLightShadows, 1, 1, Flags, DepthClearValue);
    Resources.PointLightShadowMaps = RHI::CreateTexture(PointLightDesc, ERHIResourceState::PixelShaderResource);

    if (Resources.PointLightShadowMaps)
    {
        Resources.PointLightShadowMaps->SetDebugName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    Resources.PointLightShadowMapDSVs.Clear();
    Resources.PointLightShadowMapDSVs.Reserve(Resources.MaxPointLightShadows);

    Resources.PointLightShadowMapFaceDSVs.Clear();
    Resources.PointLightShadowMapFaceDSVs.Reserve(Resources.MaxPointLightShadows * RHI_NUM_CUBE_FACES);

    const EFormat ShadowMapFormat = Resources.PointLightShadowMaps->GetDesc().Format;
    for (uint32 LightIndex = 0; LightIndex < Resources.MaxPointLightShadows; ++LightIndex)
    {
        const FRHIDepthStencilViewDesc PerLightDSVDesc = FRHIDepthStencilViewDesc::CreateTextureCubeArray(
            ShadowMapFormat, 0, static_cast<uint16>(LightIndex), 1);

        FRHIDepthStencilViewRef PerLightDSV = RHI::CreateDepthStencilView(Resources.PointLightShadowMaps.Get(), PerLightDSVDesc);
        if (!PerLightDSV)
        {
            return false;
        }

        Resources.PointLightShadowMapDSVs.Add(PerLightDSV);

        for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
        {
            const FRHIDepthStencilViewDesc PerFaceDSVDesc = FRHIDepthStencilViewDesc::CreateTexture2DArray(
                ShadowMapFormat, 0, static_cast<uint16>((LightIndex * RHI_NUM_CUBE_FACES) + FaceIndex), 1);

            FRHIDepthStencilViewRef PerFaceDSV = RHI::CreateDepthStencilView(Resources.PointLightShadowMaps.Get(), PerFaceDSVDesc);
            if (!PerFaceDSV)
            {
                return false;
            }

            Resources.PointLightShadowMapFaceDSVs.Add(PerFaceDSV);
        }
    }

    return true;
}

void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const auto GetRenderMapRenderPassType = []() -> ECubeMapRenderPassType
    {
        const bool bUseVSInstancing = RHI::bSupportRenderTargetArrayIndexFromVertexShader && CVarPointLightsEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing = !bUseVSInstancing && RHI::bSupportsGeometryShaders && CVarPointLightsEnableGeometryShaderInstancing.GetValue();

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

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.PointLightShadowMaps.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::DepthWrite));

    const ECubeMapRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (RenderPassType == ECubeMapRenderPassType::SinglePass)
    {
        Execute<ECubeMapRenderPassType::SinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
    {
        Execute<ECubeMapRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::MultiPass)
    {
        Execute<ECubeMapRenderPassType::MultiPass>(CommandList, Resources, Scene);
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.PointLightShadowMaps.Get(), ERHIResourceState::DepthWrite, ERHIResourceState::NonPixelShaderResource));
}

template<ECubeMapRenderPassType RenderPassType>
void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const bool bBindless = RHI::bSupportsBindless && GShadowsBindless && Resources.MaterialDataBufferSRV.IsValid();

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
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 0);
                    }
                    else
                    {
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                        if (Features.HasAlphaMask())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
                        }

                        if (Features.HasHeightMap())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 2);
                        }
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
        // Per cube-face data
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
                    
                    // If we have a pixel-shader, bind all resources that shader needs
                    if (Instance->PixelShader)
                    {
                        CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                        if (bBindless)
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 0);
                        }
                        else
                        {
                            CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                            if (Features.HasAlphaMask())
                            {
                                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
                            }
                            if (Features.HasHeightMap())
                            {
                                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 2);
                            }
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

FCascadeGenerationPass::FCascadeGenerationPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CascadeGen()
    , CascadeGenShader()
{
}

FCascadeGenerationPass::~FCascadeGenerationPass()
{
    CascadeGen.Reset();
    CascadeGenShader.Reset();
}

bool FCascadeGenerationPass::Initialize(FFrameResources& Resources)
{
    CascadeGenShader = FShaderCache::Get().GetShader<FCascadeMatrixGenCS>();
    if (!CascadeGenShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc CascadeMatrixGenPSODesc;
    CascadeMatrixGenPSODesc.Shader = CascadeGenShader.Get();

    CascadeGen = RHI::CreateComputePipelineState(CascadeMatrixGenPSODesc);
    if (!CascadeGen)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CascadeGen->SetDebugName("CascadeGen PSO");
    }

	FRHIBufferDesc CascadeMatrixBufferDesc;
    CascadeMatrixBufferDesc.Stride = sizeof(FCascadeMatricesHLSL);
    CascadeMatrixBufferDesc.Size   = CascadeMatrixBufferDesc.Stride * NUM_SHADOW_CASCADES;
    CascadeMatrixBufferDesc.Flags  = EBufferFlags::RWBuffer | EBufferFlags::Default;

    Resources.CascadeMatrixBuffer = RHI::CreateBuffer(CascadeMatrixBufferDesc, ERHIResourceState::UnorderedAccess, nullptr);
    if (!Resources.CascadeMatrixBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
    }

    FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferSRV = RHI::CreateShaderResourceView(Resources.CascadeMatrixBuffer.Get(), SRVDesc);
    if (!Resources.CascadeMatrixBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferUAV = RHI::CreateUnorderedAccessView(Resources.CascadeMatrixBuffer.Get(), UAVDesc);
    if (!Resources.CascadeMatrixBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

	FRHIBufferDesc CascadeSplitsBufferDesc;
    CascadeSplitsBufferDesc.Stride = sizeof(FCascadeSplitHLSL);
    CascadeSplitsBufferDesc.Size   = CascadeSplitsBufferDesc.Stride * NUM_SHADOW_CASCADES;
    CascadeSplitsBufferDesc.Flags  = EBufferFlags::RWBuffer | EBufferFlags::Default;

    Resources.CascadeSplitsBuffer = RHI::CreateBuffer(CascadeSplitsBufferDesc, ERHIResourceState::UnorderedAccess, nullptr);
    if (!Resources.CascadeSplitsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
    }

    SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferSRV = RHI::CreateShaderResourceView(Resources.CascadeSplitsBuffer.Get(), SRVDesc);
    if (!Resources.CascadeSplitsBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    UAVDesc = FRHIUnorderedAccessViewDesc::CreateBuffer(0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferUAV = RHI::CreateUnorderedAccessView(Resources.CascadeSplitsBuffer.Get(), UAVDesc);
    if (!Resources.CascadeSplitsBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FCascadeGenerationPass::Execute(FRHICommandList& CommandList, FFrameResources& Resources)
{
    GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

    const FRHITransitionBarrierDesc CascadeBuffersToWrite[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(Resources.CascadeMatrixBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
        FRHITransitionBarrierDesc::CreateBuffer(Resources.CascadeSplitsBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess),
    };

    CommandList.TransitionBarrier(CascadeBuffersToWrite);

    CommandList.SetComputePipelineState(CascadeGen.Get());

    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 1);

    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeMatrixBufferUAV.Get(), 0);
    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeSplitsBufferUAV.Get(), 1);

    CommandList.SetShaderResourceView(CascadeGenShader.Get(), Resources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

    CommandList.Dispatch(1, 1, 1);

    const FRHITransitionBarrierDesc CascadeBuffersToRead[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(Resources.CascadeMatrixBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateBuffer(Resources.CascadeSplitsBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource),
    };

    CommandList.TransitionBarrier(CascadeBuffersToRead);
}

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
        (Features.IsDoubleSided() ? PIPELINE_FLAG_DOUBLE_SIDED : 0u) | (bEnableDepthClipping ? PIPELINE_FLAG_DEPTH_CLIPPING : 0u);

    const FCascadeShadowVS::FPermutation Permutation = FCascadeShadowRules::Create(Features, bBindless, RenderPassType);
    const FGraphicsPipelineKey           PSOKey(Permutation.GetPermutationID(), PipelineFlags, Batch.Declaration.GetID());

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

        // Only materials that can discard fragments need a PixelShader, the rest render depth-only
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
            const String DebugName = String::CreateFormatted("CSM PipelineState%s %d", bBindless ? " [Bindless]" : "", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        // Return the new instance
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
	FRHIBufferDesc PerCascadeBufferDesc;
    PerCascadeBufferDesc.Stride = sizeof(FPerCascadeHLSL);
    PerCascadeBufferDesc.Size   = sizeof(FPerCascadeHLSL);
    PerCascadeBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

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
    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::NoDefaultDSV;

    const FClearValue DepthClearValue(RendererTextureFormats::ShadowMapFormat, 1.0f, 0);
    FRHITextureDesc CascadeDesc = FRHITextureDesc::CreateTexture2DArray(RendererTextureFormats::ShadowMapFormat, Resources.CascadeSize, Resources.CascadeSize, NUM_SHADOW_CASCADES, 1, 1, Flags, DepthClearValue);
    Resources.ShadowCascades = RHI::CreateTexture(CascadeDesc, ERHIResourceState::NonPixelShaderResource);

    if (Resources.ShadowCascades)
    {
        const String DebugName = String::CreateFormatted("Shadow Map Cascades");
        Resources.ShadowCascades->SetDebugName(DebugName);
    }
    else
    {
        DEBUG_BREAK();
        return false;
    }

    for (uint16 Index = 0; Index < NUM_SHADOW_CASCADES; Index++)
    {
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2DArray(
            Resources.ShadowCascades->GetDesc().Format, 0, 1, Index, 1);

        Resources.ShadowCascadesSRVs[Index] = RHI::CreateShaderResourceView(Resources.ShadowCascades.Get(), SRVDesc);
        if (!Resources.ShadowCascadesSRVs[Index])
        {
            DEBUG_BREAK();
            return false;
        }
    }

    // Pre-create the combined cascade DSV (covers all cascades in one pass) and per-cascade DSVs
    // so they are reused across frames instead of being recreated each time.
    {
        const FRHIDepthStencilViewDesc CombinedDSVDesc = FRHIDepthStencilViewDesc::CreateTexture2DArray(
            Resources.ShadowCascades->GetDesc().Format, 0, 0, NUM_SHADOW_CASCADES);

        Resources.ShadowCascadesCombinedDSV = RHI::CreateDepthStencilView(Resources.ShadowCascades.Get(), CombinedDSVDesc);
        if (!Resources.ShadowCascadesCombinedDSV)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    for (uint16 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
    {
        const FRHIDepthStencilViewDesc PerCascadeDSVDesc = FRHIDepthStencilViewDesc::CreateTexture2DArray(
            Resources.ShadowCascades->GetDesc().Format, 0, Index, 1);

        Resources.ShadowCascadePerCascadeDSVs[Index] = RHI::CreateDepthStencilView(Resources.ShadowCascades.Get(), PerCascadeDSVDesc);
        if (!Resources.ShadowCascadePerCascadeDSVs[Index])
        {
            DEBUG_BREAK();
            return false;
        }
    }

    return true;
}

void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const auto GetRenderMapRenderPassType = []() -> ECascadeRenderPassType
    {
        constexpr uint32 MinViewInstanceCount = 4;

        const bool bUseVSInstancing   = RHI::bSupportRenderTargetArrayIndexFromVertexShader && CVarCSMEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing   = !bUseVSInstancing && RHI::bSupportsGeometryShaders && CVarCSMEnableGeometryShaderInstancing.GetValue();
        const bool bUseViewInstancing = !bUseGSInstancing && RHI::bSupportsViewInstancing && RHI::MaxViewInstanceCount >= MinViewInstanceCount && CVarCSMEnableViewInstancing.GetValue();

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
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.ShadowCascades.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::DepthWrite));

        if (RenderPassType == ECascadeRenderPassType::SinglePass)
        {
            Execute<ECascadeRenderPassType::SinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            Execute<ECascadeRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            Execute<ECascadeRenderPassType::ViewInstancingSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::MultiPass)
        {
            Execute<ECascadeRenderPassType::MultiPass>(CommandList, Resources, Scene);
        }

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.ShadowCascades.Get(), ERHIResourceState::DepthWrite, ERHIResourceState::NonPixelShaderResource));
    }
}

template<ECascadeRenderPassType RenderPassType>
void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const bool bBindless = RHI::bSupportsBindless && GShadowsBindless && Resources.MaterialDataBufferSRV.IsValid();

    constexpr bool bIsSinglePass = 
        RenderPassType == ECascadeRenderPassType::SinglePass ||
        RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass ||
        RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass;

    FSceneDirectionalLight* SceneDirectionalLight = Scene->GetDirectionalLight();
    if constexpr (bIsSinglePass)
    {
        FRHIDepthStencilView* DepthStencilView = Resources.ShadowCascadesCombinedDSV.Get();

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
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 1);
                }
                else
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                    if (Features.HasAlphaMask())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
                    }
                    if (Features.HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 2);
                    }
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
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 1);
                    }
                    else
                    {
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                        if (Features.HasAlphaMask())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
                        }
                        if (Features.HasHeightMap())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Resources.MaterialDataBufferSRV.Get(), 2);
                        }
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

FShadowMaskRenderPass::FShadowMaskRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , PipelineStates()
    , ShadowSettingsBuffer(nullptr)
{
}

FShadowMaskRenderPass::~FShadowMaskRenderPass()
{
}

bool FShadowMaskRenderPass::Initialize(FFrameResources& Resources)
{
    if (!CreateResources(Resources, Resources.CurrentRenderWidth, Resources.CurrentRenderHeight))
    {
        return false;
    }

    FComputePipelineStateInstance Instance;
    if (!RetrievePipelineState(CreateCurrentPermutation(), Instance))
    {
        return false;
    }

    if (!Instance.Shader)
    {
        return false;
    }

    if (!Instance.PipelineState)
    {
        return false;
    }

	FRHIBufferDesc SettingsBufferDesc;
    SettingsBufferDesc.Stride = sizeof(FDirectionalShadowSettingsHLSL);
    SettingsBufferDesc.Size   = sizeof(FDirectionalShadowSettingsHLSL);
    SettingsBufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

    ShadowSettingsBuffer = RHI::CreateBuffer(SettingsBufferDesc, ERHIResourceState::ConstantBuffer);
    if (!ShadowSettingsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadowSettingsBuffer->SetDebugName("ShadowSettingsBuffer");
    }

    return true;
}

bool FShadowMaskRenderPass::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;

    FRHITextureDesc ShadowMaskDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::ShadowMaskFormat, Width, Height, 1, 1, Flags);
    Resources.DirectionalShadowMask = RHI::CreateTexture(ShadowMaskDesc, ERHIResourceState::NonPixelShaderResource);

    if (Resources.DirectionalShadowMask)
    {
        Resources.DirectionalShadowMask->SetDebugName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureDesc CascadeIndexBufferDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, Flags);
    Resources.CascadeIndexBuffer = RHI::CreateTexture(CascadeIndexBufferDesc, ERHIResourceState::NonPixelShaderResource);

    if (Resources.CascadeIndexBuffer)
    {
        Resources.CascadeIndexBuffer->SetDebugName("Cascade Index Debug Buffer");
    }
    else
    {
        return false;
    }

    return true;
}

void FShadowMaskRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, bool bForceDebugMode)
{
    if (Resources.DirectionalShadowMask->GetDesc().Extent.X == 0 || Resources.DirectionalShadowMask->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Render ShadowMasks");

    TRACE_SCOPE("Render ShadowMasks");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

    FDirectionalShadowSettingsHLSL ShadowSettings;
    Memory::Memzero(&ShadowSettings);

    ShadowSettings.FilterSize    = Math::Max<float>(static_cast<float>(CVarCSMFilterSize.GetValue()), 1.0f);
    ShadowSettings.MaxFilterSize = Math::Max<float>(static_cast<float>(CVarCSMMaxFilterSize.GetValue()), 1.0f);
    ShadowSettings.ShadowMapSize = Resources.ShadowCascades->GetDesc().Extent.X;
    ShadowSettings.FrameIndex    = GetRenderer()->GetFrameCounter().GetFrameIndex();
    ShadowSettings.NumSamples    = Math::Clamp<uint32>(CVarCSMNumPoissonDiscSamples.GetValue(), 4, 128);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(ShadowSettingsBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
    CommandList.UpdateBuffer(ShadowSettingsBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalShadowSettingsHLSL)), &ShadowSettings);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(ShadowSettingsBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.DirectionalShadowMask.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));

    FShadowMaskCS::FPermutation Permutation = CreateCurrentPermutation();

    const bool bDebugMode = Permutation.Get<FCSMDebug>() || bForceDebugMode;
    Permutation.Set<FCSMDebug>(bDebugMode);

    FComputePipelineStateInstance PipelineStateInstance;
    if (!RetrievePipelineState(Permutation, PipelineStateInstance))
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.DirectionalShadowMask.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
        DEBUG_BREAK();
        return;
    }

    if (bDebugMode)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.CascadeIndexBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));
    }

    CommandList.SetComputePipelineState(PipelineStateInstance.PipelineState.Get());

    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.DirectionalLightDataBuffer.Get(), 1);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), ShadowSettingsBuffer.Get(), 2);

    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeSplitsBufferSRV.Get(), 1);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.ShadowCascades->GetShaderResourceView(), 4);

    CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.DirectionalShadowMask->GetUnorderedAccessView(), 0);

    if (bDebugMode)
    {
        CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
    }

    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerPointCmp.Get(), 0);
    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerLinearCmp.Get(), 1);
    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerPoint.Get(), 2);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = Math::DivideByMultiple(Resources.DirectionalShadowMask->GetDesc().Extent.X, NumThreads);
    const uint32 ThreadsY = Math::DivideByMultiple(Resources.DirectionalShadowMask->GetDesc().Extent.Y, NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.DirectionalShadowMask.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
    if (bDebugMode)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.CascadeIndexBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
    }
}

bool FShadowMaskRenderPass::RetrievePipelineState(const FShadowMaskCS::FPermutation& Permutation, FComputePipelineStateInstance& OutPSO)
{
    const int32 PermutationID = FShadowMaskCS::RemapPermutation(Permutation).GetPermutationID();
    if (FComputePipelineStateInstance* PipelineState = PipelineStates.Find(PermutationID))
    {
        OutPSO = *PipelineState;
        return true;
    }

    FComputePipelineStateInstance PipelineStateInstance;
    PipelineStateInstance.Shader = FShaderCache::Get().GetShader<FShadowMaskCS>(Permutation);

    if (!PipelineStateInstance.Shader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc ShadowMaskGenPSODesc;
    ShadowMaskGenPSODesc.Shader = PipelineStateInstance.Shader.Get();

    PipelineStateInstance.PipelineState = RHI::CreateComputePipelineState(ShadowMaskGenPSODesc);
    if (!PipelineStateInstance.PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineStateInstance.PipelineState->SetDebugName(String::CreateFormatted("ShadowMask PSO (Permutation %d)", PermutationID));
    }

    PipelineStates.Add(PermutationID, PipelineStateInstance);
    OutPSO = PipelineStateInstance;
    return true;
}

FShadowMaskCS::FPermutation FShadowMaskRenderPass::CreateCurrentPermutation()
{
    const ECSMFilterFunction FilterFunction = static_cast<ECSMFilterFunction>(Math::Clamp<int32>(CVarCSMFilterFunction.GetValue(), 0, static_cast<int32>(ECSMFilterFunction::Count) - 1));

    FShadowMaskCS::FPermutation Permutation;
    Permutation.Set<FCSMFilterModeDim>(static_cast<ECSMFilterMode>(Math::Clamp<int32>(CVarCSMFilterMode.GetValue(), 0, static_cast<int32>(ECSMFilterMode::Count) - 1)));
    Permutation.Set<FCSMFilterFunctionDim>(FilterFunction);
    Permutation.Set<FCSMDebug>(GCSMDebugCascades);
    Permutation.Set<FCSMBlendCascades>(CVarCSMBlendCascades.GetValue());
    Permutation.Set<FCSMCascadeFromProjection>(CVarCSMSelectCascadeFromProjection.GetValue());
    Permutation.Set<FCSMRotateSamples>(CVarCSMRotateSamples.GetValue());
    Permutation.Set<FCSMNumSamples>(FCSMNumSamples::FromSampleCount(CVarCSMNumPoissonDiscSamples.GetValue()));

    return FShadowMaskCS::RemapPermutation(Permutation);
}
