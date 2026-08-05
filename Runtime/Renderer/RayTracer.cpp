#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Templates/CString.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Engine.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Texture.h"
#include "Renderer/RayTracer.h"
#include "Renderer/RayTracingBindless.h"
#include "Renderer/ReflectionSettings.h"
#include "Renderer/RendererStats.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "Renderer/Scene/SceneSkyLight.h"

static bool GRayTracingEnableLocalShaderBindings = true;
static FAutoConsoleVariableRef CVarRayTracingEnableLocalShaderBindings(
    "Renderer.RayTracing.EnableLocalShaderBindings",
    "When true (default), ray tracing reflections carry per-hit-group resources via local-root-signature SBT records + explicit global bindings. When false they use the SM 6.6 bindless descriptor heap. Inverse of the old Renderer.RayTracing.Bindless. Forced to bindless on backends without SBT descriptor support.",
    GRayTracingEnableLocalShaderBindings);

static bool GRayTracingCompaction = false;
static FAutoConsoleVariableRef CVarRayTracingCompaction(
    "Renderer.RayTracing.Compaction",
    "When true, each BLAS is compacted once after build (multi-frame read-back + in-place swap). Requires AllowCompaction on the BLAS. UNVALIDATED on hardware.",
    GRayTracingCompaction);

static bool GRayTracingASCache = false;
static FAutoConsoleVariableRef CVarRayTracingASCache(
    "Renderer.RayTracing.ASCache",
    "When true, each BLAS is serialized once after build into the on-disk acceleration-structure cache. UNVALIDATED on hardware.",
    GRayTracingASCache);

static bool GRayTracingInlineReflections = false;
static FAutoConsoleVariableRef CVarRayTracingInlineReflections(
    "Renderer.RayTracing.InlineReflections",
    "When true, reflections are produced by an inline RayQuery compute pass (TraceRayInline) instead of DispatchRays. Requires inline ray tracing support. Reuses the bindless geometry table.",
    GRayTracingInlineReflections);

static bool GRayTracingSER = false;
static FAutoConsoleVariableRef CVarRayTracingSER(
    "Renderer.RayTracing.SER",
    "When true, reflections use a Shader-Execution-Reordering RayGen (HitObject/MaybeReorderThread, SM 6.9) over the bindless path. Requires SER support. No-op reorder on API-only devices.",
    GRayTracingSER);

bool GReflectionsEnabled = true;
static FAutoConsoleVariableRef CVarReflectionsEnabled(
    "Renderer.RayTracing.Reflections.Enable",
    "Enables ray-traced reflections. Acceleration structures are still built while this is off, so other ray traced effects keep working and re-enabling does not rebuild the scene.",
    GReflectionsEnabled);

static bool GReflectionDenoise = true;
static FAutoConsoleVariableRef CVarReflectionDenoise(
    "Renderer.RayTracing.Reflections.Denoise",
    "Enable the reflection denoiser (temporal accumulation + SVGF a-trous). Off = raw 1-spp reflection trace.",
    GReflectionDenoise);

static int32 GReflectionAtrousIterations = 4;
static FAutoConsoleVariableRef CVarReflectionAtrousIterations(
    "Renderer.RayTracing.Reflections.AtrousIterations",
    "Number of SVGF a-trous spatial-filter iterations (0 disables the spatial filter).",
    GReflectionAtrousIterations);

static float GReflectionTemporalAlpha = 0.1f;
static FAutoConsoleVariableRef CVarReflectionTemporalAlpha(
    "Renderer.RayTracing.Reflections.TemporalAlpha",
    "Minimum temporal blend toward the current frame (1 = no history, lower = more accumulation).",
    GReflectionTemporalAlpha);

static float GReflectionMaxRadiance = 8.0f;
static FAutoConsoleVariableRef CVarReflectionMaxRadiance(
    "Renderer.RayTracing.Reflections.MaxRadiance",
    "Luminance clamp applied to each reflection sample before temporal accumulation to suppress fireflies (<= 0 disables the clamp).",
    GReflectionMaxRadiance);

static float GReflectionHistoryClampGamma = 1.0f;
static FAutoConsoleVariableRef CVarReflectionHistoryClampGamma(
    "Renderer.RayTracing.Reflections.HistoryClampGamma",
    "Width (in neighborhood std-devs) of the temporal history clamp used to suppress motion ghosting. Lower = tighter/less ghosting but more noise. <= 0 disables history rectification.",
    GReflectionHistoryClampGamma);

float GReflectionMaxHistoryLength = 32.0f;
static FAutoConsoleVariableRef CVarReflectionMaxHistoryLength(
    "Renderer.RayTracing.Reflections.MaxHistoryLength",
    "Maximum number of frames of reflection temporal accumulation. Shorter = faster response / less ghosting, longer = smoother / more ghosting.",
    GReflectionMaxHistoryLength);

static int32 GReflectionNeighborhoodRadius = 4;
static FAutoConsoleVariableRef CVarReflectionNeighborhoodRadius(
    "Renderer.RayTracing.Reflections.NeighborhoodRadius",
    "Half-window (in texels) of the current-frame neighborhood used to build the temporal history color clamp box.",
    GReflectionNeighborhoodRadius);

static float GReflectionCameraMotionMaxHistory = 8.0f;
static FAutoConsoleVariableRef CVarReflectionCameraMotionMaxHistory(
    "Renderer.RayTracing.Reflections.CameraMotionMaxHistory",
    "Accumulation cap (frames) applied while the camera is translating, so history decays quickly and does not smear. <= 0 disables the camera-motion reset.",
    GReflectionCameraMotionMaxHistory);

static bool GReflectionHalfRes = false;
static FAutoConsoleVariableRef CVarReflectionHalfRes(
    "Renderer.RayTracing.Reflections.HalfRes",
    "Trace + denoise reflections at half resolution and bilateral-upsample to full res (faster, softer).",
    GReflectionHalfRes);

static float GReflectionAtrousPhiColor = 4.0f;
static FAutoConsoleVariableRef CVarReflectionAtrousPhiColor(
    "Renderer.RayTracing.Reflections.AtrousPhiColor",
    "Color sensitivity of the SVGF a-trous edge-stopping function. Lower preserves more detail but keeps more noise.",
    GReflectionAtrousPhiColor);

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

