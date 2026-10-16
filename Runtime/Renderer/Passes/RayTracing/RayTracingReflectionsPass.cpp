#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Paths.h"
#include "RHI/RHI.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Resources/Texture.h"
#include "Renderer/Shaders/MaterialBindless.h"
#include "Renderer/Shaders/RayTracingBindless.h"
#include "Renderer/Settings/ReflectionSettings.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RayTracing/RayTracingReflectionsPass.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneSkyLight.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static bool GRayTracingEnableLocalShaderBindings = true;
static FAutoConsoleVariableRef CVarRayTracingEnableLocalShaderBindings(
    TEXT("Renderer.RayTracing.EnableLocalShaderBindings"),
    TEXT("When true (default), ray tracing reflections carry per-hit-group resources via local-root-signature SBT records + explicit global "
    "bindings. When false they use the SM 6.6 bindless descriptor heap. Inverse of the old Renderer.RayTracing.Bindless. Forced to "
    "bindless on backends without SBT descriptor support."),
    GRayTracingEnableLocalShaderBindings);

static bool GRayTracingInlineReflections = false;
static FAutoConsoleVariableRef CVarRayTracingInlineReflections(
    TEXT("Renderer.RayTracing.InlineReflections"),
    TEXT("When true, reflections are produced by an inline RayQuery compute pass (TraceRayInline) instead of DispatchRays. Requires inline ray "
    "tracing support. Reuses the bindless geometry table."),
    GRayTracingInlineReflections);

static bool GRayTracingSER = false;
static FAutoConsoleVariableRef CVarRayTracingSER(
    TEXT("Renderer.RayTracing.SER"),
    TEXT("When true, reflections use a Shader-Execution-Reordering RayGen (HitObject/MaybeReorderThread, SM 6.9) over the bindless path. "
    "Requires SER support. No-op reorder on API-only devices."),
    GRayTracingSER);

bool GReflectionsEnabled = true;
static FAutoConsoleVariableRef CVarReflectionsEnabled(
    TEXT("Renderer.RayTracing.Reflections.Enable"),
    TEXT("Enables ray-traced reflections. Acceleration structures are still built while this is off, so other ray traced effects keep working "
    "and re-enabling does not rebuild the scene."),
    GReflectionsEnabled);

static float GReflectionMaxRayDistance = 10000.0f;
static FAutoConsoleVariableRef CVarReflectionMaxRayDistance(
    TEXT("Renderer.RayTracing.Reflections.MaxRayDistance"),
    TEXT("Maximum distance (TMax) traced by a reflection ray. Shorter distances trade far-field reflections for traversal cost."),
    GReflectionMaxRayDistance);

float GReflectionMirrorRoughnessThreshold = 0.05f;
static FAutoConsoleVariableRef CVarReflectionMirrorRoughnessThreshold(
    TEXT("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold"),
    TEXT("Surfaces below this roughness trace a perfect mirror ray; above it the direction is GGX importance-sampled."),
    GReflectionMirrorRoughnessThreshold);

static float GReflectionRayBias = 0.02f;
static FAutoConsoleVariableRef CVarReflectionRayBias(
    TEXT("Renderer.RayTracing.Reflections.RayBias"),
    TEXT("Distance the reflection ray origin is pushed along the surface normal to avoid self-intersection."),
    GReflectionRayBias);

struct EReflectionSampler
{
    enum Type : int32
    {
        White     = 0,
        Halton    = 1,
        BlueNoise = 2,

        Min = White,
        Max = BlueNoise,
    };
};

static int32 GReflectionSampler = EReflectionSampler::BlueNoise;
static FAutoConsoleVariableRef CVarReflectionSampler(
    TEXT("Renderer.RayTracing.Reflections.Sampler"),
    TEXT("Sequence used to importance-sample the GGX lobe for reflection rays: 0 = white noise, 1 = Halton, 2 = blue noise. Falls back to white "
    "noise when the blue noise mask failed to load."),
    GReflectionSampler);

