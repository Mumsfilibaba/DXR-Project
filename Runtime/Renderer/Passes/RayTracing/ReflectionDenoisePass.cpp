#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Renderer/Settings/ReflectionSettings.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RayTracing/ReflectionDenoisePass.h"
#include "Renderer/Shaders/RayTracingShaders.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static bool GReflectionDenoise = true;
static FAutoConsoleVariableRef CVarReflectionDenoise(
    TEXT("Renderer.RayTracing.Reflections.Denoise"),
    TEXT("Enable the reflection denoiser (temporal accumulation + SVGF a-trous). Off = raw 1-spp reflection trace."),
    GReflectionDenoise);

static int32 GReflectionAtrousIterations = 4;
static FAutoConsoleVariableRef CVarReflectionAtrousIterations(
    TEXT("Renderer.RayTracing.Reflections.AtrousIterations"),
    TEXT("Number of SVGF a-trous spatial-filter iterations (0 disables the spatial filter)."),
    GReflectionAtrousIterations);

static float GReflectionTemporalAlpha = 0.1f;
static FAutoConsoleVariableRef CVarReflectionTemporalAlpha(
    TEXT("Renderer.RayTracing.Reflections.TemporalAlpha"),
    TEXT("Minimum temporal blend toward the current frame (1 = no history, lower = more accumulation)."),
    GReflectionTemporalAlpha);

static float GReflectionMaxRadiance = 8.0f;
static FAutoConsoleVariableRef CVarReflectionMaxRadiance(
    TEXT("Renderer.RayTracing.Reflections.MaxRadiance"),
    TEXT("Luminance clamp applied to each reflection sample before temporal accumulation to suppress fireflies (<= 0 disables the clamp)."),
    GReflectionMaxRadiance);

static float GReflectionHistoryClampGamma = 1.0f;
static FAutoConsoleVariableRef CVarReflectionHistoryClampGamma(
    TEXT("Renderer.RayTracing.Reflections.HistoryClampGamma"),
    TEXT("Width (in neighborhood std-devs) of the temporal history clamp used to suppress motion ghosting. Lower = tighter/less ghosting but "
    "more noise. <= 0 disables history rectification."),
    GReflectionHistoryClampGamma);

float GReflectionMaxHistoryLength = 32.0f;
static FAutoConsoleVariableRef CVarReflectionMaxHistoryLength(
    TEXT("Renderer.RayTracing.Reflections.MaxHistoryLength"),
    TEXT("Maximum number of frames of reflection temporal accumulation. Shorter = faster response / less ghosting, longer = smoother / more "
    "ghosting."),
    GReflectionMaxHistoryLength);

static int32 GReflectionNeighborhoodRadius = 4;
static FAutoConsoleVariableRef CVarReflectionNeighborhoodRadius(
    TEXT("Renderer.RayTracing.Reflections.NeighborhoodRadius"),
    TEXT("Half-window (in texels) of the current-frame neighborhood used to build the temporal history color clamp box."),
    GReflectionNeighborhoodRadius);

static float GReflectionCameraMotionMaxHistory = 8.0f;
static FAutoConsoleVariableRef CVarReflectionCameraMotionMaxHistory(
    TEXT("Renderer.RayTracing.Reflections.CameraMotionMaxHistory"),
    TEXT("Accumulation cap (frames) applied while the camera is translating, so history decays quickly and does not smear. <= 0 disables the "
    "camera-motion reset."),
    GReflectionCameraMotionMaxHistory);

static bool GReflectionHalfRes = false;
static FAutoConsoleVariableRef CVarReflectionHalfRes(
    TEXT("Renderer.RayTracing.Reflections.HalfRes"),
    TEXT("Trace + denoise reflections at half resolution and bilateral-upsample to full res (faster, softer)."),
    GReflectionHalfRes);

static float GReflectionAtrousPhiColor = 4.0f;
static FAutoConsoleVariableRef CVarReflectionAtrousPhiColor(
    TEXT("Renderer.RayTracing.Reflections.AtrousPhiColor"),
    TEXT("Color sensitivity of the SVGF a-trous edge-stopping function. Lower preserves more detail but keeps more noise."),
    GReflectionAtrousPhiColor);