// Mirrors REFLECTION_SAMPLER_* in Shaders/Reflections/ReflectionSampling.hlsli.
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
    "Sequence used to importance-sample the GGX lobe for reflection rays: 0 = white noise, 1 = Halton, 2 = blue noise. Falls back to white noise when the blue noise mask failed to load.",
    GReflectionSampler);

FRayTracer::FRayTracer(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CurrentSERHitGroupCapacity(0)
    , CurrentHitGroupCapacity(0)
    , CurrentBindlessHitGroupCapacity(0)
    , GeometryTableCapacity(0)
    , SerializationHelper(&ASCache)
    , ReflectionHistoryIndex(0)
    , bReflectionHistoryValid(false)
    , bDenoiserHalfRes(false)
    , DenoiserFullWidth(0)
    , DenoiserFullHeight(0)
    , ReflectionNoiseSize(0)
{
}

FRayTracer::~FRayTracer()
{
}

bool FRayTracer::Initialize(FFrameResources& Resources)
{
    TArray<uint8> Code;

    const auto InitializeExplicit = [&]() -> bool
    {
        {
            FShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_3, EShaderStage::RayGen);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayGenShader = RHI::CreateRayGenShader(Code);
            if (!RayGenShader)
            {
                return false;
            }
        }

        {
            FShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_3, EShaderStage::RayClosestHit);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayClosestHitShader = RHI::CreateRayClosestHitShader(Code);
            if (!RayClosestHitShader)
            {
                return false;
            }
        }

        {
            FShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_3, EShaderStage::RayMiss);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayMissShader = RHI::CreateRayMissShader(Code);
            if (!RayMissShader)
            {
                return false;
            }
        }

        FRHIRayTracingPipelineStateDesc PSODesc;
        PSODesc.RayGenShaders           = { RayGenShader.Get() };
        PSODesc.MissShaders             = { RayMissShader.Get() };
        PSODesc.HitGroups               = { FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShader.Get() }) };
        PSODesc.MaxRecursionDepth       = 1;
        PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
        PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);

        LocalPipeline = RHI::CreateRayTracingPipelineState(PSODesc);
        return LocalPipeline != nullptr;
    };

    if (RHI::bSupportsShaderBindingTableDescriptors)
    {
        if (!InitializeExplicit())
        {
            LOG_WARNING("[RayTracer]: Explicit ray tracing variant unavailable. Only the bindless path will be used");

            LocalPipeline.Reset();
            RayGenShader.Reset();
            RayMissShader.Reset();
            RayClosestHitShader.Reset();
        }
    }

    const auto InitializeBindless = [&]() -> bool
    {
        TArray<FShaderDefine> Defines =
        {
            FShaderDefine("RAY_TRACING_BINDLESS", "1")
        };

        {
            FShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_6, EShaderStage::RayGen, Defines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayGenShaderBindless = RHI::CreateRayGenShader(Code);
            if (!RayGenShaderBindless)
            {
                return false;
            }
        }

        {
            FShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_6, EShaderStage::RayClosestHit, Defines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayClosestHitShaderBindless = RHI::CreateRayClosestHitShader(Code);
            if (!RayClosestHitShaderBindless)
            {
                return false;
            }
        }

        {
            FShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_6, EShaderStage::RayMiss, Defines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
            {
                return false;
            }

            RayMissShaderBindless = RHI::CreateRayMissShader(Code);
            if (!RayMissShaderBindless)
            {
                return false;
            }
        }

        FRHIRayTracingPipelineStateDesc PSODesc;
        PSODesc.RayGenShaders           = { RayGenShaderBindless.Get() };
        PSODesc.MissShaders             = { RayMissShaderBindless.Get() };
        PSODesc.HitGroups               = { FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShaderBindless.Get() }) };
        PSODesc.MaxRecursionDepth       = 1;
        PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
        PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);

        BindlessPipeline = RHI::CreateRayTracingPipelineState(PSODesc);
        return BindlessPipeline != nullptr;
    };

    if (!RHI::bSupportsBindless || !InitializeBindless())
    {
        LOG_WARNING("[RayTracer]: Bindless ray tracing variant unavailable. Only the explicit-binding path will be used");

        BindlessPipeline.Reset();
        RayGenShaderBindless.Reset();
        RayMissShaderBindless.Reset();
        RayClosestHitShaderBindless.Reset();
    }

    if (!LocalPipeline && !BindlessPipeline)
    {
        LOG_ERROR("[RayTracer]: No usable ray tracing pipeline (neither explicit nor bindless could be created)");
        DEBUG_BREAK();
        return false;
    }

    if (RHI::bSupportsShaderExecutionReordering && BindlessPipeline)
    {
        const auto InitializeSER = [&]() -> bool
        {
            TArray<FShaderDefine> SERDefines =
            {
                FShaderDefine("RAY_TRACING_BINDLESS", "1"),
                FShaderDefine("RAY_TRACING_SHADER_EXECUTION_REORDERING", "1")
            };

            {
                FShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_9, EShaderStage::RayGen, SERDefines);
                if (!FShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
                {
                    return false;
                }

                RayGenShaderSER = RHI::CreateRayGenShader(Code);
                if (!RayGenShaderSER)
                {
                    return false;
                }
            }

            {
                FShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_9, EShaderStage::RayClosestHit, SERDefines);
                if (!FShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
                {
                    return false;
                }

                RayClosestHitShaderSER = RHI::CreateRayClosestHitShader(Code);
                if (!RayClosestHitShaderSER)
                {
                    return false;
                }
            }

            {
                FShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_9, EShaderStage::RayMiss, SERDefines);
                if (!FShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
                {
                    return false;
                }

                RayMissShaderSER = RHI::CreateRayMissShader(Code);
                if (!RayMissShaderSER)
                {
                    return false;
                }
            }

            FRHIRayTracingPipelineStateDesc PSODesc;
            PSODesc.RayGenShaders           = { RayGenShaderSER.Get() };
            PSODesc.MissShaders             = { RayMissShaderSER.Get() };
            PSODesc.HitGroups               = { FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShaderSER.Get() }) };
            PSODesc.MaxRecursionDepth       = 1;
            PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
            PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);
            PSODesc.Flags                   = ERayTracingPipelineFlags::AllowShaderExecutionReordering;

            SERPipeline = RHI::CreateRayTracingPipelineState(PSODesc);
            return SERPipeline != nullptr;
        };

        if (!InitializeSER())
        {
            LOG_WARNING("[RayTracer]: SER ray tracing variant unavailable. The SER path will be disabled");

            SERPipeline.Reset();
            RayGenShaderSER.Reset();
            RayMissShaderSER.Reset();
            RayClosestHitShaderSER.Reset();
        }
    }

    {
        FRHIBufferDesc IndicesDesc;
        IndicesDesc.Size   = sizeof(FRayTracingSceneConstantsHLSL);
        IndicesDesc.Stride = sizeof(FRayTracingSceneConstantsHLSL);
        IndicesDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

        Resources.RayTracingSceneConstantsBuffer = RHI::CreateBuffer(IndicesDesc, ERHIResourceState::ConstantBuffer, nullptr);
        if (!Resources.RayTracingSceneConstantsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }

        Resources.RayTracingSceneConstantsBuffer->SetDebugName("RayTracing Scene Constants");
    }

    ASCacheBackend = MakeUniquePtr<FDiskAccelerationStructureCacheBackend>(Paths::GetAssetDir() + "/RTASCache");
    ASCache.SetBackend(ASCacheBackend.Get());

    if (RHI::bSupportsInlineRayTracing)
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_6, EShaderStage::Compute);
        if (FShaderCompiler::Get().CompileFromFile("Shaders/InlineReflections.hlsl", CompileInfo, Code))
        {
            InlineReflectionsShader = RHI::CreateComputeShader(Code);
            if (InlineReflectionsShader)
            {
                FRHIComputePipelineStateDesc PSODesc;
                PSODesc.Shader = InlineReflectionsShader.Get();

                InlineReflectionsPipeline = RHI::CreateComputePipelineState(PSODesc);
                if (InlineReflectionsPipeline)
                {
                    InlineReflectionsPipeline->SetDebugName("Inline RT Reflections PSO");
                }
            }
        }

        if (!InlineReflectionsPipeline)
        {
            LOG_WARNING("[RayTracer]: Inline ray tracing reflections pipeline unavailable. The inline path will be disabled");

            InlineReflectionsShader.Reset();
            InlineReflectionsPipeline.Reset();
        }

        FShaderCompileInfo PrimaryDebugInfo("Main", EShaderModel::SM_6_6, EShaderStage::Compute);
        if (FShaderCompiler::Get().CompileFromFile("Shaders/PrimaryRayDebug.hlsl", PrimaryDebugInfo, Code))
        {
            PrimaryRayDebugShader = RHI::CreateComputeShader(Code);
            if (PrimaryRayDebugShader)
            {
                FRHIComputePipelineStateDesc PSODesc;
                PSODesc.Shader = PrimaryRayDebugShader.Get();

                PrimaryRayDebugPipeline = RHI::CreateComputePipelineState(PSODesc);
                if (PrimaryRayDebugPipeline)
                {
                    PrimaryRayDebugPipeline->SetDebugName("RT Primary-Ray Debug PSO");
                }
            }
        }

        if (!PrimaryRayDebugPipeline)
        {
            LOG_WARNING("[RayTracer]: Primary-ray debug pipeline unavailable. The RT primary-ID debug view will be disabled");

            PrimaryRayDebugShader.Reset();
            PrimaryRayDebugPipeline.Reset();
        }
    }

    {
        FShaderCompileInfo TemporalInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (FShaderCompiler::Get().CompileFromFile("Shaders/Reflections/ReflectionTemporal.hlsl", TemporalInfo, Code))
        {
            ReflectionTemporalShader = RHI::CreateComputeShader(Code);
            if (ReflectionTemporalShader)
            {
                FRHIComputePipelineStateDesc PSODesc;
                PSODesc.Shader = ReflectionTemporalShader.Get();

                ReflectionTemporalPipeline = RHI::CreateComputePipelineState(PSODesc);
                if (ReflectionTemporalPipeline)
                {
                    ReflectionTemporalPipeline->SetDebugName("Reflection Temporal PSO");
                }
            }
        }

        FShaderCompileInfo AtrousInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (FShaderCompiler::Get().CompileFromFile("Shaders/Reflections/ReflectionAtrous.hlsl", AtrousInfo, Code))
        {
            ReflectionAtrousShader = RHI::CreateComputeShader(Code);
            if (ReflectionAtrousShader)
            {
                FRHIComputePipelineStateDesc PSODesc;
                PSODesc.Shader = ReflectionAtrousShader.Get();

                ReflectionAtrousPipeline = RHI::CreateComputePipelineState(PSODesc);
                if (ReflectionAtrousPipeline)
                {
                    ReflectionAtrousPipeline->SetDebugName("Reflection A-Trous PSO");
                }
            }
        }

        FShaderCompileInfo UpsampleInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (FShaderCompiler::Get().CompileFromFile("Shaders/Reflections/ReflectionUpsample.hlsl", UpsampleInfo, Code))
        {
            ReflectionUpsampleShader = RHI::CreateComputeShader(Code);
            if (ReflectionUpsampleShader)
            {
                FRHIComputePipelineStateDesc PSODesc;
                PSODesc.Shader = ReflectionUpsampleShader.Get();

                ReflectionUpsamplePipeline = RHI::CreateComputePipelineState(PSODesc);
                if (ReflectionUpsamplePipeline)
                {
                    ReflectionUpsamplePipeline->SetDebugName("Reflection Upsample PSO");
                }
            }
        }

        if (!ReflectionTemporalPipeline || !ReflectionAtrousPipeline)
        {
            LOG_WARNING("[RayTracer]: Reflection denoiser pipelines unavailable. Reflections will use the raw trace");
            
            ReflectionTemporalPipeline.Reset();
            ReflectionTemporalShader.Reset();
            ReflectionAtrousPipeline.Reset();
            ReflectionAtrousShader.Reset();
        }
    }

    LoadReflectionNoiseMask();

    if (!CreateResources(Resources, Resources.CurrentRenderWidth, Resources.CurrentRenderHeight))
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