FRayTracingReflectionsPass::FRayTracingReflectionsPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CurrentSERHitGroupCapacity(0)
    , CurrentHitGroupCapacity(0)
    , CurrentBindlessHitGroupCapacity(0)
    , ReflectionNoiseSize(0)
#if STATS_ENABLED
    , AlphaTestFrame(0)
#endif
{
}

FRayTracingReflectionsPass::~FRayTracingReflectionsPass()
{
    Release();
}

FRayTracingVariant FRayTracingReflectionsPass::CreateVariant(const FRayTracingPermutation& Permutation)
{
    FRHIRayGenShaderRef        RayGenShader     = FShaderCache::Get().GetShader<FRayGenShader>(Permutation);
    FRHIRayMissShaderRef       MissShader       = FShaderCache::Get().GetShader<FRayMissShader>(Permutation);
    FRHIRayClosestHitShaderRef ClosestHitShader = FShaderCache::Get().GetShader<FRayClosestHitShader>(Permutation);
    FRHIRayAnyHitShaderRef     AnyHitShader     = FShaderCache::Get().GetShader<FRayAnyHitShader>(Permutation);

    if (!RayGenShader || !MissShader || !ClosestHitShader || !AnyHitShader)
    {
        return FRayTracingVariant();
    }

    FRHIRayTracingPipelineStateDesc PSODesc;
    PSODesc.RayGenShaders =
    {
        RayGenShader.Get()
    };

    PSODesc.MissShaders =
    {
        MissShader.Get()
    };

    PSODesc.HitGroups =
    {
        FRHIRayTracingHitGroupInfo(TEXT("HitGroup"), ERayTracingHitGroupType::Triangles, { ClosestHitShader.Get() }),
        FRHIRayTracingHitGroupInfo(TEXT("AlphaTestedHitGroup"), ERayTracingHitGroupType::Triangles, { ClosestHitShader.Get(), AnyHitShader.Get() })
    };

    PSODesc.MaxRecursionDepth       = 1;
    PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
    PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);
    PSODesc.Flags                   = Permutation.Get<FRayTracingSER>()
        ? ERayTracingPipelineFlags::AllowShaderExecutionReordering
        : ERayTracingPipelineFlags::None;

    if (RHI::bSupportsOpacityMicromap)
    {
        PSODesc.Flags |= ERayTracingPipelineFlags::AllowOpacityMicromap;
    }

    FRayTracingVariant Variant;
    Variant.Pipeline = RHI::CreateRayTracingPipelineState(PSODesc);

    if (!Variant.Pipeline)
    {
        return FRayTracingVariant();
    }

    Variant.RayGenShader = RayGenShader;
    return Variant;
}

