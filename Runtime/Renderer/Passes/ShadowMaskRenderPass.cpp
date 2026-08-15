#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Renderer/Passes/ShadowMaskRenderPass.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Settings/ShadowSettings.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/SceneRenderer.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

bool GCSMDebugCascades = false;
static FAutoConsoleVariableRef CVarCSMDebugCascades(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    GCSMDebugCascades,
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
    UNREFERENCED_VARIABLE(Resources);

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

    const FRHIBufferDesc SettingsBufferDesc = FRHIBufferDesc::CreateConstantBuffer(sizeof(FDirectionalShadowSettingsHLSL));
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

FDirectionalShadowSettingsHLSL FShadowMaskRenderPass::CreateShadowSettings(const FFrameResources& Resources, uint32 FrameIndex)
{
    FDirectionalShadowSettingsHLSL ShadowSettings;
    Memory::Memzero(&ShadowSettings);

    ShadowSettings.FilterSize    = Math::Max<float>(static_cast<float>(CVarCSMFilterSize.GetValue()), 1.0f);
    ShadowSettings.MaxFilterSize = Math::Max<float>(static_cast<float>(CVarCSMMaxFilterSize.GetValue()), 1.0f);
    ShadowSettings.ShadowMapSize = Resources.ShadowCascades ? Resources.ShadowCascades->GetDesc().Extent.X : 0;
    ShadowSettings.FrameIndex    = FrameIndex;
    ShadowSettings.NumSamples    = Math::Clamp<uint32>(CVarCSMNumPoissonDiscSamples.GetValue(), 4, 128);

    return ShadowSettings;
}

void FShadowMaskRenderPass::Record(FRHICommandList& CommandList, const FFrameResources& Resources, bool bForceDebugMode)
{
    if (Resources.DirectionalShadowMask->GetDesc().Extent.X == 0 || Resources.DirectionalShadowMask->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Render ShadowMasks");

    TRACE_SCOPE("Render ShadowMasks");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

    const FDirectionalShadowSettingsHLSL ShadowSettings = CreateShadowSettings(Resources, GetRenderer()->GetFrameCounter().GetFrameIndex());

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(ShadowSettingsBuffer.Get(), ERHIResourceState::ConstantBuffer, ERHIResourceState::CopyDest));
    CommandList.UpdateBuffer(ShadowSettingsBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalShadowSettingsHLSL)), &ShadowSettings);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(ShadowSettingsBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ConstantBuffer));

    FShadowMaskCS::FPermutation Permutation = CreateCurrentPermutation();

    const bool bDebugMode = Permutation.Get<FCSMDebug>() || bForceDebugMode;
    Permutation.Set<FCSMDebug>(bDebugMode);

    FComputePipelineStateInstance PipelineStateInstance;
    if (!RetrievePipelineState(Permutation, PipelineStateInstance))
    {
        DEBUG_BREAK();
        return;
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
        PipelineStateInstance.PipelineState->SetDebugName(String::Printf("ShadowMask PSO (Permutation %d)", PermutationID));
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

void FShadowMaskRenderPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    FSceneDirectionalLight* DirectionalLight = Context.Scene ? Context.Scene->GetDirectionalLight() : nullptr;

    const bool bEnableSunShadows  = GSunShadowsEnabled && (!DirectionalLight || DirectionalLight->bCastShadows);
    const bool bNeedsCascadeDebug = Context.View &&
        (Context.View->DebugView == FSceneRenderView::EDebugView::ShadowCascadeIndex ||
            Context.View->DebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay ||
            Context.View->SecondaryDebugView == FSceneRenderView::EDebugView::ShadowCascadeIndex ||
            Context.View->SecondaryDebugView == FSceneRenderView::EDebugView::ShadowCascadeOverlay);

    const bool bEnablePass =
        GShadowsEnabled &&
        GShadowMaskEnabled &&
        bEnableSunShadows;

    GraphBuilder.AddPass("ShadowMask", ERenderGraphPassFlags::Compute, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.ShadowCascades, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.DirectionalShadowMask, ERHIResourceState::UnorderedAccess);
            PassBuilder.WriteTexture(Context.CascadeIndexBuffer, ERHIResourceState::UnorderedAccess);

            // The mask shader samples both cascade buffers, so it has to declare them or they resolve to nothing
            if (Context.CascadeMatrixBufferSRV)
            {
                PassBuilder.Read(Context.CascadeMatrixBufferSRV);
            }

            if (Context.CascadeSplitsBufferSRV)
            {
                PassBuilder.Read(Context.CascadeSplitsBufferSRV);
            }

            if (Context.ShadowSettingsBuffer)
            {
                PassBuilder.ReadBuffer(Context.ShadowSettingsBuffer, ERHIResourceState::ConstantBuffer);
            }
        },
        [this, Context, bNeedsCascadeDebug](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, bNeedsCascadeDebug);
        });

    GraphBuilder.AddPass("ShadowMaskClear", ERenderGraphPassFlags::Compute, !bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.WriteTexture(Context.DirectionalShadowMask, ERHIResourceState::UnorderedAccess);
            PassBuilder.WriteTexture(Context.CascadeIndexBuffer, ERHIResourceState::UnorderedAccess);
        },
        [Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            const Vector4 MaskClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            PassCommandList.ClearUnorderedAccessViewFloat(PassResources.Get(Context.DirectionalShadowMask)->GetUnorderedAccessView(), MaskClearColor);

            const Vector4 DebugClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            PassCommandList.ClearUnorderedAccessViewFloat(PassResources.Get(Context.CascadeIndexBuffer)->GetUnorderedAccessView(), DebugClearColor);
        });
}