bool FRayTracer::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    if (Width == 0 || Height == 0)
    {
        return false;
    }

    const ETextureUsageFlags UAVAndSRV = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::RayTracingOutputFormat, Width, Height, 1, 1, UAVAndSRV);
    Resources.RayTracingOutput = RHI::CreateTexture(OutputDesc, ERHIResourceState::UnorderedAccess);

    if (!Resources.RayTracingOutput)
    {
        return false;
    }

    Resources.RayTracingOutput->SetDebugName("RayTracing Output");

    bDenoiserHalfRes   = GReflectionHalfRes && (ReflectionUpsamplePipeline != nullptr);
    DenoiserFullWidth  = Width;
    DenoiserFullHeight = Height;

    const uint32 ChainWidth  = bDenoiserHalfRes ? Math::Max<uint32>((Width + 1u) / 2u, 1u) : Width;
    const uint32 ChainHeight = bDenoiserHalfRes ? Math::Max<uint32>((Height + 1u) / 2u, 1u) : Height;

    const auto CreateTarget = [&](EFormat Format, const CHAR* Name) -> FRHITextureRef
    {
        FRHITextureDesc Desc = FRHITextureDesc::CreateTexture2D(Format, ChainWidth, ChainHeight, 1, 1, UAVAndSRV);
        FRHITextureRef Texture = RHI::CreateTexture(Desc, ERHIResourceState::UnorderedAccess);
        if (Texture)
        {
            Texture->SetDebugName(Name);
        }
        return Texture;
    };

    Resources.ReflectionTrace = CreateTarget(EFormat::R16G16B16A16_Float, "Reflection Trace");
    if (!Resources.ReflectionTrace)
    {
        return false;
    }

    const CHAR* HistoryNames[2]  = { "Reflection History 0",  "Reflection History 1"  };
    const CHAR* MomentsNames[2]  = { "Reflection Moments 0",  "Reflection Moments 1"  };
    const CHAR* DenoisedNames[2] = { "Reflection Denoised 0", "Reflection Denoised 1" };

    for (int32 Index = 0; Index < 2; ++Index)
    {
        Resources.ReflectionHistory[Index]  = CreateTarget(EFormat::R16G16B16A16_Float, HistoryNames[Index]);
        Resources.ReflectionMoments[Index]  = CreateTarget(EFormat::R16G16_Float,       MomentsNames[Index]);
        Resources.ReflectionDenoised[Index] = CreateTarget(EFormat::R16G16B16A16_Float, DenoisedNames[Index]);

        if (!Resources.ReflectionHistory[Index] || !Resources.ReflectionMoments[Index] || !Resources.ReflectionDenoised[Index])
        {
            return false;
        }
    }

    bReflectionHistoryValid = false;
    return true;
}