bool FRayTracingReflectionsPass::Initialize(FFrameResources& Resources)
{
    if (RHI::bSupportsRayTracingPipeline)
    {
        if (RHI::bSupportsShaderBindingTableDescriptors)
        {
            FRayTracingPermutation Permutation;
            Permutation.Set<FBindless>(false);
            Permutation.Set<FRayTracingSER>(false);

            LocalVariant = CreateVariant(Permutation);
            if (!LocalVariant)
            {
                LOG_WARNING("[RayTracingReflections]: Explicit ray tracing variant unavailable. Only the bindless path will be used");
            }
        }

        if (RHI::bSupportsBindless)
        {
            FRayTracingPermutation Permutation;
            Permutation.Set<FBindless>(true);
            Permutation.Set<FRayTracingSER>(false);

            BindlessVariant = CreateVariant(Permutation);
            if (!BindlessVariant)
            {
                LOG_WARNING("[RayTracingReflections]: Bindless ray tracing variant unavailable. Only the explicit-binding path will be used");
            }
        }

        if (!LocalVariant && !BindlessVariant)
        {
            LOG_WARNING("[RayTracingReflections]: No usable ray tracing pipeline (neither explicit nor bindless could be created). Ray traced reflections are disabled");
        }

        if (BindlessVariant)
        {
            FRayTracingPermutation Permutation;
            Permutation.Set<FBindless>(true);
            Permutation.Set<FRayTracingSER>(true);

            SERVariant = CreateVariant(Permutation);
            if (!SERVariant && RHI::bSupportsShaderExecutionReordering)
            {
                LOG_WARNING("[RayTracingReflections]: SER ray tracing variant unavailable. The SER path will be disabled");
            }
        }
    }

    {
        const FRHIBufferDesc ConstantsDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FRayTracingSceneConstantsHLSL));
        Resources.RayTracingSceneConstantsBuffer = RHI::CreateBuffer(ConstantsDesc, ERHIResourceState::ConstantBuffer, nullptr);
        if (!Resources.RayTracingSceneConstantsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }

        Resources.RayTracingSceneConstantsBuffer->SetDebugName(TEXT("RayTracing Scene Constants"));
    }

    if (RHI::bSupportsInlineRayTracing && RHI::bSupportsBindless)
    {
        InlineReflectionsShader = FShaderCache::Get().GetShader<FInlineReflectionsCS>();
        if (InlineReflectionsShader)
        {
            FRHIComputePipelineStateDesc PSODesc;
            PSODesc.Shader = InlineReflectionsShader.Get();

            InlineReflectionsPipeline = RHI::CreateComputePipelineState(PSODesc);
            if (InlineReflectionsPipeline)
            {
                InlineReflectionsPipeline->SetDebugName(TEXT("Inline RT Reflections PSO"));
            }
            else
            {
                InlineReflectionsShader.Reset();
            }
        }

        if (!InlineReflectionsPipeline)
        {
            LOG_WARNING("[RayTracingReflections]: Inline ray tracing reflections pipeline unavailable. The inline path will be disabled");
        }
    }

#if STATS_ENABLED
    {
        const FRHIBufferDesc CounterDesc = FRHIBufferDesc::CreateStructuredBuffer(sizeof(uint32), 1, EBufferFlags::Default | EBufferFlags::UnorderedAccessBuffer | EBufferFlags::CopySource | EBufferFlags::CopyDest);
        AlphaTestCounter = RHI::CreateBuffer(CounterDesc, ERHIResourceState::UnorderedAccess, nullptr);
        if (!AlphaTestCounter)
        {
            DEBUG_BREAK();
            return false;
        }

        AlphaTestCounter->SetDebugName(TEXT("RayTracing Alpha Test Counter"));
        AlphaTestCounterUAV = RHI::CreateUnorderedAccessView(AlphaTestCounter.Get(), FRHIUnorderedAccessViewDesc::CreateBuffer(0, 1));

        for (FRHIBufferRef& Readback : AlphaTestReadbacks)
        {
            Readback = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(sizeof(uint32)), ERHIResourceState::CopyDest, nullptr);
        }
    }
#endif

    LoadReflectionNoiseMask();
    return true;
}

void FRayTracingReflectionsPass::Release()
{
    LocalVariant.Reset();
    BindlessVariant.Reset();
    SERVariant.Reset();

    InlineReflectionsPipeline.Reset();
    InlineReflectionsShader.Reset();

    CurrentSERHitGroupCapacity      = 0;
    CurrentHitGroupCapacity         = 0;
    CurrentBindlessHitGroupCapacity = 0;

    ReflectionNoiseTexture.Reset();
    ReflectionNoiseSize = 0;

#if STATS_ENABLED
    AlphaTestCounterUAV.Reset();
    AlphaTestCounter.Reset();
    for (FRHIBufferRef& Readback : AlphaTestReadbacks)
    {
        Readback.Reset();
    }

    AlphaTestFrame = 0;
#endif
}

#if STATS_ENABLED

void FRayTracingReflectionsPass::ClearAlphaTestCounter(FRHICommandList& CommandList)
{
    static constexpr uint32 Zero = 0;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(AlphaTestCounter.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopyDest));
    CommandList.UpdateBuffer(AlphaTestCounter.Get(), FBufferRegion(0, sizeof(uint32)), &Zero);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(AlphaTestCounter.Get(), ERHIResourceState::CopyDest, ERHIResourceState::UnorderedAccess));
}

