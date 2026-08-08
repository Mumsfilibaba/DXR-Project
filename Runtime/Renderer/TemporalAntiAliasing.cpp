#include "RHI/RHI.h"
#include "Core/Misc/FrameProfiler.h"
#include "Renderer/TemporalAntiAliasing.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/CommonShaders.h"

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

void FTemporalAntiAliasing::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    FRHITextureRef CurrentBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    if (CurrentBuffer->GetDesc().Extent.X == 0 || CurrentBuffer->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "TAA");

    TRACE_SCOPE("TAA");

    GPU_TRACE_SCOPE(CommandList, "TAA");

    if (!bHistoryValid)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.SceneTarget.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));

        for (FRHITextureRef& HistoryBuffer : TAAHistoryBuffers)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(HistoryBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::CopyDest));
            CommandList.CopyTexture(HistoryBuffer.Get(), FrameResources.SceneTarget.Get());
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(HistoryBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::NonPixelShaderResource));
        }

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(FrameResources.SceneTarget.Get(), ERHIResourceState::CopySource, ERHIResourceState::UnorderedAccess));

        CurrentBufferIndex = 0;
        bHistoryValid = true;

        return;
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CurrentBuffer.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));

    CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;

    CommandList.SetComputePipelineState(TemporalAntiAliasingPSO.Get());

    CommandList.SetConstantBuffer(TemporalAntiAliasingShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetUnorderedAccessView(TemporalAntiAliasingShader.Get(), FrameResources.SceneTarget->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(TemporalAntiAliasingShader.Get(), CurrentBuffer->GetUnorderedAccessView(), 1);

    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), FrameResources.GBuffer[EGBufferIndex::Velocity]->GetShaderResourceView(), 1);

    FRHITextureRef CurrentReadBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.SetShaderResourceView(TemporalAntiAliasingShader.Get(), CurrentReadBuffer->GetShaderResourceView(), 2);

    CommandList.SetSamplerState(TemporalAntiAliasingShader.Get(), LinearSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = Math::DivideByMultiple(CurrentBuffer->GetDesc().Extent.X, NumThreads);
    const uint32 ThreadsY = Math::DivideByMultiple(CurrentBuffer->GetDesc().Extent.Y, NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CurrentBuffer.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
}

bool FTemporalAntiAliasing::CreateResources(FFrameResources& /* FrameResources */, uint32 Width, uint32 Height)
{
    // TAA History-Buffer
    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopyDest;
    FRHITextureDesc TAABufferDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::SceneTargetFormat, Width, Height, 1, 1, UsageFlags);

    uint32 Index = 0;
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer = RHI::CreateTexture(TAABufferDesc, ERHIResourceState::NonPixelShaderResource);
        if (TAABuffer)
        {
            TAABuffer->SetDebugName(String::CreateFormatted("TAA History-Buffer[%u]", Index++));
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