bool FRayTracer::NeedsReflectionReconfigure() const
{
    const bool bWantHalfRes = GReflectionHalfRes && (ReflectionUpsamplePipeline != nullptr);
    return bWantHalfRes != bDenoiserHalfRes;
}

void FRayTracer::LoadReflectionNoiseMask()
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
        LOG_WARNING("[RayTracer]: Failed to load reflection noise mask '%s'. The blue noise sampler will fall back to white noise", *FullPath);
        return;
    }

    FTexture2D* Texture2D = ReflectionNoiseTexture->GetTexture2D();
    if (!Texture2D || (Texture2D->GetWidth() != Texture2D->GetHeight()))
    {
        LOG_WARNING("[RayTracer]: Reflection noise mask '%s' is not square. The blue noise sampler will fall back to white noise", *FullPath);

        ReflectionNoiseTexture.Reset();
        return;
    }

    ReflectionNoiseSize = Texture2D->GetWidth();
    LOG_INFO("[RayTracer]: Loaded reflection noise mask '%s' (%ux%u)", *FullPath, ReflectionNoiseSize, ReflectionNoiseSize);
}

FRHITexture* FRayTracer::GetReflectionNoiseMask() const
{
    if (!ReflectionNoiseTexture)
    {
        return nullptr;
    }

    FTexture2D* Texture2D = ReflectionNoiseTexture->GetTexture2D();
    return Texture2D ? Texture2D->GetRHITexture().Get() : nullptr;
}

void FRayTracer::Release()
{
    LocalPipeline.Reset();
    RayGenShader.Reset();
    RayMissShader.Reset();
    RayClosestHitShader.Reset();

    BindlessPipeline.Reset();
    RayGenShaderBindless.Reset();
    RayMissShaderBindless.Reset();
    RayClosestHitShaderBindless.Reset();

    SERPipeline.Reset();
    RayGenShaderSER.Reset();
    RayMissShaderSER.Reset();
    RayClosestHitShaderSER.Reset();

    InlineReflectionsPipeline.Reset();
    InlineReflectionsShader.Reset();

    PrimaryRayDebugPipeline.Reset();
    PrimaryRayDebugShader.Reset();

    ReflectionTemporalPipeline.Reset();
    ReflectionTemporalShader.Reset();
    ReflectionAtrousPipeline.Reset();
    ReflectionAtrousShader.Reset();
    ReflectionUpsamplePipeline.Reset();
    ReflectionUpsampleShader.Reset();
    bReflectionHistoryValid = false;

    ReflectionNoiseTexture.Reset();
    ReflectionNoiseSize = 0;

    CompactionHelper.ReleaseAll();
    SerializationHelper.ReleaseAll();
    CompactionRequested.Clear();
    SerializationRequested.Clear();
    ASCache.SetBackend(nullptr);
    ASCacheBackend.Reset();
}

void FRayTracer::ReleaseRayTracingGeometry(FScene* Scene)
{
    if (Scene)
    {
        for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
        {
            if (StaticMesh && StaticMesh->Mesh)
            {
                StaticMesh->Mesh->ReleaseRayTracingGeometry();
            }
        }
    }

    CompactionRequested.Clear();
    SerializationRequested.Clear();
}