// One literal per iteration, because the graph stores a pass name as a raw pointer
static const CHAR_T* ATROUS_PASS_NAMES[FReflectionDenoisePass::MaxAtrousIterations] =
{
    TEXT("ReflectionAtrous0"),
    TEXT("ReflectionAtrous1"),
    TEXT("ReflectionAtrous2"),
    TEXT("ReflectionAtrous3"),
    TEXT("ReflectionAtrous4"),
    TEXT("ReflectionAtrous5"),
    TEXT("ReflectionAtrous6"),
    TEXT("ReflectionAtrous7"),
};

constexpr uint32 DENOISE_THREAD_COUNT = 8;

struct FTemporalConstants
{
    float ScreenSize[2];
    float TemporalAlpha;
    float MaxRadiance;
    float HistoryClampGamma;
    float MaxHistoryLength;
    int32 NeighborhoodRadius;
    float CameraMotionMaxHistory;
};

struct FAtrousConstants
{
    float ScreenSize[2];
    int32 StepSize;
    float PhiColor;
};

struct FUpsampleConstants
{
    float FullSize[2];
    float HalfSize[2];
};

FReflectionDenoisePass::FReflectionDenoisePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , HistoryIndex(0)
    , bHistoryValid(false)
{
}

FReflectionDenoisePass::~FReflectionDenoisePass()
{
    Release();
}

bool FReflectionDenoisePass::Initialize(FFrameResources& /*Resources*/)
{
    const auto CreateComputePipeline = [](FRHIComputeShaderRef& OutShader, const CHAR_T* DebugName) -> FRHIComputePipelineStateRef
    {
        if (!OutShader)
        {
            return nullptr;
        }

        FRHIComputePipelineStateDesc PSODesc;
        PSODesc.Shader = OutShader.Get();

        FRHIComputePipelineStateRef Pipeline = RHI::CreateComputePipelineState(PSODesc);
        if (Pipeline)
        {
            Pipeline->SetDebugName(DebugName);
        }
        else
        {
            OutShader.Reset();
        }

        return Pipeline;
    };

    TemporalShader   = FShaderCache::Get().GetShader<FReflectionTemporalCS>();
    TemporalPipeline = CreateComputePipeline(TemporalShader, TEXT("Reflection Temporal PSO"));

    AtrousShader   = FShaderCache::Get().GetShader<FReflectionAtrousCS>();
    AtrousPipeline = CreateComputePipeline(AtrousShader, TEXT("Reflection A-Trous PSO"));

    UpsampleShader   = FShaderCache::Get().GetShader<FReflectionUpsampleCS>();
    UpsamplePipeline = CreateComputePipeline(UpsampleShader, TEXT("Reflection Upsample PSO"));

    if (!TemporalPipeline || !AtrousPipeline)
    {
        LOG_WARNING("[ReflectionDenoise]: Reflection denoiser pipelines unavailable. Reflections will use the raw trace");

        TemporalPipeline.Reset();
        TemporalShader.Reset();
        AtrousPipeline.Reset();
        AtrousShader.Reset();
    }

    return true;
}

void FReflectionDenoisePass::Release()
{
    TemporalPipeline.Reset();
    TemporalShader.Reset();
    AtrousPipeline.Reset();
    AtrousShader.Reset();
    UpsamplePipeline.Reset();
    UpsampleShader.Reset();
    bHistoryValid = false;
}

