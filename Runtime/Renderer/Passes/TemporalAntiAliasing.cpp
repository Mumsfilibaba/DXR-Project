#include "RHI/RHI.h"
#include "Core/Misc/FrameProfiler.h"
#include "Renderer/Passes/TemporalAntiAliasing.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

class FTemporalAntiAliasingCS
{
    DECLARE_SHADER_TYPE(FTemporalAntiAliasingCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FTemporalAntiAliasingCS, "Shaders/TemporalAntiAliasing.hlsl", "Main", EShaderModel::SM_6_2);

FTemporalAntiAliasing::FTemporalAntiAliasing(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CurrentBufferIndex(0)
    , bHistoryValid(false)
{
}

FTemporalAntiAliasing::~FTemporalAntiAliasing()
{
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer.Reset();
    }

    LinearSampler.Reset();
    TemporalAntiAliasingPSO.Reset();
    TemporalAntiAliasingShader.Reset();
}

bool FTemporalAntiAliasing::Initialize(FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.CurrentRenderWidth;
    const uint32 Height = FrameResources.CurrentRenderHeight;

    if (!CreateResources(FrameResources, Width, Height))
    {
        return false;
    }

    {
        TemporalAntiAliasingShader = FShaderCache::Get().GetShader<FTemporalAntiAliasingCS>();
        if (!TemporalAntiAliasingShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateDesc TemporalAntiAliasingPSODesc;
        TemporalAntiAliasingPSODesc.Shader = TemporalAntiAliasingShader.Get();

        TemporalAntiAliasingPSO = RHI::CreateComputePipelineState(TemporalAntiAliasingPSODesc);
        if (!TemporalAntiAliasingPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            TemporalAntiAliasingPSO->SetDebugName("TAA PSO");
        }
    }

    FRHISamplerStateDesc SamplerDesc = FRHISamplerStateDesc::Create(ESamplerMode::Clamp, ESamplerFilter::MinMagMipLinear);
    LinearSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!LinearSampler)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FTemporalAntiAliasing::Record(FRHICommandList& CommandList, FFrameResources& FrameResources, uint32 ReadIndex, uint32 WriteIndex)
{
    FRHITextureRef WriteBuffer = TAAHistoryBuffers[WriteIndex];
    if (WriteBuffer->GetDesc().Extent.X == 0 || WriteBuffer->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "TAA");

    TRACE_SCOPE("TAA");

    CommandList.SetComputePipelineState(TemporalAntiAliasingPSO.Get());

    CommandList.SetConstantBuffer(TemporalAntiAliasingShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetUnorderedAccessView(TemporalAntiAliasingShader.Get(), FrameResources.SceneTarget->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(TemporalAntiAliasingShader.Get(), WriteBuffer->GetUnorderedAccessView(), 1);

    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), FrameResources.GBuffer[EGBufferIndex::Velocity]->GetShaderResourceView(), 1);

    FRHITextureRef ReadBuffer = TAAHistoryBuffers[ReadIndex];
    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), ReadBuffer->GetShaderResourceView(), 2);

    CommandList.SetSamplerState(TemporalAntiAliasingShader.Get(), LinearSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = Math::DivideByMultiple(WriteBuffer->GetDesc().Extent.X, NumThreads);
    const uint32 ThreadsY = Math::DivideByMultiple(WriteBuffer->GetDesc().Extent.Y, NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CurrentBufferIndex = ReadIndex;
}

void FTemporalAntiAliasing::RecordHistorySeed(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    if (TAAHistoryBuffers[0]->GetDesc().Extent.X == 0 || TAAHistoryBuffers[0]->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "TAAHistorySeed");

    TRACE_SCOPE("TAAHistorySeed");

    for (FRHITextureRef& HistoryBuffer : TAAHistoryBuffers)
    {
        CommandList.CopyTexture(HistoryBuffer.Get(), FrameResources.SceneTarget.Get());
    }

    CurrentBufferIndex = 0;
    bHistoryValid = true;
}

bool FTemporalAntiAliasing::CreateResources(FFrameResources& /* FrameResources */, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags UsageFlags =
        ETextureUsageFlags::ShaderResourceTexture |
        ETextureUsageFlags::UnorderedAccessTexture |
        ETextureUsageFlags::CopyDest;

    FRHITextureDesc TAABufferDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::SceneTargetFormat, Width, Height, 1, 1, UsageFlags);

    uint32 Index = 0;
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer = RHI::CreateTexture(TAABufferDesc, ERHIResourceState::NonPixelShaderResource);
        if (TAABuffer)
        {
            TAABuffer->SetDebugName(String::Printf("TAA History-Buffer[%u]", Index++));
        }
        else
        {
            return false;
        }
    }

    bHistoryValid = false;
    CurrentBufferIndex = 0;

    return true;
}

void FTemporalAntiAliasing::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    if (!bHistoryValid)
    {
        GraphBuilder.AddPass("TAAHistorySeed", ERenderGraphPassFlags::Copy, GEnableTemporalAntiAliasing,
            [&Context](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(Context.SceneTarget, ERHIResourceState::CopySource);
                PassBuilder.WriteTexture(Context.TAAHistory[0], ERHIResourceState::CopyDest);
                PassBuilder.WriteTexture(Context.TAAHistory[1], ERHIResourceState::CopyDest);
            },
            [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
            {
                FPassResources Resolved = Context.CreatePassResources(PassResources);
                PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
                RecordHistorySeed(PassCommandList, *Context.FrameResources);
            });

        return;
    }

    const uint32 WriteIndex = CurrentBufferIndex;
    const uint32 ReadIndex  = (CurrentBufferIndex + 1) % 2;

    GraphBuilder.AddPass("TAA", ERenderGraphPassFlags::Compute, GEnableTemporalAntiAliasing,
        [&Context, ReadIndex, WriteIndex](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferVelocity, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.SceneTarget, ERHIResourceState::UnorderedAccess);
            PassBuilder.WriteTexture(Context.SceneTarget, ERHIResourceState::UnorderedAccess);
            PassBuilder.ReadTexture(Context.TAAHistory[ReadIndex], ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.TAAHistory[WriteIndex], ERHIResourceState::UnorderedAccess);
        },
        [this, Context, ReadIndex, WriteIndex](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, ReadIndex, WriteIndex);
        });
}