void FRayTracer::BuildSceneAccelerationData(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bNeedBindlessData)
{
    TRACE_SCOPE("Gather Instances");

    Resources.RayTracingGeometryInstances.Clear();
    Resources.RayTracingHitGroupBindings.Clear();
    Resources.RayTracingGeometryTableData.Clear();

    uint32 NextInstanceIndex       = 0;
    uint32 LazyBLASBuildsThisFrame = 0;
    uint32 SkippedNullGeometry     = 0;

    for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
    {
        TSharedPtr<FMaterial> Material = StaticMesh->GetMaterial();
        if (Material->HasAlphaMask())
        {
            continue;
        }

        const bool bHadGeometry = StaticMesh->Mesh->GetRayTracingGeometry() != nullptr;
        StaticMesh->Mesh->CreateRayTracingGeometry();

        FRHIGeometryAccelerationStructure* Geometry = StaticMesh->Mesh->GetRayTracingGeometry();
        if (!Geometry)
        {
            ++SkippedNullGeometry;
            continue;
        }

        if (!bHadGeometry)
        {
            ++LazyBLASBuildsThisFrame;
        }

        FRHIShaderResourceView* AlbedoSRV   = SafeGetDefaultSRV(Material->AlbedoMap);
        FRHIShaderResourceView* NormalSRV   = SafeGetDefaultSRV(Material->NormalMap);
        FRHIShaderResourceView* MaterialSRV = SafeGetDefaultSRV(Material->MaterialMap);

        if (FEngine* Engine = FEngine::Get())
        {
            if (!AlbedoSRV && Engine->BaseTexture)
            {
                AlbedoSRV = Engine->BaseTexture->GetShaderResourceView();
            }

            if (!NormalSRV && Engine->BaseNormal)
            {
                NormalSRV = Engine->BaseNormal->GetShaderResourceView();
            }

            if (!MaterialSRV && Engine->BaseTexture)
            {
                MaterialSRV = Engine->BaseTexture->GetShaderResourceView();
            }
        }

        const uint32 InstanceIndex = NextInstanceIndex++;

        FRHIShaderResourceView* VertexBufferSRV = StaticMesh->Mesh->GetVertexBufferSRV(EVertexStream::Packed);
        FRHIShaderResourceView* IndexBufferSRV  = StaticMesh->Mesh->GetIndexBufferSRV();

        const Matrix3x4 InstanceTransform = StaticMesh->PerObjectBuffer.Transform;

        uint32 HitGroupIndex = 0;
        if (uint32* ExistingIndex = Resources.RayTracingMeshToHitGroupIndex.Find(StaticMesh->Mesh.Get()))
        {
            HitGroupIndex = *ExistingIndex;
        }
        else
        {
            HitGroupIndex = Resources.RayTracingHitGroupBindings.Size();
            Resources.RayTracingMeshToHitGroupIndex[StaticMesh->Mesh.Get()] = HitGroupIndex;

            TArray<FRHIHitGroupLocalShaderBinding>& Record = Resources.RayTracingHitGroupBindings.Emplace();
            if (!bNeedBindlessData)
            {
                if (VertexBufferSRV)
                {
                    Record.Add(FRHIHitGroupLocalShaderBinding::MakeShaderResourceView(VertexBufferSRV, 0));
                }

                if (IndexBufferSRV)
                {
                    Record.Add(FRHIHitGroupLocalShaderBinding::MakeShaderResourceView(IndexBufferSRV, 1));
                }

                Record.Add(FRHIHitGroupLocalShaderBinding::MakeShaderResourceView(AlbedoSRV, 2));
                Record.Add(FRHIHitGroupLocalShaderBinding::MakeShaderResourceView(NormalSRV, 3));
                Record.Add(FRHIHitGroupLocalShaderBinding::MakeShaderResourceView(MaterialSRV, 4));
                Record.Add(FRHIHitGroupLocalShaderBinding::MakeSamplerState(Material->GetMaterialSampler(), 0));
            }
        }

        {
            FRayTracingGeometryIndicesHLSL Geo;
            if (VertexBufferSRV)
            {
                Geo.VerticesHandle = VertexBufferSRV->GetBindlessHandle();
            }

            if (IndexBufferSRV)
            {
                Geo.IndicesHandle = IndexBufferSRV->GetBindlessHandle();
            }
            Geo.MaterialIndex   = Material->GetBufferIndex();
            Geo.DeterminantSign = StaticMesh->PerObjectBuffer.DeterminantSign;

            if (int32(InstanceIndex) >= Resources.RayTracingGeometryTableData.Size())
            {
                Resources.RayTracingGeometryTableData.Resize(int32(InstanceIndex) + 1);
            }

            Resources.RayTracingGeometryTableData[InstanceIndex] = Geo;
        }

        FRHIGeometryAccelerationStructureInstance Instance;
        Instance.Geometry      = Geometry;
        Instance.Flags         = Material->IsDoubleSided() ? ERayTracingInstanceFlags::CullDisable : ERayTracingInstanceFlags::None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = InstanceIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = InstanceTransform;
        Resources.RayTracingGeometryInstances.Emplace(Instance);
    }

    STAT_SET(STAT_RT_Active,                  1);
    STAT_SET(STAT_RT_InstanceCount,           Resources.RayTracingGeometryInstances.Size());
    STAT_SET(STAT_RT_HitGroupCount,           Resources.RayTracingHitGroupBindings.Size());
    STAT_SET(STAT_RT_GeometryTableRows,       Resources.RayTracingGeometryTableData.Size());
    STAT_SET(STAT_RT_LazyBLASBuildsThisFrame, LazyBLASBuildsThisFrame);
    STAT_SET(STAT_RT_SkippedNullGeometry,     SkippedNullGeometry);

    if (!Resources.RayTracingScene)
    {
        FRHISceneAccelerationStructureDesc SceneDesc(MakeArrayView(Resources.RayTracingGeometryInstances), EAccelerationStructureBuildFlags::None);
        Resources.RayTracingScene = RHI::CreateSceneAccelerationStructure(SceneDesc);
        if (Resources.RayTracingScene)
        {
            Resources.RayTracingScene->SetDebugName("RayTracing Scene (TLAS)");
        }
    }
    else
    {
        FRHISceneAccelerationStructureBuildDesc BuildSceneDesc;
        BuildSceneDesc.Instances    = Resources.RayTracingGeometryInstances.Data();
        BuildSceneDesc.NumInstances = Resources.RayTracingGeometryInstances.Size();
        BuildSceneDesc.bUpdate      = false;

        CommandList.BuildSceneAccelerationStructure(Resources.RayTracingScene.Get(), BuildSceneDesc);
    }

    if (GRayTracingCompaction || GRayTracingASCache)
    {
        for (const FRHIGeometryAccelerationStructureInstance& Instance : Resources.RayTracingGeometryInstances)
        {
            FRHIGeometryAccelerationStructure* Geometry = Instance.Geometry;
            if (!Geometry)
            {
                continue;
            }

            if (GRayTracingCompaction && !CompactionRequested.Contains(Geometry))
            {
                CompactionHelper.RequestCompaction(CommandList, Geometry);
                CompactionRequested.Emplace(Geometry);
            }

            if (GRayTracingASCache && ASCache.HasBackend() && !SerializationRequested.Contains(Geometry))
            {
                CHAR KeyBuffer[32] = {};
                CString::Snprintf(KeyBuffer, int32(sizeof(KeyBuffer)), "blas_%llx", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(Geometry)));

                SerializationHelper.RequestSerialize(CommandList, Geometry, String(KeyBuffer));
                SerializationRequested.Emplace(Geometry);
            }
        }
    }

    if (CompactionHelper.HasPendingWork())
    {
        CompactionHelper.Tick(CommandList);
    }

    if (SerializationHelper.HasPendingWork())
    {
        SerializationHelper.Tick(CommandList);
    }

    if (!Resources.RayTracingGeometryTableData.IsEmpty())
    {
        const uint32 RequiredCount = uint32(Resources.RayTracingGeometryTableData.Size());
        if (!Resources.RayTracingGeometryTableBuffer || RequiredCount > GeometryTableCapacity)
        {
            GeometryTableCapacity = RequiredCount;

            FRHIBufferDesc TableDesc;
            TableDesc.Stride = sizeof(FRayTracingGeometryIndicesHLSL);
            TableDesc.Size   = TableDesc.Stride * GeometryTableCapacity;
            TableDesc.Flags  = EBufferFlags::ShaderResourceBuffer | EBufferFlags::Dynamic;

            Resources.RayTracingGeometryTableBuffer = RHI::CreateBuffer(TableDesc, ERHIResourceState::GenericRead, nullptr);
            Resources.RayTracingGeometryTableSRV    = nullptr;

            if (Resources.RayTracingGeometryTableBuffer)
            {
                Resources.RayTracingGeometryTableBuffer->SetDebugName("RayTracing Geometry Table");

                FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, GeometryTableCapacity);
                Resources.RayTracingGeometryTableSRV = RHI::CreateShaderResourceView(Resources.RayTracingGeometryTableBuffer.Get(), SRVDesc);
            }
        }

        if (Resources.RayTracingGeometryTableBuffer)
        {
            CommandList.UpdateBuffer(Resources.RayTracingGeometryTableBuffer.Get(), FBufferRegion(0, sizeof(FRayTracingGeometryIndicesHLSL) * RequiredCount), Resources.RayTracingGeometryTableData.Data());
        }
    }
}

