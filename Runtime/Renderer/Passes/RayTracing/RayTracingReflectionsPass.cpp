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
    "Renderer.RayTracing.EnableLocalShaderBindings",
    "When true (default), ray tracing reflections carry per-hit-group resources via local-root-signature SBT records + explicit global "
    "bindings. When false they use the SM 6.6 bindless descriptor heap. Inverse of the old Renderer.RayTracing.Bindless. Forced to "
    "bindless on backends without SBT descriptor support.",
    GRayTracingEnableLocalShaderBindings);

static bool GRayTracingInlineReflections = false;
static FAutoConsoleVariableRef CVarRayTracingInlineReflections(
    "Renderer.RayTracing.InlineReflections",
    "When true, reflections are produced by an inline RayQuery compute pass (TraceRayInline) instead of DispatchRays. Requires inline ray "
    "tracing support. Reuses the bindless geometry table.",
    GRayTracingInlineReflections);

static bool GRayTracingSER = false;
static FAutoConsoleVariableRef CVarRayTracingSER(
    "Renderer.RayTracing.SER",
    "When true, reflections use a Shader-Execution-Reordering RayGen (HitObject/MaybeReorderThread, SM 6.9) over the bindless path. "
    "Requires SER support. No-op reorder on API-only devices.",
    GRayTracingSER);

bool GReflectionsEnabled = true;
static FAutoConsoleVariableRef CVarReflectionsEnabled(
    "Renderer.RayTracing.Reflections.Enable",
    "Enables ray-traced reflections. Acceleration structures are still built while this is off, so other ray traced effects keep working "
    "and re-enabling does not rebuild the scene.",
    GReflectionsEnabled);

static float GReflectionMaxRayDistance = 10000.0f;
static FAutoConsoleVariableRef CVarReflectionMaxRayDistance(
    "Renderer.RayTracing.Reflections.MaxRayDistance",
    "Maximum distance (TMax) traced by a reflection ray. Shorter distances trade far-field reflections for traversal cost.",
    GReflectionMaxRayDistance);

float GReflectionMirrorRoughnessThreshold = 0.05f;
static FAutoConsoleVariableRef CVarReflectionMirrorRoughnessThreshold(
    "Renderer.RayTracing.Reflections.MirrorRoughnessThreshold",
    "Surfaces below this roughness trace a perfect mirror ray; above it the direction is GGX importance-sampled.",
    GReflectionMirrorRoughnessThreshold);

static float GReflectionRayBias = 0.02f;
static FAutoConsoleVariableRef CVarReflectionRayBias(
    "Renderer.RayTracing.Reflections.RayBias",
    "Distance the reflection ray origin is pushed along the surface normal to avoid self-intersection.",
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
    "Renderer.RayTracing.Reflections.Sampler",
    "Sequence used to importance-sample the GGX lobe for reflection rays: 0 = white noise, 1 = Halton, 2 = blue noise. Falls back to white "
    "noise when the blue noise mask failed to load.",
    GReflectionSampler);

