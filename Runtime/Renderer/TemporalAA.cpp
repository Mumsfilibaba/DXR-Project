#include "TemporalAA.h"
#include "Renderer.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"

bool FTemporalAA::Initialize(FFrameResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/TemporalAA.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        TemporalAAShader = RHICreateComputeShader(ShaderCode);
        if (!TemporalAAShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer TemporalAAInitializer(TemporalAAShader.Get());
        TemporalAAPSO = RHICreateComputePipelineState(TemporalAAInitializer);
        if (!TemporalAAPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            TemporalAAPSO->SetName("TemporalAA PSO");
        }
    }

    FRHISamplerStateDesc SamplerInitializer(ESamplerMode::Clamp, ESamplerFilter::MinMagMipLinear);
    LinearSampler = RHICreateSamplerState(SamplerInitializer);
    if (!LinearSampler)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FTemporalAA::Release()
{
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer.Reset();
    }

    LinearSampler.Reset();
    TemporalAAPSO.Reset();
    TemporalAAShader.Reset();
}

void FTemporalAA::Render(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin TemporalAA");

    TRACE_SCOPE("TemporalAA");

    GPU_TRACE_SCOPE(CommandList, "TemporalAA");

    FRHITextureRef CurrentBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.TransitionTexture(CurrentBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;

    CommandList.SetComputePipelineState(TemporalAAPSO.Get());

    CommandList.SetConstantBuffer(TemporalAAShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), FrameResources.FinalTarget->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), CurrentBuffer->GetUnorderedAccessView(), 1);
    
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[GBufferIndex_Velocity]->GetShaderResourceView(), 1);
    
    FRHITextureRef CurrentReadBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), CurrentReadBuffer->GetShaderResourceView(), 2);

    CommandList.SetSamplerState(TemporalAAShader.Get(), LinearSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = FMath::DivideByMultiple(CurrentBuffer->GetWidth(), NumThreads);
    const uint32 ThreadsY = FMath::DivideByMultiple(CurrentBuffer->GetHeight(), NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(CurrentBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(TAAHistoryBuffers[0]->GetShaderResourceView()),
        TAAHistoryBuffers[0],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(TAAHistoryBuffers[1]->GetShaderResourceView()),
        TAAHistoryBuffers[1],
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End TemporalAA");
}

bool FTemporalAA::ResizeResources(FFrameResources& FrameResources)
{
    return CreateRenderTarget(FrameResources);
}

bool FTemporalAA::CreateRenderTarget(FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.MainViewport->GetWidth();
    const uint32 Height = FrameResources.MainViewport->GetHeight();

    // TAA History-Buffer
    FRHITextureDesc TAABufferDesc = FRHITextureDesc::CreateTexture2D(FrameResources.FinalTargetFormat, Width, Height, 1, 1, ETextureUsageFlags::ShaderResource | ETextureUsageFlags::UnorderedAccess);

    uint32 Index = 0;
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer = RHICreateTexture(TAABufferDesc, EResourceAccess::NonPixelShaderResource);
        if (TAABuffer)
        {
            TAABuffer->SetName(FString::CreateFormatted("TAA History-Buffer[%u]", Index++));
        }
        else
        {
            return false;
        }
    }

    return true;
}
