#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Renderer/TemporalAA.h"
#include "Renderer/SceneRenderer.h"

FTemporalAA::FTemporalAA(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CurrentBufferIndex(0)
    , bHistoryValid(false)
{
}

FTemporalAA::~FTemporalAA()
{
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer.Reset();
    }

    LinearSampler.Reset();
    TemporalAAPSO.Reset();
    TemporalAAShader.Reset();
}

bool FTemporalAA::Initialize(FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.CurrentRenderWidth;
    const uint32 Height = FrameResources.CurrentRenderHeight;

    if (!CreateResources(FrameResources, Width, Height))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/TemporalAA.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        TemporalAAShader = RHI::CreateComputeShader(ShaderCode);
        if (!TemporalAAShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateDesc TemporalAA_PSODesc;
        TemporalAA_PSODesc.Shader = TemporalAAShader.Get();
        
        TemporalAAPSO = RHI::CreateComputePipelineState(TemporalAA_PSODesc);
        if (!TemporalAAPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            TemporalAAPSO->SetDebugName("TemporalAA PSO");
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

void FTemporalAA::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    FRHITextureRef CurrentBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    if (CurrentBuffer->GetDesc().Extent.X == 0 || CurrentBuffer->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "TemporalAA");

    TRACE_SCOPE("TemporalAA");

    GPU_TRACE_SCOPE(CommandList, "TemporalAA");

    if (!bHistoryValid)
    {
        // After resize or first frame: seed both history buffers with the current
        // SceneTarget so subsequent frames have valid history to blend with.
        CommandList.TransitionTextureState(FrameResources.SceneTarget.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));

        for (FRHITextureRef& HistoryBuffer : TAAHistoryBuffers)
        {
            CommandList.TransitionTextureState(HistoryBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::CopyDest));
            CommandList.CopyTexture(HistoryBuffer.Get(), FrameResources.SceneTarget.Get());
            CommandList.TransitionTextureState(HistoryBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::NonPixelShaderResource));
        }

        CommandList.TransitionTextureState(FrameResources.SceneTarget.Get(), FRHITextureTransition::Make(EResourceAccess::CopySource, EResourceAccess::UnorderedAccess));

        CurrentBufferIndex = 0;
        bHistoryValid = true;

        return;
    }

    CommandList.TransitionTextureState(CurrentBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

    CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;

    CommandList.SetComputePipelineState(TemporalAAPSO.Get());

    CommandList.SetConstantBuffer(TemporalAAShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), FrameResources.SceneTarget->GetUnorderedAccessView(), 0);
    CommandList.SetUnorderedAccessView(TemporalAAShader.Get(), CurrentBuffer->GetUnorderedAccessView(), 1);
    
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), FrameResources.GBuffer[EGBufferIndex::Velocity]->GetShaderResourceView(), 1);
    
    FRHITextureRef CurrentReadBuffer = TAAHistoryBuffers[CurrentBufferIndex];
    CommandList.SetShaderResourceView(TemporalAAShader.Get(), CurrentReadBuffer->GetShaderResourceView(), 2);

    CommandList.SetSamplerState(TemporalAAShader.Get(), LinearSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = Math::DivideByMultiple(CurrentBuffer->GetDesc().Extent.X, NumThreads);
    const uint32 ThreadsY = Math::DivideByMultiple(CurrentBuffer->GetDesc().Extent.Y, NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTextureState(CurrentBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
}

bool FTemporalAA::CreateResources(FFrameResources& /* FrameResources */, uint32 Width, uint32 Height)
{
    // TAA History-Buffer
    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::UnorderedAccessTexture;
    FRHITextureDesc TAABufferDesc = FRHITextureDesc::CreateTexture2D(RendererTextureFormats::SceneTargetFormat, Width, Height, 1, 1, UsageFlags);

    uint32 Index = 0;
    for (FRHITextureRef& TAABuffer : TAAHistoryBuffers)
    {
        TAABuffer = RHI::CreateTexture(TAABufferDesc, EResourceAccess::NonPixelShaderResource);
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
