#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Renderer/Settings/ReflectionSettings.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneSkyLight.h"
#include "Renderer/Settings/ShadowSettings.h"
#include "Renderer/Passes/TiledLightPass.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static bool GDrawTileDebug = false;
static FAutoConsoleVariableRef CVarDrawTileDebug(
    "Renderer.Debug.DrawTiledLightning",
    "Draws the tiled lightning overlay, that displays how many lights are used in a certain tile",
    GDrawTileDebug);

float GIndirectSpecularStrength = 1.0f;
static FAutoConsoleVariableRef CVarIndirectSpecularStrength(
    "Renderer.Reflections.IndirectSpecularStrength",
    "Scalar multiplier applied to the indirect specular (IBL / ray-traced reflection) contribution in the deferred light pass.",
    GIndirectSpecularStrength);

class FBRDFIntegrationCS
{
    DECLARE_SHADER_TYPE(FBRDFIntegrationCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBRDFIntegrationCS, "Shaders/BRDFIntegationGen.hlsl", "Main", EShaderModel::SM_6_2);

enum class ETiledLightDebugMode : uint8
{
    None    = 0,
    Tiles   = 1,
    Cascade = 2,

    Count,
};

class FTiledLightDebug : SHADER_PERMUTATION_ENUM("TILED_LIGHT_DEBUG_MODE", ETiledLightDebugMode);

struct FLightPassSettingsHLSL
{
    // 0-16
    int32 NumPointLights;
    int32 NumShadowCastingPointLights;
    int32 NumSkyLightMips;
    int32 NumLightProbes;

    // 16-32
    int32 ScreenWidth;
    int32 ScreenHeight;
    int32 bEnablePointLightShadows;
    int32 bEnableRayTracingReflections;

    // 32-48
    float IndirectSpecularStrength;
};

class FDeferredLightPassCS
{
    DECLARE_SHADER_TYPE(FDeferredLightPassCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<FTiledLightDebug>;
};

IMPLEMENT_SHADER_TYPE(FDeferredLightPassCS, "Shaders/DeferredLightPass.hlsl", "Main", EShaderModel::SM_6_2);

FTiledLightPass::FTiledLightPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TiledLightPassPSO(nullptr)
    , TiledLightShader(nullptr)
    , TiledLightPassPSO_TileDebug(nullptr)
    , TiledLightShader_TileDebug(nullptr)
    , TiledLightPassPSO_CascadeDebug(nullptr)
    , TiledLightShader_CascadeDebug(nullptr)
{
}

FTiledLightPass::~FTiledLightPass()
{
    TiledLightPassPSO.Reset();
    TiledLightShader.Reset();
    TiledLightPassPSO_TileDebug.Reset();
    TiledLightShader_TileDebug.Reset();
    TiledLightPassPSO_CascadeDebug.Reset();
    TiledLightShader_CascadeDebug.Reset();
}

bool FTiledLightPass::Initialize(FFrameResources& FrameResources)
{
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.GBufferSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!FrameResources.GBufferSampler)
    {
        return false;
    }

    constexpr uint32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;

    if (!RHI::Device->QueryUAVFormatSupport(LUTFormat))
    {
        LOG_ERROR("[FSceneRenderer]: R16G16_Float is not supported for UAVs");
        return false;
    }

    FRHITextureDesc LUTDesc = FRHITextureDesc::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef StagingTexture = RHI::CreateTexture(LUTDesc, ERHIResourceState::Common);

    if (!StagingTexture)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        StagingTexture->SetDebugName("Staging IntegrationLUT");
    }

    LUTDesc.UsageFlags = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest;

    FrameResources.IntegrationLUT = RHI::CreateTexture(LUTDesc, ERHIResourceState::Common);
    if (!FrameResources.IntegrationLUT)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetDebugName("IntegrationLUT");
    }

    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!FrameResources.IntegrationLUTSampler)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputeShaderRef BRDFShader = FShaderCache::Get().GetShader<FBRDFIntegrationCS>();
    if (!BRDFShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = BRDFShader.Get();

    FRHIComputePipelineStateRef BRDFPipelineState = RHI::CreateComputePipelineState(PSODesc);
    if (!BRDFPipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BRDFPipelineState->SetDebugName("BRDFIntegationGen PipelineState");
    }

    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));

    CommandList.SetComputePipelineState(BRDFPipelineState.Get());

    FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(BRDFShader.Get(), StagingUAV, 0);

    constexpr uint32 ThreadCount    = 16;
    constexpr uint32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    constexpr uint32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);

    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CommandList.UnorderedAccessBarrier(StagingTexture.Get());

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.IntegrationLUT.Get(), ERHIResourceState::Common, ERHIResourceState::CopyDest));

    CommandList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        FrameResources.IntegrationLUT.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    FDeferredLightPassCS::FPermutation LightPassPermutation;
    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::None);
    TiledLightShader = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);

    if (!TiledLightShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc DeferredLightPassPSODesc;
    DeferredLightPassPSODesc.Shader = TiledLightShader.Get();

    TiledLightPassPSO = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO->SetDebugName("DeferredLightPass PipelineState");
    }

    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::Tiles);
    TiledLightShader_TileDebug = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);

    if (!TiledLightShader_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassPSODesc.Shader = TiledLightShader_TileDebug.Get();
    TiledLightPassPSO_TileDebug = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);

    if (!TiledLightPassPSO_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_TileDebug->SetDebugName("DeferredLightPass PipelineState Tile-Debug");
    }

    LightPassPermutation.Set<FTiledLightDebug>(ETiledLightDebugMode::Cascade);
    TiledLightShader_CascadeDebug = FShaderCache::Get().GetShader<FDeferredLightPassCS>(LightPassPermutation);

    if (!TiledLightShader_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassPSODesc.Shader = TiledLightShader_CascadeDebug.Get();
    TiledLightPassPSO_CascadeDebug = RHI::CreateComputePipelineState(DeferredLightPassPSODesc);

    if (!TiledLightPassPSO_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_CascadeDebug->SetDebugName("DeferredLightPass PipelineState Cascade-Debug");
    }

    return true;
}