void FRayTracingReflectionsPass::ReadBackAlphaTestCounter(FRHICommandList& CommandList)
{
    FRHIBuffer* Readback = AlphaTestReadbacks[AlphaTestFrame % NumAlphaTestReadbacks].Get();
    if (AlphaTestFrame >= NumAlphaTestReadbacks)
    {
        if (void* Mapped = Readback->Map(0, sizeof(uint32)))
        {
            STAT_SET(STAT_RT_AlphaTests, *static_cast<const uint32*>(Mapped));
            Readback->Unmap(0, sizeof(uint32));
        }
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(AlphaTestCounter.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyBuffer(Readback, AlphaTestCounter.Get(), FRHIBufferCopyDesc(0, 0, sizeof(uint32)));
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(AlphaTestCounter.Get(), ERHIResourceState::CopySource, ERHIResourceState::UnorderedAccess));

    ++AlphaTestFrame;
}

#endif

bool FRayTracingReflectionsPass::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    if (Width == 0 || Height == 0)
    {
        return false;
    }

    const ETextureUsageFlags UAVAndSRV =
        ETextureUsageFlags::UnorderedAccessTexture |
        ETextureUsageFlags::ShaderResourceTexture;

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::RayTracingOutputFormat,
        Width, Height, 1, 1, UAVAndSRV);
    Resources.RayTracingOutput = RHI::CreateTexture(OutputDesc, ERHIResourceState::UnorderedAccess);

    if (!Resources.RayTracingOutput)
    {
        return false;
    }

    Resources.RayTracingOutput->SetDebugName(TEXT("RayTracing Output"));

    FRHITextureDesc TraceDesc = FRHITextureDesc::CreateTexture2D(EFormat::R16G16B16A16_Float,
        Resources.GetReflectionChainWidth(), Resources.GetReflectionChainHeight(), 1, 1, UAVAndSRV,
        FClearValue(), ERHIResourceStateTrackingMode::Tracked);

    Resources.ReflectionTrace = RHI::CreateTexture(TraceDesc, ERHIResourceState::UnorderedAccess);
    if (!Resources.ReflectionTrace)
    {
        return false;
    }

    Resources.ReflectionTrace->SetDebugName(TEXT("Reflection Trace"));
    return true;
}

EReflectionPath FRayTracingReflectionsPass::SelectPath() const
{
    const bool bExplicitAvailable = RHI::bSupportsShaderBindingTableDescriptors && LocalVariant;
    const bool bInlineAvailable   = RHI::bSupportsInlineRayTracing && InlineReflectionsPipeline;
    if (bInlineAvailable && (GRayTracingInlineReflections || !RHI::bSupportsRayTracingPipeline))
    {
        return EReflectionPath::Inline;
    }

    if (GRayTracingSER && RHI::bSupportsShaderExecutionReordering && SERVariant)
    {
        return EReflectionPath::ShaderExecutionReordering;
    }

    if ((!GRayTracingEnableLocalShaderBindings || !bExplicitAvailable) && BindlessVariant)
    {
        return EReflectionPath::Bindless;
    }

    return EReflectionPath::Local;
}

const FRayTracingVariant& FRayTracingReflectionsPass::GetVariant(EReflectionPath Path) const
{
    switch (Path)
    {
    case EReflectionPath::ShaderExecutionReordering:
        return SERVariant;

    case EReflectionPath::Bindless:
        return BindlessVariant;

    default:
        return LocalVariant;
    }
}

FRHIShaderBindingTableRef& FRayTracingReflectionsPass::GetShaderBindingTable(FFrameResources& Resources, EReflectionPath Path)
{
    switch (Path)
    {
    case EReflectionPath::ShaderExecutionReordering:
        return Resources.RayTracingSERShaderBindingTable;

    case EReflectionPath::Bindless:
        return Resources.RayTracingBindlessShaderBindingTable;

    default:
        return Resources.RayTracingShaderBindingTable;
    }
}