bool FReflectionDenoisePass::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    if (Width == 0 || Height == 0)
    {
        return false;
    }

    Resources.bReflectionHalfRes   = GReflectionHalfRes && (UpsamplePipeline != nullptr);
    Resources.ReflectionFullWidth  = Width;
    Resources.ReflectionFullHeight = Height;

    const uint32 ChainWidth  = Resources.GetReflectionChainWidth();
    const uint32 ChainHeight = Resources.GetReflectionChainHeight();

    const auto CreateTarget = [&](EFormat Format, const CHAR_T* Name) -> FRHITextureRef
    {
        FRHITextureDesc Desc = FRHITextureDesc::CreateTexture2D(Format, ChainWidth, ChainHeight, 1, 1,
            ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture,
            FClearValue(), ERHIResourceStateTrackingMode::Tracked);

        FRHITextureRef Texture = RHI::CreateTexture(Desc, ERHIResourceState::UnorderedAccess);
        if (Texture)
        {
            Texture->SetDebugName(Name);
        }
        return Texture;
    };

    const CHAR_T* HistoryNames[2]  = { TEXT("Reflection History 0"),  TEXT("Reflection History 1")  };
    const CHAR_T* MomentsNames[2]  = { TEXT("Reflection Moments 0"),  TEXT("Reflection Moments 1")  };
    const CHAR_T* DenoisedNames[2] = { TEXT("Reflection Denoised 0"), TEXT("Reflection Denoised 1") };

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

    bHistoryValid = false;
    return true;
}

bool FReflectionDenoisePass::NeedsReconfigure(const FFrameResources& Resources) const
{
    const bool bWantHalfRes = GReflectionHalfRes && (UpsamplePipeline != nullptr);
    return bWantHalfRes != Resources.bReflectionHalfRes;
}

bool FReflectionDenoisePass::IsDenoiseEnabled(const FFrameResources& Resources) const
{
    return GReflectionDenoise && TemporalPipeline && AtrousPipeline && Resources.ReflectionTrace;
}