void FRayTracer::PreRender(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    if (GRayTracingSER && !RHI::bSupportsShaderExecutionReordering)
    {
        static bool bLoggedSERUnsupported = false;
        if (!bLoggedSERUnsupported)
        {
            LOG_WARNING("[RayTracer]: Renderer.RayTracing.SER=1 requested but Shader Execution Reordering is unsupported on this backend. Falling back to the non-SER reflection path");
            bLoggedSERUnsupported = true;
        }
    }

    if (GRayTracingEnableLocalShaderBindings && !RHI::bSupportsShaderBindingTableDescriptors)
    {
        static bool bLoggedLocalBindingsUnsupported = false;
        if (!bLoggedLocalBindingsUnsupported)
        {
            LOG_WARNING("[RayTracer]: Renderer.RayTracing.EnableLocalShaderBindings=1 requested but this backend does not support SBT descriptor records. Falling back to the bindless reflection path");
            bLoggedLocalBindingsUnsupported = true;
        }
    }

    const bool bExplicitAvailable                  = RHI::bSupportsShaderBindingTableDescriptors && LocalPipeline;
    const bool bBindless                           = (!GRayTracingEnableLocalShaderBindings || !bExplicitAvailable) && BindlessPipeline;
    const bool bInline                             = GRayTracingInlineReflections && RHI::bSupportsInlineRayTracing && InlineReflectionsPipeline;
    const bool bIsShaderExecutionReorderingEnabled = !bInline && GRayTracingSER && RHI::bSupportsShaderExecutionReordering && SERPipeline;
    const bool bNeedBindlessData                   = bBindless || bInline || bIsShaderExecutionReorderingEnabled;
    const bool bDenoise                            = GReflectionDenoise && ReflectionTemporalPipeline && ReflectionAtrousPipeline && Resources.ReflectionTrace;

    FRHITexture* const TraceTarget  = bDenoise ? Resources.ReflectionTrace.Get() : Resources.RayTracingOutput.Get();
    FRHITexture*       DiffuseCube  = nullptr;
    FRHITexture*       SpecularCube = nullptr;

    BuildSceneAccelerationData(CommandList, Resources, Scene, bNeedBindlessData);

    if (!GReflectionsEnabled)
    {
        bReflectionHistoryValid = false;
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
        else if (Scene->GetSkybox())
        {
            DiffuseCube  = Scene->GetSkybox()->CubeMap.Get();
            SpecularCube = Scene->GetSkybox()->CubeMap.Get();

            Constants.NumSkyLightMips = Scene->GetSkybox()->CubeMap->GetDesc().NumMipLevels;
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

    if (Resources.IntegrationLUT)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Resources.IntegrationLUT.Get(), ERHIResourceState::NonPixelShaderResource));
    }

    if (DiffuseCube)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DiffuseCube, ERHIResourceState::NonPixelShaderResource));
    }

    if (SpecularCube)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SpecularCube, ERHIResourceState::NonPixelShaderResource));
    }

    if (FSceneSkybox* Skybox = Scene->GetSkybox())
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Skybox->CubeMap.Get(), ERHIResourceState::NonPixelShaderResource));
    }

    const FRHITransitionBarrierDesc GBufferToRead[] =
    {
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Normal].Get(), ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Depth].Get(), ERHIResourceState::NonPixelShaderResource),
        FRHITransitionBarrierDesc::CreateTexture(Resources.GBuffer[EGBufferIndex::Material].Get(), ERHIResourceState::NonPixelShaderResource),
    };

    CommandList.TransitionBarrier(GBufferToRead);

    FRHISamplerState* const EnvSampler = Resources.LightProbeSampler ? Resources.LightProbeSampler.Get() : Resources.GBufferSampler.Get();
    FRHISamplerState* const LUTSampler = Resources.IntegrationLUTSampler ? Resources.IntegrationLUTSampler.Get() : Resources.GBufferSampler.Get();

    FRHITexture* const NoiseMask = GetReflectionNoiseMask();
    if (NoiseMask)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(NoiseMask, ERHIResourceState::NonPixelShaderResource));
    }

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
            CommandList.SetShaderResourceView(Shader, Skybox->CubeMap->GetShaderResourceView(), 1);
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

    if (bInline)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(TraceTarget, ERHIResourceState::UnorderedAccess));

        CommandList.SetComputePipelineState(InlineReflectionsPipeline.Get());
        BindReflectionGlobals(InlineReflectionsShader.Get());

        const uint32 InlineWidth  = TraceTarget->GetDesc().Extent.X;
        const uint32 InlineHeight = TraceTarget->GetDesc().Extent.Y;

        constexpr uint32 ThreadCount = 8;
        const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(InlineWidth, ThreadCount);
        const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(InlineHeight, ThreadCount);

        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        CommandList.UnorderedAccessBarrier(TraceTarget);

        if (bDenoise)
        {
            DenoiseReflections(CommandList, Resources);
        }
        return;
    }

    FRHIRayTracingPipelineState* ActivePipeline           = bIsShaderExecutionReorderingEnabled ? SERPipeline.Get() : (bBindless ? BindlessPipeline.Get() : LocalPipeline.Get());
    FRHIShaderBindingTableRef&   ActiveShaderBindingTable = bIsShaderExecutionReorderingEnabled ? Resources.RayTracingSERShaderBindingTable : (bBindless ? Resources.RayTracingBindlessShaderBindingTable : Resources.RayTracingShaderBindingTable);
    uint32&                      ActiveCapacity           = bIsShaderExecutionReorderingEnabled ? CurrentSERHitGroupCapacity : (bBindless ? CurrentBindlessHitGroupCapacity : CurrentHitGroupCapacity);

    if (!ActivePipeline)
    {
        static bool bLoggedNoActivePipeline = false;
        
        if (!bLoggedNoActivePipeline)
        {
            LOG_WARNING("[RayTracer]: No usable ray tracing reflection pipeline for the selected path. Skipping the RT reflection pass");
            bLoggedNoActivePipeline = true;
        }

        return;
    }

    const uint32 NumHitGroupRecords = Resources.RayTracingHitGroupBindings.Size();
    if (!ActiveShaderBindingTable || NumHitGroupRecords > ActiveCapacity)
    {
        FRHIShaderBindingTableDesc SBTDesc(ActivePipeline, 1, 1, 0, NumHitGroupRecords);
        ActiveShaderBindingTable      = RHI::CreateShaderBindingTable(SBTDesc);
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

    CommandList.BuildShaderBindingTable(ShaderBindingTable);

    CommandList.SetRayTracingPipelineState(ActivePipeline);

    FRHIRayGenShader* RayGenShaderForGlobals = bIsShaderExecutionReorderingEnabled ? RayGenShaderSER.Get() : (bBindless ? RayGenShaderBindless.Get() : RayGenShader.Get());
    BindReflectionGlobals(RayGenShaderForGlobals);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(TraceTarget, ERHIResourceState::UnorderedAccess));

    const uint32 Width  = TraceTarget->GetDesc().Extent.X;
    const uint32 Height = TraceTarget->GetDesc().Extent.Y;
    CommandList.DispatchRays(ShaderBindingTable, Width, Height, 1);

    CommandList.UnorderedAccessBarrier(TraceTarget);

    if (bDenoise)
    {
        DenoiseReflections(CommandList, Resources);
    }
}