uint32& FRayTracingReflectionsPass::GetHitGroupCapacity(EReflectionPath Path)
{
    switch (Path)
    {
    case EReflectionPath::ShaderExecutionReordering:
        return CurrentSERHitGroupCapacity;

    case EReflectionPath::Bindless:
        return CurrentBindlessHitGroupCapacity;

    default:
        return CurrentHitGroupCapacity;
    }
}

bool FRayTracingReflectionsPass::NeedsBindlessData() const
{
    return SelectPath() != EReflectionPath::Local;
}

bool FRayTracingReflectionsPass::UsesInlineRayTracing() const
{
    return SelectPath() == EReflectionPath::Inline;
}

bool FRayTracingReflectionsPass::IsTraceEnabled() const
{
    if (!RenderSettings::IsRayTracingEnabled() || !GReflectionsEnabled)
    {
        return false;
    }

    const EReflectionPath Path = SelectPath();
    return Path == EReflectionPath::Inline || GetVariant(Path).Pipeline != nullptr;
}

void FRayTracingReflectionsPass::LoadReflectionNoiseMask()
{
    FString FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith(TEXT("/")))
    {
        FullPath += TEXT("/");
    }

    FullPath += TEXT("Textures/Noise/BlueNoise_Vec2_128_RG.dds");

    ReflectionNoiseTexture = FAssetManager::Get().LoadTexture(FullPath, false);
    if (!ReflectionNoiseTexture)
    {
        LOG_WARNING("[RayTracingReflections]: Failed to load reflection noise mask '%s'. The blue noise sampler will fall back to white noise", *FullPath);
        return;
    }

    FTexture2D* Texture2D = ReflectionNoiseTexture->GetTexture2D();
    if (!Texture2D || (Texture2D->GetWidth() != Texture2D->GetHeight()))
    {
        LOG_WARNING("[RayTracingReflections]: Reflection noise mask '%s' is not square. The blue noise sampler will fall back to white noise", *FullPath);

        ReflectionNoiseTexture.Reset();
        return;
    }

    ReflectionNoiseSize = Texture2D->GetWidth();
    LOG_INFO("[RayTracingReflections]: Loaded reflection noise mask '%s' (%ux%u)", *FullPath, ReflectionNoiseSize, ReflectionNoiseSize);
}

FRHITexture* FRayTracingReflectionsPass::GetReflectionNoiseMask() const
{
    if (!ReflectionNoiseTexture)
    {
        return nullptr;
    }

    FTexture2D* Texture2D = ReflectionNoiseTexture->GetTexture2D();
    return Texture2D ? Texture2D->GetRHITexture().Get() : nullptr;
}