FRayTracingReflectionsPass::FRayTracingReflectionsPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CurrentSERHitGroupCapacity(0)
    , CurrentHitGroupCapacity(0)
    , CurrentBindlessHitGroupCapacity(0)
    , ReflectionNoiseSize(0)
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

    if (!RayGenShader || !MissShader || !ClosestHitShader)
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
        FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { ClosestHitShader.Get() })
    };

    PSODesc.MaxRecursionDepth       = 1;
    PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
    PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);
    PSODesc.Flags                   = Permutation.Get<FRayTracingSER>()
        ? ERayTracingPipelineFlags::AllowShaderExecutionReordering
        : ERayTracingPipelineFlags::None;

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
        LOG_ERROR("[RayTracingReflections]: No usable ray tracing pipeline (neither explicit nor bindless could be created)");
        DEBUG_BREAK();
        return false;
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

    {
        const FRHIBufferDesc ConstantsDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FRayTracingSceneConstantsHLSL));
        Resources.RayTracingSceneConstantsBuffer = RHI::CreateBuffer(ConstantsDesc, ERHIResourceState::ConstantBuffer, nullptr);
        if (!Resources.RayTracingSceneConstantsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }

        Resources.RayTracingSceneConstantsBuffer->SetDebugName("RayTracing Scene Constants");
    }

    if (RHI::bSupportsInlineRayTracing)
    {
        InlineReflectionsShader = FShaderCache::Get().GetShader<FInlineReflectionsCS>();
        if (InlineReflectionsShader)
        {
            FRHIComputePipelineStateDesc PSODesc;
            PSODesc.Shader = InlineReflectionsShader.Get();

            InlineReflectionsPipeline = RHI::CreateComputePipelineState(PSODesc);
            if (InlineReflectionsPipeline)
            {
                InlineReflectionsPipeline->SetDebugName("Inline RT Reflections PSO");
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

    ReflectionNoiseTexture.Reset();
    ReflectionNoiseSize = 0;
}

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

    Resources.RayTracingOutput->SetDebugName("RayTracing Output");

    FRHITextureDesc TraceDesc = FRHITextureDesc::CreateTexture2D(EFormat::R16G16B16A16_Float,
        Resources.GetReflectionChainWidth(), Resources.GetReflectionChainHeight(), 1, 1, UAVAndSRV,
        FClearValue(), ERHIResourceStateTrackingMode::Tracked);

    Resources.ReflectionTrace = RHI::CreateTexture(TraceDesc, ERHIResourceState::UnorderedAccess);
    if (!Resources.ReflectionTrace)
    {
        return false;
    }

    Resources.ReflectionTrace->SetDebugName("Reflection Trace");
    return true;
}

EReflectionPath FRayTracingReflectionsPass::SelectPath() const
{
    const bool bExplicitAvailable = RHI::bSupportsShaderBindingTableDescriptors && LocalVariant;
    if (GRayTracingInlineReflections && RHI::bSupportsInlineRayTracing && InlineReflectionsPipeline)
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

bool FRayTracingReflectionsPass::NeedsBindlessData() const
{
    return SelectPath() != EReflectionPath::Local;
}

bool FRayTracingReflectionsPass::IsTraceEnabled() const
{
    if (!RHI::bSupportsRayTracing || !GRayTracingEnabled || !GReflectionsEnabled)
    {
        return false;
    }

    const EReflectionPath Path = SelectPath();
    return Path == EReflectionPath::Inline || GetVariant(Path).Pipeline != nullptr;
}

void FRayTracingReflectionsPass::LoadReflectionNoiseMask()
{
    String FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += "Textures/Noise/BlueNoise_Vec2_128_RG.dds";

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

    const CHAR* MissingResource = nullptr;
    if (!TraceTarget)
    {
        MissingResource = bDenoise ? "ReflectionTrace" : "RayTracingOutput";
    }
    else if (!Resources.RayTracingSceneConstantsBuffer)
    {
        MissingResource = "RayTracingSceneConstantsBuffer";
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
        }
        else if (FSceneSkybox* Skybox = Scene->GetSkybox())
        {
            if (Skybox->CubeMap)
            {
                DiffuseCube  = Skybox->CubeMap.Get();
                SpecularCube = Skybox->CubeMap.Get();

                Constants.NumSkyLightMips = Skybox->CubeMap->GetDesc().NumMipLevels;
            }
        }

        Constants.SunDirection = Resources.DirectionalLightData.Direction;
        Constants.SunColor     = Resources.DirectionalLightData.Color;

        const int32 PointLightCount = Math::Min<int32>(Resources.ShadowCastingPointLightsData.Size(), RT_MAX_POINT_LIGHTS);
        Constants.NumPointLights = static_cast<uint32>(PointLightCount);

        for (int32 LightIndex = 0; LightIndex < PointLightCount; ++LightIndex)
        {
            Constants.PointLightPositionRadius[LightIndex] = Resources.ShadowCastingPointLightsPosRad[LightIndex];

            const Vector3 LightColor = Resources.ShadowCastingPointLightsData[LightIndex].Color;
            Constants.PointLightColor[LightIndex] = Vector4(LightColor.X, LightColor.Y, LightColor.Z, 0.0f);
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

        CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
        CommandList.SetSamplerState(Shader, EnvSampler, 2);
        CommandList.SetSamplerState(Shader, LUTSampler, 3);
    };

    const uint32 Width  = TraceTarget->GetDesc().Extent.X;
    const uint32 Height = TraceTarget->GetDesc().Extent.Y;

    if (Path == EReflectionPath::Inline)
    {
        CommandList.SetComputePipelineState(InlineReflectionsPipeline.Get());

        BindReflectionGlobals(InlineReflectionsShader.Get());

        constexpr uint32 ThreadCount = 8;
        const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
        const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        return;
    }

    const FRayTracingVariant&    ActiveVariant            = GetVariant(Path);
    FRHIRayTracingPipelineState* ActivePipeline           = ActiveVariant.Pipeline.Get();
    FRHIShaderBindingTableRef&   ActiveShaderBindingTable = Resources.RayTracingShaderBindingTable;
    uint32&                      ActiveCapacity           = CurrentHitGroupCapacity;

    if (Path == EReflectionPath::ShaderExecutionReordering)
    {
        ActiveShaderBindingTable = Resources.RayTracingSERShaderBindingTable;
        ActiveCapacity           = CurrentSERHitGroupCapacity;
    }
    else if (Path == EReflectionPath::Bindless)
    {
        ActiveShaderBindingTable = Resources.RayTracingBindlessShaderBindingTable;
        ActiveCapacity           = CurrentBindlessHitGroupCapacity;
    }

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

    const uint32 NumHitGroupRecords = Math::Max<uint32>(static_cast<uint32>(Resources.RayTracingHitGroupBindings.Size()), 1u);
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
    CommandList.SetHitRecordLocalShaderBindings(ShaderBindingTable, ERayTracingShaderRecordKind::RayGeneration, 0, nullptr, 0);
    CommandList.SetHitRecordLocalShaderBindings(ShaderBindingTable, ERayTracingShaderRecordKind::Miss, 0, nullptr, 0);

    uint32 RecordIndex = 0;
    for (const TArray<FRHIHitGroupLocalShaderBinding>& Record : Resources.RayTracingHitGroupBindings)
    {
        CommandList.SetHitRecordLocalShaderBindings(ShaderBindingTable, ERayTracingShaderRecordKind::HitGroup, RecordIndex++, Record.Data(), Record.Size());
    }

    if (RecordIndex == 0)
    {
        CommandList.SetHitRecordLocalShaderBindings(ShaderBindingTable, ERayTracingShaderRecordKind::HitGroup, 0, nullptr, 0);
    }

    CommandList.BuildShaderBindingTable(ShaderBindingTable);
    CommandList.SetRayTracingPipelineState(ActivePipeline);

    BindReflectionGlobals(ActiveVariant.RayGenShader.Get());

    CommandList.DispatchRays(ShaderBindingTable, Width, Height, 1);
}

void FRayTracingReflectionsPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, bool bDenoise)
{
    FRenderGraphTexture* const TraceTarget = bDenoise ? Context.ReflectionTrace : Context.RayTracingOutput;

    GraphBuilder.AddPass("RayTracingReflections", ERenderGraphPassFlags::Compute, IsTraceEnabled(),
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
