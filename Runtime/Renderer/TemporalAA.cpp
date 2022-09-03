#include "TemporalAA.h"
#include "Renderer.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTemporalAA

bool FTemporalAA::Init(FFrameResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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
            TemporalAAPSO->SetName("CascadeGen PSO");
        }
    }

    FRHISamplerStateInitializer SamplerInitializer(ESamplerMode::Clamp, ESamplerFilter::MinMagMipLinear);
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
    for (FRHITexture2DRef& TAABuffer : TAAHistoryBuffers)
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

    FRHITexture2DRef CurrentBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.TransitionTexture(
        CurrentBuffer.Get(),
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::UnorderedAccess);

    CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;

    CommandList.SetComputePipelineState(TemporalAAPSO.Get());

    CommandList.SetConstantBuffer(TemporalAAShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), FrameResources.FinalTarget->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), CurrentBuffer->GetUnorderedAccessView(), 1);
    
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[GBufferIndex_Velocity]->GetShaderResourceView(), 1);
    
    FRHITexture2DRef CurrentReadBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), CurrentReadBuffer->GetShaderResourceView(), 2);

    CommandList.SetSamplerState(TemporalAAShader.Get(), LinearSampler.Get(), 0);

    const FIntVector3 ThreadGroupXYZ = TemporalAAShader->GetThreadGroupXYZ();
    const uint32 ThreadsX = NMath::DivideByMultiple(CurrentBuffer->GetWidth(), ThreadGroupXYZ.x);
    const uint32 ThreadsY = NMath::DivideByMultiple(CurrentBuffer->GetHeight(), ThreadGroupXYZ.y);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(
        CurrentBuffer.Get(),
        EResourceAccess::UnorderedAccess,
        EResourceAccess::NonPixelShaderResource);

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
    const uint32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    // TAA History-Buffer
    FRHITexture2DInitializer TextureInitializer(
        FrameResources.FinalTargetFormat, 
        Width,
        Height, 
        1,
        1, 
        ETextureUsageFlags::AllowSRV | ETextureUsageFlags::AllowUAV,
        EResourceAccess::NonPixelShaderResource, 
        nullptr);

    uint32 Index = 0;
    for (FRHITexture2DRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer = RHICreateTexture2D(TextureInitializer);
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