void FRayTracingReflectionsPass::Record(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bDenoise)
{
    if (GRayTracingSER && !RHI::bSupportsShaderExecutionReordering)
    {
        static bool bLoggedSERUnsupported = false;
        if (!bLoggedSERUnsupported)
        {
            LOG_WARNING("[RayTracingReflections]: Renderer.RayTracing.SER=1 requested but Shader Execution Reordering is unsupported on this backend. Falling back to the non-SER reflection path");
            bLoggedSERUnsupported = true;
        }
    }

    if (GRayTracingEnableLocalShaderBindings && !RHI::bSupportsShaderBindingTableDescriptors)
    {
        static bool bLoggedLocalBindingsUnsupported = false;
        if (!bLoggedLocalBindingsUnsupported)
        {
            LOG_WARNING("[RayTracingReflections]: Renderer.RayTracing.EnableLocalShaderBindings=1 requested but this backend does not support SBT descriptor records. Falling back to the bindless reflection path");
            bLoggedLocalBindingsUnsupported = true;
        }
    }

    const EReflectionPath Path = SelectPath();

    FRHITexture* const TraceTarget  = bDenoise ? Resources.ReflectionTrace.Get() : Resources.RayTracingOutput.Get();
    FRHITexture*       DiffuseCube  = nullptr;
    FRHITexture*       SpecularCube = nullptr;

    const CHAR_T* MissingResource = nullptr;
    if (!TraceTarget)
    {
        MissingResource = bDenoise ? TEXT("ReflectionTrace") : TEXT("RayTracingOutput");
    }
    else if (!Resources.RayTracingSceneConstantsBuffer)
    {
        MissingResource = TEXT("RayTracingSceneConstantsBuffer");
    }

    if (MissingResource)
    {
        static bool bLoggedMissingResource = false;

        if (!bLoggedMissingResource)
        {
            LOG_WARNING("[RayTracingReflections]: %s has not been allocated. Skipping the RT reflection pass", MissingResource);
            bLoggedMissingResource = true;
        }

        return;
    }

    {
        FRayTracingSceneConstantsHLSL Constants;
        Constants.FrameIndex = GetRenderer()->GetFrameCounter().GetFrameIndex();

        if (FSceneSkyLight* SkyLight = Scene->GetSkyLight())
        {
            DiffuseCube  = SkyLight->DiffuseCubeMap.Get();
            SpecularCube = SkyLight->SpecularCubeMap.Get();

            if (SkyLight->SpecularCubeMap)
            {
                Constants.NumSkyLightMips = SkyLight->SpecularCubeMap->GetDesc().NumMipLevels;
            }

            Constants.SkyLightColor    = SkyLight->Color;
            Constants.SkyLightRotation = SkyLight->Rotation;
        }
        else if (FSceneSkybox* Skybox = Scene->GetSkybox())
        {
            if (Skybox->CubeMap)
            {
                DiffuseCube  = Skybox->CubeMap.Get();
                SpecularCube = Skybox->CubeMap.Get();

                Constants.NumSkyLightMips = Skybox->CubeMap->GetDesc().NumMipLevels;
            }

            Constants.SkyLightColor    = FVector3(Skybox->Intensity, Skybox->Intensity, Skybox->Intensity);
            Constants.SkyLightRotation = Skybox->Rotation;
        }

        if (FSceneSkybox* Skybox = Scene->GetSkybox())
        {
            Constants.SkyboxIntensity = Skybox->Intensity;
            Constants.SkyboxRotation  = Skybox->Rotation;
        }

        Constants.SunDirection = Resources.DirectionalLightData.Direction;
        Constants.SunColor     = Resources.DirectionalLightData.Color;

        const int32 PointLightCount = Math::Min<int32>(Resources.ShadowCastingPointLightsData.Size(), RT_MAX_POINT_LIGHTS);
        Constants.NumPointLights = static_cast<uint32>(PointLightCount);

        for (int32 LightIndex = 0; LightIndex < PointLightCount; ++LightIndex)
        {
            Constants.PointLightPositionRadius[LightIndex] = Resources.ShadowCastingPointLightsPosRad[LightIndex];

            const FVector3 LightColor = Resources.ShadowCastingPointLightsData[LightIndex].Color;
            Constants.PointLightColor[LightIndex] = FVector4(LightColor.X, LightColor.Y, LightColor.Z, 0.0f);
        }

        Constants.ReflectionMaxRayDistance           = Math::Max(1.0f, GReflectionMaxRayDistance);
        Constants.ReflectionMirrorRoughnessThreshold = Math::Clamp(GReflectionMirrorRoughnessThreshold, 0.0f, 1.0f);
        Constants.ReflectionRayBias                  = Math::Max(0.0f, GReflectionRayBias);
        Constants.ReflectionSampler                  = static_cast<uint32>(Math::Clamp<int32>(GReflectionSampler, EReflectionSampler::Min, EReflectionSampler::Max));
        Constants.ReflectionNoiseSize                = ReflectionNoiseSize;

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.RayTracingSceneConstantsBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
        CommandList.UpdateBuffer(Resources.RayTracingSceneConstantsBuffer.Get(), FBufferRegion(0, sizeof(FRayTracingSceneConstantsHLSL)), &Constants);
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(Resources.RayTracingSceneConstantsBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));
    }

    FRHISamplerState* const EnvSampler = Resources.LightProbeSampler ? Resources.LightProbeSampler.Get() : Resources.GBufferSampler.Get();
    FRHISamplerState* const LUTSampler = Resources.IntegrationLUTSampler ? Resources.IntegrationLUTSampler.Get() : Resources.GBufferSampler.Get();
    FRHITexture*      const NoiseMask  = GetReflectionNoiseMask();

    const auto BindReflectionGlobals = [&](auto* Shader)
    {
        CommandList.SetConstantBuffer(Shader, Resources.CameraBuffer.Get(), 0);
        CommandList.SetConstantBuffer(Shader, Resources.RayTracingSceneConstantsBuffer.Get(), 1);

        if (Resources.RayTracingScene)
        {
            CommandList.SetShaderResourceView(Shader, Resources.RayTracingScene->GetShaderResourceView(), 0);
        }

        if (FSceneSkybox* Skybox = Scene->GetSkybox())
        {
            if (Skybox->CubeMap)
            {
                CommandList.SetShaderResourceView(Shader, Skybox->CubeMap->GetShaderResourceView(), 1);
            }
        }

        CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 2);
        CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 3);
        CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Material]->GetShaderResourceView(), 4);

        if (DiffuseCube)
        {
            CommandList.SetShaderResourceView(Shader, DiffuseCube->GetShaderResourceView(), 5);
        }

        if (SpecularCube)
        {
            CommandList.SetShaderResourceView(Shader, SpecularCube->GetShaderResourceView(), 6);
        }

        if (Resources.IntegrationLUT)
        {
            CommandList.SetShaderResourceView(Shader, Resources.IntegrationLUT->GetShaderResourceView(), 7);
        }

        if (Resources.MaterialDataBufferSRV)
        {
            CommandList.SetShaderResourceView(Shader, Resources.MaterialDataBufferSRV.Get(), 8);
        }

        if (Resources.RayTracingGeometryTableSRV)
        {
            CommandList.SetShaderResourceView(Shader, Resources.RayTracingGeometryTableSRV.Get(), 9);
        }

        if (NoiseMask)
        {
            CommandList.SetShaderResourceView(Shader, NoiseMask->GetShaderResourceView(), 10);
        }

        CommandList.SetUnorderedAccessView(Shader, TraceTarget->GetUnorderedAccessView(), 0);