void FTiledLightPass::Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    if (FrameResources.CurrentRenderWidth == 0 || FrameResources.CurrentRenderHeight == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "LightPass");

    TRACE_SCOPE("LightPass");

    const bool bDrawCascades = GCSMDebugCascades;

    FRHIComputeShader* LightPassShader;
    if (GDrawTileDebug)
    {
        LightPassShader = TiledLightShader_TileDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_TileDebug.Get());
    }
    else if (bDrawCascades)
    {
        LightPassShader = TiledLightShader_CascadeDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_CascadeDebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 3);

    const bool bUseRayTracingReflections =
        RHI::bSupportsRayTracing &&
        GRayTracingEnabled &&
        GReflectionsEnabled &&
        FrameResources.RayTracingOutput;

    if (bUseRayTracingReflections)
    {
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.RayTracingOutput->GetShaderResourceView(), 4);
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 5);

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->DiffuseCubeMap->GetShaderResourceView(), 6);
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->SpecularCubeMap->GetShaderResourceView(), 7);
        }

        if (!Scene->GetLightProbes().IsEmpty())
        {
            // TODO: Support more than the first probe
            if (FSceneLightProbe* LightProbe = Scene->GetLightProbes().First())
            {
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->DiffuseCubeMap->GetShaderResourceView(), 8);
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->SpecularCubeMap->GetShaderResourceView(), 9);
            }
        }
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.DirectionalShadowMask->GetShaderResourceView(), 10);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 11);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 12);

    if (bDrawCascades)
    {
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 13);
    }

    CommandList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.LightProbeBuffer.Get(), 6);

    CommandList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CommandList.SetSamplerState(LightPassShader, FrameResources.LightProbeSampler.Get(), 1);
    CommandList.SetSamplerState(LightPassShader, FrameResources.GBufferSampler.Get(), 2);
    CommandList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* SceneTargetUAV = FrameResources.SceneTarget->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(LightPassShader, SceneTargetUAV, 0);

    FLightPassSettingsHLSL LightPassSettings;

    const int32 RenderWidth  = FrameResources.CurrentRenderWidth;
    const int32 RenderHeight = FrameResources.CurrentRenderHeight;

    LightPassSettings.NumSkyLightMips              = 0;
    LightPassSettings.NumShadowCastingPointLights  = FrameResources.ShadowCastingPointLightsData.Size();
    LightPassSettings.NumPointLights               = FrameResources.PointLightsData.Size();
    LightPassSettings.NumLightProbes               = FrameResources.LightProbeInfos.Size();
    LightPassSettings.ScreenWidth                  = static_cast<int32>(RenderWidth);
    LightPassSettings.ScreenHeight                 = static_cast<int32>(RenderHeight);
    LightPassSettings.bEnableRayTracingReflections = bUseRayTracingReflections ? 1 : 0;
    LightPassSettings.IndirectSpecularStrength     = GIndirectSpecularStrength;

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            LightPassSettings.NumSkyLightMips = SkyLight->SpecularCubeMap->GetDesc().NumMipLevels;
        }
    }

    LightPassSettings.bEnablePointLightShadows = (GPointLightShadowsEnabled && GShadowsEnabled) ? 1 : 0;

    constexpr uint32 NumConstants = sizeof(FLightPassSettingsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(LightPassShader, &LightPassSettings, NumConstants);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenHeight, NumThreads);
    CommandList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);
}

void FTiledLightPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("Lighting", ERenderGraphPassFlags::Compute, true,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferAlbedo, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferMaterial, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.DirectionalShadowMask, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.PointLightShadowMaps, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.SSAOBuffer, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.CascadeIndexBuffer, ERHIResourceState::NonPixelShaderResource);

            if (Context.RayTracingOutput)
            {
                PassBuilder.ReadTexture(Context.RayTracingOutput, ERHIResourceState::NonPixelShaderResource);
            }

            if (Context.IntegrationLUT)
            {
                PassBuilder.ReadTexture(Context.IntegrationLUT, ERHIResourceState::NonPixelShaderResource);
            }

            PassBuilder.WriteTexture(Context.SceneTarget, ERHIResourceState::UnorderedAccess);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene);
        });
}