void FReflectionDenoisePass::RecordTemporal(FRHICommandList& CommandList, FFrameResources& Resources, uint32 ReadIndex, uint32 WriteIndex, FRHITexture* Target)
{
    const CHAR_T* MissingResource = nullptr;

    const auto RequireResource = [&MissingResource](const void* Resource, const CHAR_T* Name)
    {
        if (!Resource && !MissingResource)
        {
            MissingResource = Name;
        }
    };

    RequireResource(TemporalPipeline.Get(), TEXT("TemporalPipeline"));
    RequireResource(TemporalShader.Get(), TEXT("TemporalShader"));
    RequireResource(Resources.ReflectionTrace.Get(), TEXT("ReflectionTrace"));
    RequireResource(Resources.GBuffer[EGBufferIndex::Normal].Get(), TEXT("GBuffer.Normal"));
    RequireResource(Resources.GBuffer[EGBufferIndex::Depth].Get(), TEXT("GBuffer.Depth"));
    RequireResource(Resources.GBuffer[EGBufferIndex::Velocity].Get(), TEXT("GBuffer.Velocity"));
    RequireResource(Resources.ReflectionHistory[ReadIndex].Get(), TEXT("ReflectionHistory[Read]"));
    RequireResource(Resources.ReflectionMoments[ReadIndex].Get(), TEXT("ReflectionMoments[Read]"));
    RequireResource(Resources.ReflectionHistory[WriteIndex].Get(), TEXT("ReflectionHistory[Write]"));
    RequireResource(Resources.ReflectionMoments[WriteIndex].Get(), TEXT("ReflectionMoments[Write]"));
    RequireResource(Resources.CameraBuffer.Get(), TEXT("CameraBuffer"));
    RequireResource(Target, TEXT("Target"));

    if (MissingResource)
    {
        static bool bLoggedMissingResource = false;

        if (!bLoggedMissingResource)
        {
            LOG_WARNING("[ReflectionDenoise]: %s has not been allocated. Skipping the temporal denoise step", MissingResource);
            bLoggedMissingResource = true;
        }

        return;
    }

    const uint32 Width  = Resources.ReflectionTrace->GetDesc().Extent.X;
    const uint32 Height = Resources.ReflectionTrace->GetDesc().Extent.Y;

    FRHIComputeShader* Shader = TemporalShader.Get();
    CommandList.SetComputePipelineState(TemporalPipeline.Get());

    CommandList.SetShaderResourceView(Shader, Resources.ReflectionTrace->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Velocity]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(Shader, Resources.ReflectionHistory[ReadIndex]->GetShaderResourceView(), 4);
    CommandList.SetShaderResourceView(Shader, Resources.ReflectionMoments[ReadIndex]->GetShaderResourceView(), 5);
    CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
    CommandList.SetUnorderedAccessView(Shader, Resources.ReflectionHistory[WriteIndex]->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(Shader, Resources.ReflectionMoments[WriteIndex]->GetUnorderedAccessView(), 1);
    CommandList.SetUnorderedAccessView(Shader, Target->GetUnorderedAccessView(), 2);
    CommandList.SetConstantBuffer(Shader, Resources.CameraBuffer.Get(), 0);

    FTemporalConstants Constants;
    Constants.ScreenSize[0]          = float(Width);
    Constants.ScreenSize[1]          = float(Height);
    Constants.TemporalAlpha          = bHistoryValid ? GReflectionTemporalAlpha : 1.0f;
    Constants.MaxRadiance            = GReflectionMaxRadiance;
    Constants.HistoryClampGamma      = GReflectionHistoryClampGamma;
    Constants.MaxHistoryLength       = Math::Max(1.0f, GReflectionMaxHistoryLength);
    Constants.NeighborhoodRadius     = Math::Max(0, GReflectionNeighborhoodRadius);
    Constants.CameraMotionMaxHistory = GReflectionCameraMotionMaxHistory;

    CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

    CommandList.Dispatch(Math::DivideByMultiple<uint32>(Width, DENOISE_THREAD_COUNT), Math::DivideByMultiple<uint32>(Height, DENOISE_THREAD_COUNT), 1);

    bHistoryValid = true;
    HistoryIndex  = WriteIndex;

    Resources.ReflectionHistoryIndex = WriteIndex;
}

void FReflectionDenoisePass::RecordAtrous(FRHICommandList& CommandList, FFrameResources& Resources, FRHITexture* Source, FRHITexture* Target, int32 Iteration)
{
    const uint32 Width  = Resources.ReflectionTrace->GetDesc().Extent.X;
    const uint32 Height = Resources.ReflectionTrace->GetDesc().Extent.Y;

    FRHIComputeShader* Shader = AtrousShader.Get();
    CommandList.SetComputePipelineState(AtrousPipeline.Get());

    CommandList.SetShaderResourceView(Shader, Source->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 2);
    CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
    CommandList.SetUnorderedAccessView(Shader, Target->GetUnorderedAccessView(), 0);
    CommandList.SetConstantBuffer(Shader, Resources.CameraBuffer.Get(), 0);

    FAtrousConstants Constants;
    Constants.ScreenSize[0] = float(Width);
    Constants.ScreenSize[1] = float(Height);
    Constants.StepSize      = 1 << Iteration;
    Constants.PhiColor      = Math::Max(0.0f, GReflectionAtrousPhiColor);

    CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

    CommandList.Dispatch(Math::DivideByMultiple<uint32>(Width, DENOISE_THREAD_COUNT), Math::DivideByMultiple<uint32>(Height, DENOISE_THREAD_COUNT), 1);
}

void FReflectionDenoisePass::RecordUpsample(FRHICommandList& CommandList, FFrameResources& Resources, FRHITexture* Source)
{
    FRHITexture* FullOutput = Resources.RayTracingOutput.Get();

    const uint32 HalfWidth  = Resources.ReflectionTrace->GetDesc().Extent.X;
    const uint32 HalfHeight = Resources.ReflectionTrace->GetDesc().Extent.Y;
    const uint32 FullWidth  = FullOutput->GetDesc().Extent.X;
    const uint32 FullHeight = FullOutput->GetDesc().Extent.Y;

    FRHIComputeShader* Shader = UpsampleShader.Get();
    CommandList.SetComputePipelineState(UpsamplePipeline.Get());

    CommandList.SetShaderResourceView(Shader, Source->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(Shader, Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 2);
    CommandList.SetSamplerState(Shader, Resources.GBufferSampler.Get(), 0);
    CommandList.SetUnorderedAccessView(Shader, FullOutput->GetUnorderedAccessView(), 0);

    FUpsampleConstants Constants;
    Constants.FullSize[0] = float(FullWidth);
    Constants.FullSize[1] = float(FullHeight);
    Constants.HalfSize[0] = float(HalfWidth);
    Constants.HalfSize[1] = float(HalfHeight);

    CommandList.SetShaderConstants(Shader, &Constants, sizeof(Constants) / sizeof(uint32));

    CommandList.Dispatch(Math::DivideByMultiple<uint32>(FullWidth, DENOISE_THREAD_COUNT), Math::DivideByMultiple<uint32>(FullHeight, DENOISE_THREAD_COUNT), 1);
}

void FReflectionDenoisePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, bool bTraceEnabled)
{
    FFrameResources& Resources = *Context.FrameResources;

    if (!bTraceEnabled)
    {
        // Nothing was traced this frame, so the history no longer describes the scene
        bHistoryValid = false;
        return;
    }

    const bool bResourcesRegistered = Context.ReflectionTrace && Context.RayTracingOutput
        && Context.ReflectionHistory[0]  && Context.ReflectionHistory[1]
        && Context.ReflectionMoments[0]  && Context.ReflectionMoments[1]
        && Context.ReflectionDenoised[0] && Context.ReflectionDenoised[1];

    if (!IsDenoiseEnabled(Resources) || !bResourcesRegistered)
    {
        return;
    }

    const uint32 ReadIndex  = HistoryIndex;
    const uint32 WriteIndex = HistoryIndex ^ 1u;

    const int32 NumIterations = Math::Clamp<int32>(GReflectionAtrousIterations, 0, MaxAtrousIterations);
    const bool  bHalfRes      = Resources.bReflectionHalfRes;

    // With no spatial filter and no upsample to follow, the temporal step already produces the final image
    FRenderGraphTexture* const TemporalTarget = (!bHalfRes && NumIterations <= 0) ? Context.RayTracingOutput : Context.ReflectionDenoised[0];

    GraphBuilder.AddPass(TEXT("ReflectionTemporal"), ERenderGraphPassFlags::Compute, true,
        [&Context, ReadIndex, WriteIndex, TemporalTarget](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.ReflectionTrace, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferVelocity, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.ReflectionHistory[ReadIndex], ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.ReflectionMoments[ReadIndex], ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.ReflectionHistory[WriteIndex], ERHIResourceState::UnorderedAccess);
            PassBuilder.WriteTexture(Context.ReflectionMoments[WriteIndex], ERHIResourceState::UnorderedAccess);
            PassBuilder.WriteTexture(TemporalTarget, ERHIResourceState::UnorderedAccess);
        },
        [this, Context, ReadIndex, WriteIndex, TemporalTarget](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordTemporal(PassCommandList, *Context.FrameResources, ReadIndex, WriteIndex, TemporalTarget->GetRHITexture());
        });

    FRenderGraphTexture* Source = Context.ReflectionDenoised[0];
    for (int32 Iteration = 0; Iteration < NumIterations; ++Iteration)
    {
        const bool bIsLast = (Iteration == NumIterations - 1);

        FRenderGraphTexture* const Target = (bIsLast && !bHalfRes)
            ? Context.RayTracingOutput
            : ((Source == Context.ReflectionDenoised[0]) ? Context.ReflectionDenoised[1] : Context.ReflectionDenoised[0]);

        GraphBuilder.AddPass(ATROUS_PASS_NAMES[Iteration], ERenderGraphPassFlags::Compute, true,
            [&Context, Source, Target](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(Source, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.WriteTexture(Target, ERHIResourceState::UnorderedAccess);
            },
            [this, Context, Source, Target, Iteration](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
            {
                FPassResources Resolved = Context.CreatePassResources(PassResources);
                PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
                RecordAtrous(PassCommandList, *Context.FrameResources, Source->GetRHITexture(), Target->GetRHITexture(), Iteration);
            });

        Source = Target;
    }

    if (!bHalfRes)
    {
        return;
    }

    FRenderGraphTexture* const ChainResult = Source;

    GraphBuilder.AddPass(TEXT("ReflectionUpsample"), ERenderGraphPassFlags::Compute, true,
        [&Context, ChainResult](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(ChainResult, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.RayTracingOutput, ERHIResourceState::UnorderedAccess);
        },
        [this, Context, ChainResult](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordUpsample(PassCommandList, *Context.FrameResources, ChainResult->GetRHITexture());
        });
}