void FRayTracer::RenderPrimaryRayDebug(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "RT Primary-Ray Debug");

    if (!PrimaryRayDebugPipeline || !Resources.RayTracingOutput)
    {
        return;
    }

    BuildSceneAccelerationData(CommandList, Resources, Scene, /*bNeedBindlessData=*/true);

    if (!Resources.RayTracingScene)
    {
        return;
    }

    FRHITexture* Output = Resources.RayTracingOutput.Get();
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Output, ERHIResourceState::UnorderedAccess));

    CommandList.SetComputePipelineState(PrimaryRayDebugPipeline.Get());
    CommandList.SetConstantBuffer(PrimaryRayDebugShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetShaderResourceView(PrimaryRayDebugShader.Get(), Resources.RayTracingScene->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(PrimaryRayDebugShader.Get(), Output->GetUnorderedAccessView(), 0);

    const uint32 Width  = Output->GetDesc().Extent.X;
    const uint32 Height = Output->GetDesc().Extent.Y;

    constexpr uint32 ThreadCount = 8;
    const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
    CommandList.UnorderedAccessBarrier(Output);
}

void FRayTracer::DenoiseReflections(FRHICommandList& CommandList, FFrameResources& Resources)
{
    RHI_EVENT_SCOPE(CommandList, "RT Reflection Denoise");

    const uint32 Width  = Resources.ReflectionTrace->GetDesc().Extent.X;
    const uint32 Height = Resources.ReflectionTrace->GetDesc().Extent.Y;

    constexpr uint32 ThreadCount = 8;
    const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

    FRHITexture* Normal   = Resources.GBuffer[EGBufferIndex::Normal].Get();
    FRHITexture* Depth    = Resources.GBuffer[EGBufferIndex::Depth].Get();
    FRHITexture* Velocity = Resources.GBuffer[EGBufferIndex::Velocity].Get();

    const uint32 PrevIndex = ReflectionHistoryIndex;
    const uint32 CurIndex  = ReflectionHistoryIndex ^ 1u;

    FRHITexture* HistoryPrev = Resources.ReflectionHistory[PrevIndex].Get();
    FRHITexture* HistoryCur  = Resources.ReflectionHistory[CurIndex].Get();
    FRHITexture* MomentsPrev = Resources.ReflectionMoments[PrevIndex].Get();
    FRHITexture* MomentsCur  = Resources.ReflectionMoments[CurIndex].Get();
    FRHITexture* DenoisedA   = Resources.ReflectionDenoised[0].Get();
    FRHITexture* DenoisedB   = Resources.ReflectionDenoised[1].Get();

    const int32 NumIterations = Math::Clamp<int32>(GReflectionAtrousIterations, 0, 8);
    const bool  bHalfRes      = bDenoiserHalfRes;

    FRHITexture* TemporalTarget = (!bHalfRes && NumIterations <= 0) ? Resources.RayTracingOutput.Get() : DenoisedA;
    FRHITexture* ChainResult    = TemporalTarget;

    const auto RequireSRV = [&](FRHITexture* Texture)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::NonPixelShaderResource));
    };

    const auto RequireUAV = [&](FRHITexture* Texture)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::UnorderedAccess));
    };

    // -----------------------------------------------------------------------------------------
    // Temporal accumulation
    // -----------------------------------------------------------------------------------------

    {
        RHI_EVENT_SCOPE(CommandList, "Temporal");

        RequireSRV(Resources.ReflectionTrace.Get());
        RequireSRV(Normal);
        RequireSRV(Depth);
        RequireSRV(Velocity);
        RequireSRV(HistoryPrev);
        RequireSRV(MomentsPrev);
        RequireUAV(HistoryCur);
        RequireUAV(MomentsCur);
        RequireUAV(TemporalTarget);

        FRHIComputeShader* Shader = ReflectionTemporalShader.Get();
        CommandList.SetComputePipelineState(ReflectionTemporalPipeline.Get());

        CommandList.SetShaderResourceView(Shader, Resources.ReflectionTrace->GetShaderResourceView(), 0);
        CommandList.SetShaderResourceView(Shader, Normal->GetShaderResourceView(), 1);
        CommandList.SetShaderResourceView(Shader, Depth->GetShaderResourceView(), 2);
        CommandList.SetShaderResourceView(Shader, Velocity->GetShaderResourceView(), 3);
        CommandList.SetShaderResourceView(Shader, HistoryPrev->GetShaderResourceView(), 4);
        CommandList.SetShaderResourceView(Shader, MomentsPrev->GetShaderResourceView(), 5);
        CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
        CommandList.SetUnorderedAccessView(Shader, HistoryCur->GetUnorderedAccessView(), 0);
        CommandList.SetUnorderedAccessView(Shader, MomentsCur->GetUnorderedAccessView(), 1);
        CommandList.SetUnorderedAccessView(Shader, TemporalTarget->GetUnorderedAccessView(), 2);
        CommandList.SetConstantBuffer(Shader, Resources.CameraBuffer.Get(), 0);

        struct FTemporalConstants
        {
            float    ScreenSize[2];
            float    TemporalAlpha;
            float    MaxRadiance;
            float    HistoryClampGamma;
            float    MaxHistoryLength;
            int32    NeighborhoodRadius;
            float    CameraMotionMaxHistory;
        } Constants;

        Constants.ScreenSize[0]          = float(Width);
        Constants.ScreenSize[1]          = float(Height);
        Constants.TemporalAlpha          = bReflectionHistoryValid ? GReflectionTemporalAlpha : 1.0f;
        Constants.MaxRadiance            = GReflectionMaxRadiance;
        Constants.HistoryClampGamma      = GReflectionHistoryClampGamma;
        Constants.MaxHistoryLength       = Math::Max(1.0f, GReflectionMaxHistoryLength);
        Constants.NeighborhoodRadius     = Math::Max(0, GReflectionNeighborhoodRadius);
        Constants.CameraMotionMaxHistory = GReflectionCameraMotionMaxHistory;

        CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        CommandList.UnorderedAccessBarrier(TemporalTarget);

        RequireSRV(HistoryCur);
        RequireSRV(MomentsCur);
    }

    // -----------------------------------------------------------------------------------------
    // Spatial SVGF a-trous
    // -----------------------------------------------------------------------------------------

    if (NumIterations > 0)
    {
        RHI_EVENT_SCOPE(CommandList, "A-Trous");

        FRHIComputeShader* Shader = ReflectionAtrousShader.Get();

        FRHITexture* Source = DenoisedA;
        for (int32 Iteration = 0; Iteration < NumIterations; ++Iteration)
        {
            const bool bLast = (Iteration == NumIterations - 1);
            FRHITexture* Dest = (bLast && !bHalfRes) 
                ? Resources.RayTracingOutput.Get() 
                : ((Source == DenoisedA) ? DenoisedB : DenoisedA);

            RequireSRV(Source);
            RequireSRV(Normal);
            RequireSRV(Depth);
            RequireUAV(Dest);

            CommandList.SetComputePipelineState(ReflectionAtrousPipeline.Get());
            CommandList.SetShaderResourceView(Shader, Source->GetShaderResourceView(), 0);
            CommandList.SetShaderResourceView(Shader, Normal->GetShaderResourceView(), 1);
            CommandList.SetShaderResourceView(Shader, Depth->GetShaderResourceView(), 2);
            CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
            CommandList.SetUnorderedAccessView(Shader, Dest->GetUnorderedAccessView(), 0);
            CommandList.SetConstantBuffer(Shader, Resources.CameraBuffer.Get(), 0);

            struct FAtrousConstants
            {
                float ScreenSize[2];
                int32 StepSize;
                float PhiColor;
            } Constants;

            Constants.ScreenSize[0] = float(Width);
            Constants.ScreenSize[1] = float(Height);
            Constants.StepSize      = 1 << Iteration;
            Constants.PhiColor      = Math::Max(0.0f, GReflectionAtrousPhiColor);

            CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

            CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
            CommandList.UnorderedAccessBarrier(Dest);

            Source = Dest;
        }

        ChainResult = Source;
    }

    if (bHalfRes)
    {
        RHI_EVENT_SCOPE(CommandList, "Upsample");

        FRHITexture* ChainFinal = ChainResult;
        FRHITexture* FullOutput = Resources.RayTracingOutput.Get();

        RequireSRV(ChainFinal);
        RequireSRV(Normal);
        RequireSRV(Depth);
        RequireUAV(FullOutput);

        FRHIComputeShader* Shader = ReflectionUpsampleShader.Get();
        CommandList.SetComputePipelineState(ReflectionUpsamplePipeline.Get());
        CommandList.SetShaderResourceView(Shader, ChainFinal->GetShaderResourceView(), 0);
        CommandList.SetShaderResourceView(Shader, Normal->GetShaderResourceView(), 1);
        CommandList.SetShaderResourceView(Shader, Depth->GetShaderResourceView(), 2);
        CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
        CommandList.SetUnorderedAccessView(Shader, FullOutput->GetUnorderedAccessView(), 0);

        const uint32 FullWidth  = FullOutput->GetDesc().Extent.X;
        const uint32 FullHeight = FullOutput->GetDesc().Extent.Y;

        struct FUpsampleConstants
        {
            float FullSize[2];
            float HalfSize[2];
        } Constants;

        Constants.FullSize[0] = float(FullWidth);
        Constants.FullSize[1] = float(FullHeight);
        Constants.HalfSize[0] = float(Width);
        Constants.HalfSize[1] = float(Height);

        CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

        CommandList.Dispatch(Math::DivideByMultiple<uint32>(FullWidth, ThreadCount), Math::DivideByMultiple<uint32>(FullHeight, ThreadCount), 1);
        CommandList.UnorderedAccessBarrier(FullOutput);
    }

    bReflectionHistoryValid = true;
    ReflectionHistoryIndex  = CurIndex;

    Resources.ReflectionHistoryIndex = CurIndex;
}