#if STATS_ENABLED
        CommandList.SetUnorderedAccessView(Shader, AlphaTestCounterUAV.Get(), 1);
#endif

        CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
        CommandList.SetSamplerState(Shader, EnvSampler, 2);
        CommandList.SetSamplerState(Shader, LUTSampler, 3);
    };

    const uint32 Width  = TraceTarget->GetDesc().Extent.X;
    const uint32 Height = TraceTarget->GetDesc().Extent.Y;

    if (Path == EReflectionPath::Inline)
    {
        ClearAlphaTestCounter(CommandList);
        CommandList.SetComputePipelineState(InlineReflectionsPipeline.Get());

        BindReflectionGlobals(InlineReflectionsShader.Get());

        constexpr uint32 ThreadCount = 8;
        const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
        const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        ReadBackAlphaTestCounter(CommandList);
        return;
    }

    const FRayTracingVariant&    ActiveVariant            = GetVariant(Path);
    FRHIRayTracingPipelineState* ActivePipeline           = ActiveVariant.Pipeline.Get();
    FRHIShaderBindingTableRef&   ActiveShaderBindingTable = GetShaderBindingTable(Resources, Path);
    uint32&                      ActiveCapacity           = GetHitGroupCapacity(Path);

    if (!ActivePipeline)
    {
        static bool bLoggedNoActivePipeline = false;

        if (!bLoggedNoActivePipeline)
        {
            LOG_WARNING("[RayTracingReflections]: No usable ray tracing reflection pipeline for the selected path. Skipping the RT reflection pass");
            bLoggedNoActivePipeline = true;
        }

        return;
    }

    const uint32 NumHitGroupBindings = static_cast<uint32>(Resources.RayTracingHitGroupBindings.Size());

    uint32 NumReferencedHitGroups = 0;
    for (const FRHIGeometryAccelerationStructureInstance& Instance : Resources.RayTracingGeometryInstances)
    {
        NumReferencedHitGroups = Math::Max<uint32>(NumReferencedHitGroups, Instance.HitGroupIndex + 1);
    }

    CHECK(NumHitGroupBindings >= NumReferencedHitGroups);

    const uint32 NumHitGroupRecords = Math::Max<uint32>(Math::Max<uint32>(NumHitGroupBindings, NumReferencedHitGroups), 1u);
    if (!ActiveShaderBindingTable || NumHitGroupRecords > ActiveCapacity)
    {
        FRHIShaderBindingTableDesc SBTDesc = FRHIShaderBindingTableDesc(ActivePipeline, 1, 1, 0, NumHitGroupRecords);
        ActiveShaderBindingTable = RHI::CreateShaderBindingTable(SBTDesc);
        ActiveCapacity = NumHitGroupRecords;
    }

    if (!ActiveShaderBindingTable)
    {
        return;
    }

    FRHIShaderBindingTable* ShaderBindingTable = ActiveShaderBindingTable.Get();
    CommandList.SetShaderRecord(ShaderBindingTable, FRHIShaderRecordDesc(ERayTracingShaderRecordKind::RayGeneration, 0, 0));
    CommandList.SetShaderRecord(ShaderBindingTable, FRHIShaderRecordDesc(ERayTracingShaderRecordKind::Miss, 0, 0));

    for (uint32 RecordIndex = 0; RecordIndex < NumHitGroupRecords; ++RecordIndex)
    {
        if (RecordIndex < NumHitGroupBindings)
        {
            const TArray<FRHIHitGroupLocalShaderBinding>&          Record = Resources.RayTracingHitGroupBindings[RecordIndex];
            const TArrayView<const FRHIHitGroupLocalShaderBinding> Bindings(Record.Data(), Record.Size());
            const uint32                                           HitGroup = Resources.RayTracingHitGroupShaderIndices[RecordIndex];
            CommandList.SetShaderRecord(ShaderBindingTable, FRHIShaderRecordDesc(ERayTracingShaderRecordKind::HitGroup, RecordIndex, HitGroup, Bindings));
        }
        else
        {
            CommandList.SetShaderRecord(ShaderBindingTable, FRHIShaderRecordDesc(ERayTracingShaderRecordKind::HitGroup, RecordIndex, 0));
        }
    }

    CommandList.BuildShaderBindingTable(ShaderBindingTable);

    ClearAlphaTestCounter(CommandList);
    CommandList.SetRayTracingPipelineState(ActivePipeline);

    BindReflectionGlobals(ActiveVariant.RayGenShader.Get());

    CommandList.DispatchRays(ShaderBindingTable, Width, Height, 1);
    ReadBackAlphaTestCounter(CommandList);
}

void FRayTracingReflectionsPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, bool bDenoise)
{
    FRenderGraphTexture* const TraceTarget = bDenoise ? Context.ReflectionTrace : Context.RayTracingOutput;

    GraphBuilder.AddPass(TEXT("RayTracingReflections"), ERenderGraphPassFlags::Compute, IsTraceEnabled(),
        [&Context, TraceTarget](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferMaterial, ERHIResourceState::NonPixelShaderResource);

            if (Context.IntegrationLUT)
            {
                PassBuilder.ReadTexture(Context.IntegrationLUT, ERHIResourceState::NonPixelShaderResource);
            }

            if (Context.RayTracingSceneConstantsBuffer)
            {
                PassBuilder.ReadBuffer(Context.RayTracingSceneConstantsBuffer, ERHIResourceState::ConstantBuffer);
            }

            if (Context.RayTracingGeometryTableBuffer)
            {
                PassBuilder.ReadBuffer(Context.RayTracingGeometryTableBuffer, ERHIResourceState::GenericRead);
            }

            if (Context.ReflectionNoise)
            {
                PassBuilder.ReadTexture(Context.ReflectionNoise, ERHIResourceState::NonPixelShaderResource);
            }

            if (TraceTarget)
            {
                PassBuilder.WriteTexture(TraceTarget, ERHIResourceState::UnorderedAccess);
            }
        },
        [this, Context, bDenoise](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene, bDenoise);
        });
}
