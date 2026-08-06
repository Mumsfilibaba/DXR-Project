#include "RHI/ValidationLayer/RHIValidationCommandContext.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"
#include "RHI/ValidationLayer/RHIValidationShaderBindingTable.h"

using namespace RHIValidationInternal;

FRHIValidationCommandContext::FRHIValidationCommandContext(IRHICommandContext* InRealContext)
    : IRHICommandContext()
    , CommandContext(InRealContext)
    , ContextPhase(ECommandContextPhase::Finished)
    , GraphicsPipelineState(nullptr)
    , ComputePipelineState(nullptr)
    , MeshletPipelineState(nullptr)
    , RayTracingPipelineState(nullptr)
    , ActiveQueries()
{
}

FRHIValidationCommandContext::~FRHIValidationCommandContext()
{
}

bool FRHIValidationCommandContext::ValidateRecordingPhase(const CHAR* Caller) const
{
    if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("%s requires an active recording context. Call StartContext first.", Caller);
        return false;
    }

    return true;
}

void FRHIValidationCommandContext::BeginFrame()
{
    CommandContext->BeginFrame();
}

void FRHIValidationCommandContext::EndFrame()
{
    CommandContext->EndFrame();
}

void FRHIValidationCommandContext::StartContext()
{
    if (ContextPhase >= ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Invalid to call StartContext when FinishContext has not been called in-between");
        return;
    }

    CommandContext->StartContext();

    ContextPhase            = ECommandContextPhase::Recording;
    GraphicsPipelineState   = nullptr;
    ComputePipelineState    = nullptr;
    MeshletPipelineState    = nullptr;
    RayTracingPipelineState = nullptr;

    ActiveQueries.Clear();
    OpenSplits.Clear();
}

void FRHIValidationCommandContext::FinishContext()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext when inside a renderpass");
        return;
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call FinishContext before a call to StartContext");
        return;
    }

    if (!ActiveQueries.IsEmpty())
    {
        RHI_VALIDATION_ERROR("FinishContext cannot be called while queries are active.");
        return;
    }

    if (!OpenSplits.IsEmpty())
    {
        RHI_VALIDATION_ERROR("FinishContext cannot be called while %d split barrier(s) are still in flight. "
            "Every BeginOnly barrier needs a matching EndOnly on the same context", OpenSplits.Size());
        return;
    }

    CommandContext->FinishContext();
    ContextPhase = ECommandContextPhase::Finished;
}

void FRHIValidationCommandContext::BeginQuery(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("BeginQuery"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("RHIBeginQuery does not support a Query of type Timestamp");
        return;
    }

    if (ActiveQueries.Contains(Query))
    {
        RHI_VALIDATION_ERROR("BeginQuery cannot begin a query that is already active.");
        return;
    }

    CommandContext->BeginQuery(Query);
    ActiveQueries.Add(Query);
}

void FRHIValidationCommandContext::EndQuery(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("EndQuery"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndQuery when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType == EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("EndQuery does not support a Query of type Timestamp");
        return;
    }

    if (!ActiveQueries.Contains(Query))
    {
        RHI_VALIDATION_ERROR("EndQuery requires a matching BeginQuery.");
        return;
    }

    CommandContext->EndQuery(Query);
    ActiveQueries.Remove(Query);
}

void FRHIValidationCommandContext::QueryTimestamp(FRHIQuery* Query)
{
    if (!ValidateRecordingPhase("QueryTimestamp"))
    {
        return;
    }

    if (!Query)
    {
        RHI_VALIDATION_ERROR("Invalid to call QueryTimestamp when Query is nullptr");
        return;
    }

    const EQueryType QueryType = Query->GetType();
    if (QueryType != EQueryType::Timestamp)
    {
        RHI_VALIDATION_ERROR("QueryTimestamp only support a Query of type Timestamp");
        return;
    }

    CommandContext->QueryTimestamp(Query);
}

void FRHIValidationCommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    if (!RenderTargetView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearRenderTargetView when RenderTargetView is nullptr");
        return;
    }

    CommandContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void FRHIValidationCommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, const uint8 Stencil)
{
    if (!DepthStencilView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearDepthStencilView when DepthStencilView is nullptr");
        return;
    }

    CommandContext->ClearDepthStencilView(DepthStencilView, Depth, Stencil);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewFloat when UnorderedAccessView is nullptr");
        return;
    }

    CommandContext->ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
}

void FRHIValidationCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    if (!UnorderedAccessView)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearUnorderedAccessViewUint when UnorderedAccessView is nullptr");
        return;
    }

    CommandContext->ClearUnorderedAccessViewUint(UnorderedAccessView, Values);
}

void FRHIValidationCommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling EndRenderPass");
        return;
    }
    else if (ContextPhase == ECommandContextPhase::Finished)
    {
        RHI_VALIDATION_ERROR("Invalid to call RHIBeginRenderPass before calling StartContext");
        return;
    }

    if (BeginRenderPassDesc.NumRenderTargets > RHI_MAX_RENDER_TARGETS)
    {
        RHI_VALIDATION_ERROR("Trying to bind to many render-targets in a render-pass. Max is '%u' but this call is trying to bind '%u'",
            RHI_MAX_RENDER_TARGETS, BeginRenderPassDesc.NumRenderTargets);
        return;
    }

    for (uint32 Index = 0; Index < BeginRenderPassDesc.NumRenderTargets; ++Index)
    {
        if (!BeginRenderPassDesc.RenderTargets[Index].View)
        {
            RHI_VALIDATION_ERROR("BeginRenderPass: render-target attachment %u is nullptr.", Index);
            return;
        }
    }

    if (BeginRenderPassDesc.ShadingRateTexture && !BeginRenderPassDesc.ShadingRateTexture->GetDesc().IsShadingRateTexture())
    {
        RHI_VALIDATION_ERROR("BeginRenderPass: shading-rate texture lacks ShadingRateTexture usage.");
        return;
    }

    CommandContext->BeginRenderPass(BeginRenderPassDesc);
    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FRHIValidationCommandContext::EndRenderPass()
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call EndRenderPass before calling RHIBeginRenderPass");
        return;
    }

    CommandContext->EndRenderPass();
    ContextPhase = ECommandContextPhase::Recording;
}

void FRHIValidationCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    CommandContext->SetViewport(ViewportRegion);
}

void FRHIValidationCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    CommandContext->SetScissorRect(ScissorRegion);
}

void FRHIValidationCommandContext::SetBlendFactor(const Vector4& Color)
{
    CommandContext->SetBlendFactor(Color);
}

void FRHIValidationCommandContext::SetStencilRef(uint32 StencilRef)
{
    CommandContext->SetStencilRef(StencilRef);
}

void FRHIValidationCommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
    if (!RHI::bSupportsDynamicDepthBias)
    {
        RHI_VALIDATION_ERROR("SetDepthBias called but dynamic depth bias is not supported on this device");
        return;
    }

    CommandContext->SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FRHIValidationCommandContext::SetDepthBounds(float MinDepth, float MaxDepth)
{
    if (!RHI::bSupportsDepthBoundsTest)
    {
        RHI_VALIDATION_ERROR("SetDepthBounds called but the depth bounds test is not supported on this device");
        return;
    }

    if (MinDepth > MaxDepth || MinDepth < 0.0f || MaxDepth > 1.0f)
    {
        RHI_VALIDATION_ERROR("SetDepthBounds requires 0.0 <= MinDepth <= MaxDepth <= 1.0 (got %f, %f)", MinDepth, MaxDepth);
        return;
    }

    CommandContext->SetDepthBounds(MinDepth, MaxDepth);
}

void FRHIValidationCommandContext::SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc)
{
    if (!RHI::bSupportsProgrammableSamplePositions)
    {
        RHI_VALIDATION_ERROR("SetSamplePositions called but programmable sample positions are not supported on this device");
        return;
    }

    if (SamplePositionsDesc.NumSamplesPerPixel > 0)
    {
        if (SamplePositionsDesc.GridWidth  > RHI::MaxSamplePositionGridWidth ||
            SamplePositionsDesc.GridHeight > RHI::MaxSamplePositionGridHeight)
        {
            RHI_VALIDATION_ERROR("SetSamplePositions grid %ux%u exceeds the device maximum of %ux%u",
                SamplePositionsDesc.GridWidth, SamplePositionsDesc.GridHeight, RHI::MaxSamplePositionGridWidth, RHI::MaxSamplePositionGridHeight);
            return;
        }

        if ((RHI::SupportedSamplePositionSampleCounts & SamplePositionsDesc.NumSamplesPerPixel) == 0)
        {
            RHI_VALIDATION_ERROR("SetSamplePositions with %u samples per pixel is not supported on this device",
                SamplePositionsDesc.NumSamplesPerPixel);
            return;
        }

        const uint32 NumPositions = uint32(SamplePositionsDesc.NumSamplesPerPixel) * uint32(SamplePositionsDesc.GridWidth) * uint32(SamplePositionsDesc.GridHeight);
        if (NumPositions > RHI_MAX_SAMPLE_POSITIONS)
        {
            RHI_VALIDATION_ERROR("SetSamplePositions requires %u positions, exceeding RHI_MAX_SAMPLE_POSITIONS", NumPositions);
            return;
        }

        for (uint32 Index = 0; Index < NumPositions; ++Index)
        {
            const FRHISamplePosition& Position = SamplePositionsDesc.Positions[Index];
            if (Position.X < -0.5f || Position.X >= 0.5f || Position.Y < -0.5f || Position.Y >= 0.5f)
            {
                RHI_VALIDATION_ERROR("SetSamplePositions position %u (%f, %f) is outside the valid range [-0.5, 0.5)", Index, Position.X, Position.Y);
                return;
            }
        }
    }

    CommandContext->SetSamplePositions(SamplePositionsDesc);
}

void FRHIValidationCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    if (!ValidateRecordingPhase("SetVertexBuffers"))
    {
        return;
    }

    const uint32 NumVertexBuffers = uint32(InVertexBuffers.Size());
    if (BufferSlot > RHI_MAX_VERTEX_BUFFERS || NumVertexBuffers > RHI_MAX_VERTEX_BUFFERS - BufferSlot)
    {
        RHI_VALIDATION_ERROR("SetVertexBuffers exceeds RHI_MAX_VERTEX_BUFFERS.");
        return;
    }

    for (FRHIBuffer* Buffer : InVertexBuffers)
    {
        if (!Buffer || !Buffer->GetDesc().IsVertexBuffer())
        {
            RHI_VALIDATION_ERROR("SetVertexBuffers requires non-null buffers with EBufferFlags::VertexBuffer.");
            return;
        }
    }

    CommandContext->SetVertexBuffers(InVertexBuffers, BufferSlot);
}

void FRHIValidationCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    if (!ValidateRecordingPhase("SetIndexBuffer"))
    {
        return;
    }

    if (!IndexBuffer || !IndexBuffer->GetDesc().IsIndexBuffer() || IndexFormat == EIndexFormat::Unknown)
    {
        RHI_VALIDATION_ERROR("SetIndexBuffer requires an index buffer and a valid index format.");
        return;
    }

    CommandContext->SetIndexBuffer(IndexBuffer, IndexFormat);
}

void FRHIValidationCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    if (!ValidateRecordingPhase("SetStreamOutputTargets"))
    {
        return;
    }

    if (!Buffers.IsEmpty() && !Offsets)
    {
        RHI_VALIDATION_ERROR("SetStreamOutputTargets requires offsets for non-empty buffer bindings.");
        return;
    }

    for (FRHIBuffer* const Buffer : Buffers)
    {
        if (Buffer && !Buffer->GetDesc().IsStreamOutputBuffer())
        {
            String DebugName;
            Buffer->GetDebugName(DebugName);
            RHI_VALIDATION_ERROR("SetStreamOutputTargets: Buffer '%s' does not have StreamOutputBuffer flag", *DebugName);
            return;
        }
    }

    CommandContext->SetStreamOutputTargets(Buffers, Offsets);
}

void FRHIValidationCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetGraphicsPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetGraphicsPipelineState requires a non-null pipeline.");
        return;
    }

    GraphicsPipelineState = PipelineState;
    CommandContext->SetGraphicsPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetComputePipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetComputePipelineState requires a non-null pipeline.");
        return;
    }

    ComputePipelineState = PipelineState;
    CommandContext->SetComputePipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetRayTracingPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetRayTracingPipelineState: PipelineState cannot be nullptr.");
        return;
    }

    RayTracingPipelineState = PipelineState;
    CommandContext->SetRayTracingPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetMeshletPipelineState(FRHIMeshletPipelineState* PipelineState)
{
    if (!ValidateRecordingPhase("SetMeshletPipelineState"))
    {
        return;
    }

    if (!PipelineState)
    {
        RHI_VALIDATION_ERROR("SetMeshletPipelineState requires a non-null pipeline.");
        return;
    }

    MeshletPipelineState = PipelineState;
    CommandContext->SetMeshletPipelineState(PipelineState);
}

void FRHIValidationCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderConstants when Shader is nullptr");
        return;
    }

    if (!ShaderConstants || NumShaderConstants == 0 || NumShaderConstants > RHI_MAX_SHADER_CONSTANTS)
    {
        RHI_VALIDATION_ERROR("SetShaderConstants requires data and 1..RHI_MAX_SHADER_CONSTANTS constants.");
        return;
    }

    CommandContext->SetShaderConstants(Shader, ShaderConstants, NumShaderConstants);
}

void FRHIValidationCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceView when Shader is nullptr");
        return;
    }

    CommandContext->SetShaderResourceView(Shader, ShaderResourceView, RegisterIndex);
}

void FRHIValidationCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetShaderResourceViews when Shader is nullptr");
        return;
    }

    CommandContext->SetShaderResourceViews(Shader, InShaderResourceViews, RegisterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessView when Shader is nullptr");
        return;
    }

    CommandContext->SetUnorderedAccessView(Shader, UnorderedAccessView, RegisterIndex);
}

void FRHIValidationCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetUnorderedAccessViews when Shader is nullptr");
        return;
    }

    CommandContext->SetUnorderedAccessViews(Shader, InUnorderedAccessViews, RegisterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffer when Shader is nullptr");
        return;
    }

    if (ConstantBuffer && !ConstantBuffer->GetDesc().IsConstantBuffer())
    {
        RHI_VALIDATION_ERROR("SetConstantBuffer requires EBufferFlags::ConstantBuffer.");
        return;
    }

    CommandContext->SetConstantBuffer(Shader, ConstantBuffer, RegisterIndex);
}

void FRHIValidationCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetConstantBuffers when Shader is nullptr");
        return;
    }

    for (FRHIBuffer* Buffer : InConstantBuffers)
    {
        if (Buffer && !Buffer->GetDesc().IsConstantBuffer())
        {
            RHI_VALIDATION_ERROR("SetConstantBuffers requires EBufferFlags::ConstantBuffer.");
            return;
        }
    }

    CommandContext->SetConstantBuffers(Shader, InConstantBuffers, RegisterIndex);
}

void FRHIValidationCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerState when Shader is nullptr");
        return;
    }

    CommandContext->SetSamplerState(Shader, SamplerState, RegisterIndex);
}

void FRHIValidationCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    if (!Shader)
    {
        RHI_VALIDATION_ERROR("Invalid to call SetSamplerStates when Shader is nullptr");
        return;
    }

    CommandContext->SetSamplerStates(Shader, InSamplerStates, RegisterIndex);
}

void FRHIValidationCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    if (!ValidateRecordingPhase("UpdateBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateBuffer when SrcData is nullptr");
        return;
    }

    if (Dst->GetDesc().IsDefault() && !Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("UpdateBuffer on Default memory requires EBufferFlags::CopyDest");
        return;
    }

    if (!ValidateBufferRange("UpdateBuffer", Dst->GetDesc(), BufferRegion.Offset, BufferRegion.Size))
    {
        return;
    }

    CommandContext->UpdateBuffer(Dst, BufferRegion, SrcData);
}

void FRHIValidationCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    if (!ValidateRecordingPhase("UpdateTexture2D"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture2D when SrcData is nullptr");
        return;
    }

    if (!ValidateTextureRegion2D("UpdateTexture2D", Dst->GetDesc(), MipLevel, TextureRegion))
    {
        return;
    }

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Dst->GetDesc().Format);
    if (BytesPerPixel != 0 && SrcRowPitch < TextureRegion.Width * BytesPerPixel)
    {
        RHI_VALIDATION_ERROR("UpdateTexture2D: source row pitch %u is smaller than the required %u bytes.", SrcRowPitch, TextureRegion.Width * BytesPerPixel);
        return;
    }

    CommandContext->UpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
}

void FRHIValidationCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    if (!ValidateRecordingPhase("UpdateTexture3D"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture3D when Dst is nullptr");
        return;
    }

    if (!SrcData)
    {
        RHI_VALIDATION_ERROR("Invalid to call UpdateTexture3D when SrcData is nullptr");
        return;
    }

    if (Dst->GetDesc().Dimension != ETextureDimension::Texture3D)
    {
        RHI_VALIDATION_ERROR("UpdateTexture3D requires a Texture3D destination.");
        return;
    }

    if (!ValidateTextureRegion3D("UpdateTexture3D", Dst->GetDesc(), MipLevel, TextureRegion))
    {
        return;
    }

    const uint32 BytesPerPixel = GetByteStrideFromFormat(Dst->GetDesc().Format);
    if (BytesPerPixel != 0)
    {
        const uint64 MinRowPitch   = uint64(TextureRegion.Width) * BytesPerPixel;
        const uint64 MinDepthPitch = uint64(SrcRowPitch) * TextureRegion.Height;

        if (SrcRowPitch < MinRowPitch || SrcDepthPitch < MinDepthPitch)
        {
            RHI_VALIDATION_ERROR("UpdateTexture3D: source pitches are too small (Row=%u, Depth=%u, RequiredRow=%llu, RequiredDepth=%llu).",
                SrcRowPitch, SrcDepthPitch, MinRowPitch, MinDepthPitch);
            return;
        }
    }

    CommandContext->UpdateTexture3D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
}

void FRHIValidationCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!ValidateRecordingPhase("ResolveTexture"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResolveTexture when Src is nullptr");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();
    if (SrcDesc.NumSamples <= 1 || DstDesc.NumSamples != 1 ||
        SrcDesc.Format != DstDesc.Format ||
        SrcDesc.Dimension != DstDesc.Dimension ||
        SrcDesc.Extent != DstDesc.Extent ||
        SrcDesc.NumArraySlices != DstDesc.NumArraySlices)
    {
        RHI_VALIDATION_ERROR("ResolveTexture requires a multisampled source and matching single-sample destination.");
        return;
    }

    CommandContext->ResolveTexture(Dst, Src);
}

void FRHIValidationCommandContext::TranscodeSamplerFeedback(FRHITexture* Dst, uint32 DstSubresource, FRHITexture* Src, uint32 SrcSubresource, ESamplerFeedbackTranscodeMode Mode)
{
    if (!ValidateRecordingPhase("TranscodeSamplerFeedback"))
    {
        return;
    }

    if (!RHI::bSupportsSamplerFeedback)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: sampler feedback is not supported on this backend (see RHI.DumpCaps).");
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call TranscodeSamplerFeedback when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call TranscodeSamplerFeedback when Src is nullptr");
        return;
    }

    const bool bDecode = (Mode == ESamplerFeedbackTranscodeMode::Decode);

    // Decoding reads the opaque map and writes R8_UINT; encoding does the reverse.
    FRHITexture* const OpaqueTexture     = bDecode ? Src : Dst;
    FRHITexture* const ReadableTexture   = bDecode ? Dst : Src;
    const uint32       OpaqueSubresource = bDecode ? SrcSubresource : DstSubresource;

    const FRHITextureDesc& OpaqueDesc   = OpaqueTexture->GetDesc();
    const FRHITextureDesc& ReadableDesc = ReadableTexture->GetDesc();

    if (!OpaqueDesc.IsSamplerFeedbackTexture())
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: the %s must have ETextureUsageFlags::SamplerFeedback when %s.",
            bDecode ? "source" : "destination", bDecode ? "decoding" : "encoding");
        return;
    }

    if (ReadableDesc.IsSamplerFeedbackTexture())
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: exactly one side may be a sampler feedback map, but both are.");
        return;
    }

    if (ReadableDesc.Format != EFormat::R8_Uint)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: the non-opaque side must be R8_Uint. (Got '%s').", ToString(ReadableDesc.Format));
        return;
    }

    if (ReadableDesc.Dimension != ETextureDimension::Texture2D && ReadableDesc.Dimension != ETextureDimension::Texture2DArray)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: the non-opaque side must be Texture2D or Texture2DArray. (Got '%s').", ToString(ReadableDesc.Dimension));
        return;
    }

    const IntVector3 MipRegion = OpaqueDesc.SamplerFeedbackMipRegion;
    if (MipRegion.X <= 0 || MipRegion.Y <= 0)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: the feedback map has an invalid SamplerFeedbackMipRegion (%d,%d).", MipRegion.X, MipRegion.Y);
        return;
    }

    const int32 MinWidth  = Math::DivideByMultiple(OpaqueDesc.Extent.X, static_cast<uint32>(MipRegion.X));
    const int32 MinHeight = Math::DivideByMultiple(OpaqueDesc.Extent.Y, static_cast<uint32>(MipRegion.Y));

    if (ReadableDesc.Extent.X < MinWidth || ReadableDesc.Extent.Y < MinHeight)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: the non-opaque side (%d,%d) is smaller than the required (%d,%d) for a (%d,%d) feedback map with a (%d,%d) mip region.",
            ReadableDesc.Extent.X, ReadableDesc.Extent.Y, MinWidth, MinHeight,
            OpaqueDesc.Extent.X, OpaqueDesc.Extent.Y, MipRegion.X, MipRegion.Y);
        return;
    }

    if (ReadableDesc.NumArraySlices != OpaqueDesc.NumArraySlices)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: array sizes must match. (Feedback=%u, Readable=%u).",
            OpaqueDesc.NumArraySlices, ReadableDesc.NumArraySlices);
        return;
    }

    if (OpaqueDesc.Format == EFormat::SamplerFeedbackMinMipOpaque)
    {
        if (ReadableDesc.NumMipLevels != 1)
        {
            RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: MinMip transcodes require a single-mip non-opaque side. (NumMipLevels=%u).", ReadableDesc.NumMipLevels);
            return;
        }

        if (OpaqueSubresource != RHI_ALL_SUBRESOURCES)
        {
            RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: MinMip transcodes require RHI_ALL_SUBRESOURCES on the opaque side. (Got %u).", OpaqueSubresource);
            return;
        }
    }
    else if (ReadableDesc.NumMipLevels != OpaqueDesc.NumMipLevels)
    {
        RHI_VALIDATION_ERROR("TranscodeSamplerFeedback: MipRegionUsed transcodes require matching mip counts. (Feedback=%u, Readable=%u).",
            OpaqueDesc.NumMipLevels, ReadableDesc.NumMipLevels);
        return;
    }

    CommandContext->TranscodeSamplerFeedback(Dst, DstSubresource, Src, SrcSubresource, Mode);
}

void FRHIValidationCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    if (!ValidateRecordingPhase("CopyBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyBuffer when Src is nullptr");
        return;
    }

    if (!IsBufferValidAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (!IsBufferValidAsCopySource(Src->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyBuffer source requires EBufferFlags::CopySource");
        return;
    }

    if (!ValidateBufferRange("CopyBuffer source", Src->GetDesc(), CopyDesc.SrcOffset, CopyDesc.Size) ||
        !ValidateBufferRange("CopyBuffer destination", Dst->GetDesc(), CopyDesc.DstOffset, CopyDesc.Size))
    {
        return;
    }

    if (Dst == Src)
    {
        if (RHIValidationHelpers::DoRangesOverlap(
            CopyDesc.SrcOffset, CopyDesc.Size, CopyDesc.DstOffset, CopyDesc.Size))
        {
            RHI_VALIDATION_ERROR("CopyBuffer source and destination ranges overlap on the same buffer.");
            return;
        }
    }

    CommandContext->CopyBuffer(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    if (!ValidateRecordingPhase("CopyTexture"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTexture when Src is nullptr");
        return;
    }

    if (!Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("CopyTexture destination must be created with ETextureUsageFlags::CopyDest");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTexture source must be created with ETextureUsageFlags::CopySource");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();
    if (DstDesc.Dimension != SrcDesc.Dimension ||
        DstDesc.Format != SrcDesc.Format ||
        DstDesc.Extent != SrcDesc.Extent ||
        DstDesc.NumArraySlices != SrcDesc.NumArraySlices ||
        DstDesc.NumMipLevels != SrcDesc.NumMipLevels ||
        DstDesc.NumSamples != SrcDesc.NumSamples)
    {
        RHI_VALIDATION_ERROR("CopyTexture requires matching source and destination dimensions, format, extent, slices, mips, and sample count.");
        return;
    }

    CommandContext->CopyTexture(Dst, Src);
}

void FRHIValidationCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    if (!ValidateRecordingPhase("CopyTextureRegion"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegion when Src is nullptr");
        return;
    }

    if (!Dst->GetDesc().IsCopyDest())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion destination must be created with ETextureUsageFlags::CopyDest");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion source must be created with ETextureUsageFlags::CopySource");
        return;
    }

    const FRHITextureDesc& DstDesc = Dst->GetDesc();
    const FRHITextureDesc& SrcDesc = Src->GetDesc();

    if (DstDesc.Dimension != SrcDesc.Dimension)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion requires matching source and destination dimensions.");
        return;
    }

    if (CopyDesc.NumArraySlices == 0 || CopyDesc.NumMipLevels == 0 ||
        CopyDesc.Size.X <= 0 || CopyDesc.Size.Y <= 0 || CopyDesc.Size.Z < 0 ||
        (SrcDesc.Dimension == ETextureDimension::Texture3D && CopyDesc.Size.Z == 0) ||
        CopyDesc.SrcPosition.X < 0 || CopyDesc.SrcPosition.Y < 0 || CopyDesc.SrcPosition.Z < 0 ||
        CopyDesc.DstPosition.X < 0 || CopyDesc.DstPosition.Y < 0 || CopyDesc.DstPosition.Z < 0)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion requires positive XY size/counts, valid depth, and non-negative positions.");
        return;
    }

    const uint32 SrcLayers = RHIDimensionArrayLayers(SrcDesc.Dimension, SrcDesc.NumArraySlices);
    const uint32 DstLayers = RHIDimensionArrayLayers(DstDesc.Dimension, DstDesc.NumArraySlices);

    if (CopyDesc.SrcArraySlice > SrcLayers || CopyDesc.NumArraySlices > SrcLayers - CopyDesc.SrcArraySlice ||
        CopyDesc.DstArraySlice > DstLayers || CopyDesc.NumArraySlices > DstLayers - CopyDesc.DstArraySlice ||
        CopyDesc.SrcMipSlice > SrcDesc.NumMipLevels || CopyDesc.NumMipLevels > SrcDesc.NumMipLevels - CopyDesc.SrcMipSlice ||
        CopyDesc.DstMipSlice > DstDesc.NumMipLevels || CopyDesc.NumMipLevels > DstDesc.NumMipLevels - CopyDesc.DstMipSlice)
    {
        RHI_VALIDATION_ERROR("CopyTextureRegion slice or mip range exceeds the source/destination resource.");
        return;
    }

    for (uint32 MipOffset = 0; MipOffset < CopyDesc.NumMipLevels; ++MipOffset)
    {
        IntVector3 SrcExtent;
        IntVector3 DstExtent;
        if (!ValidateTextureMip("CopyTextureRegion source", SrcDesc, CopyDesc.SrcMipSlice + MipOffset, SrcExtent) ||
            !ValidateTextureMip("CopyTextureRegion destination", DstDesc, CopyDesc.DstMipSlice + MipOffset, DstExtent))
        {
            return;
        }

        const uint32 SrcX   = uint32(CopyDesc.SrcPosition.X) >> MipOffset;
        const uint32 SrcY   = uint32(CopyDesc.SrcPosition.Y) >> MipOffset;
        const uint32 SrcZ   = uint32(CopyDesc.SrcPosition.Z) >> MipOffset;
        const uint32 DstX   = uint32(CopyDesc.DstPosition.X) >> MipOffset;
        const uint32 DstY   = uint32(CopyDesc.DstPosition.Y) >> MipOffset;
        const uint32 DstZ   = uint32(CopyDesc.DstPosition.Z) >> MipOffset;
        const uint32 Width  = Math::Max(uint32(CopyDesc.Size.X) >> MipOffset, 1u);
        const uint32 Height = Math::Max(uint32(CopyDesc.Size.Y) >> MipOffset, 1u);
        const uint32 Depth  = Math::Max(uint32(CopyDesc.Size.Z) >> MipOffset, 1u);

        if (SrcX > uint32(SrcExtent.X) || Width > uint32(SrcExtent.X) - SrcX ||
            SrcY > uint32(SrcExtent.Y) || Height > uint32(SrcExtent.Y) - SrcY ||
            SrcZ > uint32(SrcExtent.Z) || Depth > uint32(SrcExtent.Z) - SrcZ ||
            DstX > uint32(DstExtent.X) || Width > uint32(DstExtent.X) - DstX ||
            DstY > uint32(DstExtent.Y) || Height > uint32(DstExtent.Y) - DstY ||
            DstZ > uint32(DstExtent.Z) || Depth > uint32(DstExtent.Z) - DstZ)
        {
            RHI_VALIDATION_ERROR("CopyTextureRegion region exceeds source or destination mip extent.");
            return;
        }
    }

    CommandContext->CopyTextureRegion(Dst, Src, CopyDesc);
}

void FRHIValidationCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    if (!ValidateRecordingPhase("CopyTextureRegionToBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegionToBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureRegionToBuffer when Src is nullptr");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer source texture must be created with ETextureUsageFlags::CopySource");
        return;
    }

    if (!IsBufferValidAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (!ValidateTextureRegion2D("CopyTextureRegionToBuffer", Src->GetDesc(), SrcMipLevel, SrcRegion) ||
        DstOffset >= Dst->GetDesc().Size)
    {
        if (DstOffset >= Dst->GetDesc().Size)
        {
            RHI_VALIDATION_ERROR("CopyTextureRegionToBuffer destination offset exceeds buffer size.");
        }
        return;
    }

    CommandContext->CopyTextureRegionToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel);
}

void FRHIValidationCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    if (!ValidateRecordingPhase("CopyTextureSubresourceToBuffer"))
    {
        return;
    }

    if (!Dst)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureSubresourceToBuffer when Dst is nullptr");
        return;
    }

    if (!Src)
    {
        RHI_VALIDATION_ERROR("Invalid to call CopyTextureSubresourceToBuffer when Src is nullptr");
        return;
    }

    if (!Src->GetDesc().IsCopySource())
    {
        RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer source texture must be created with ETextureUsageFlags::CopySource");
        return;
    }

    if (!IsBufferValidAsCopyDestination(Dst->GetDesc()))
    {
        RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer destination requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    const uint32 SrcLayers = RHIDimensionArrayLayers(Src->GetDesc().Dimension, Src->GetDesc().NumArraySlices);
    if (!ValidateTextureRegion3D("CopyTextureSubresourceToBuffer", Src->GetDesc(), SrcMipLevel, SrcRegion) ||
        SrcArraySlice >= SrcLayers || DstOffset >= Dst->GetDesc().Size)
    {
        if (SrcArraySlice >= SrcLayers)
        {
            RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer source array slice exceeds texture layer count.");
        }
        else if (DstOffset >= Dst->GetDesc().Size)
        {
            RHI_VALIDATION_ERROR("CopyTextureSubresourceToBuffer destination offset exceeds buffer size.");
        }

        return;
    }

    CommandContext->CopyTextureSubresourceToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel, SrcArraySlice);
}

void FRHIValidationCommandContext::WriteFence(FRHIFence* Fence)
{
    if (!ValidateRecordingPhase("WriteFence"))
    {
        return;
    }

    if (!Fence)
    {
        RHI_VALIDATION_ERROR("Invalid to call WriteFence when Fence is nullptr");
        return;
    }

    CommandContext->WriteFence(Fence);
}

void FRHIValidationCommandContext::DiscardContents(FRHITexture* Texture)
{
    if (!ValidateRecordingPhase("DiscardContents"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call DiscardContents when Texture is nullptr");
        return;
    }

    CommandContext->DiscardContents(Texture);
}

void FRHIValidationCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildSceneAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!RayTracingScene)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildSceneAccelerationStructure when RayTracingScene is nullptr");
        return;
    }

    if (BuildDesc.NumInstances > 0 && !BuildDesc.Instances)
    {
        RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: Instances is nullptr for a non-zero instance count.");
        return;
    }

    if (BuildDesc.bUpdate && !IsEnumFlagSet(RayTracingScene->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))
    {
        RHI_VALIDATION_ERROR("BuildSceneAccelerationStructure: update requested without AllowUpdate.");
        return;
    }

    CommandContext->BuildSceneAccelerationStructure(RayTracingScene, BuildDesc);
}

void FRHIValidationCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildGeometryAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!RayTracingGeometry)
    {
        RHI_VALIDATION_ERROR("Invalid to call BuildGeometryAccelerationStructure when RayTracingGeometry is nullptr");
        return;
    }

    if (BuildDesc.GeometryType != RayTracingGeometry->GetGeometryType())
    {
        RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: build geometry type '%s' does not match the type '%s' the acceleration structure was created with.",
            ToString(BuildDesc.GeometryType), ToString(RayTracingGeometry->GetGeometryType()));
        return;
    }

    if (BuildDesc.GeometryType == ERayTracingGeometryType::ProceduralAABBs)
    {
        if (!BuildDesc.AABBBuffer || BuildDesc.NumAABBs == 0)
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure requires an AABB buffer and non-zero AABB count for procedural geometry.");
            return;
        }

        if (BuildDesc.AABBStride == 0 || (BuildDesc.AABBStride % 16) != 0)
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: AABBStride (%u) must be a non-zero multiple of 16.", BuildDesc.AABBStride);
            return;
        }
    }
    else
    {
        if (!BuildDesc.VertexBuffer || BuildDesc.NumVertices == 0 || !BuildDesc.VertexBuffer->GetDesc().IsVertexBuffer())
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure requires a vertex buffer and non-zero vertex count.");
            return;
        }

        if (BuildDesc.NumIndices > 0 &&
            (!BuildDesc.IndexBuffer || !BuildDesc.IndexBuffer->GetDesc().IsIndexBuffer() || BuildDesc.IndexFormat == EIndexFormat::Unknown))
        {
            RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure indexed geometry requires an index buffer and valid format.");
            return;
        }
    }

    if (BuildDesc.bUpdate && !IsEnumFlagSet(RayTracingGeometry->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate))
    {
        RHI_VALIDATION_ERROR("BuildGeometryAccelerationStructure: update requested without AllowUpdate.");
        return;
    }

    CommandContext->BuildGeometryAccelerationStructure(RayTracingGeometry, BuildDesc);
}

void FRHIValidationCommandContext::SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    if (!ValidateRecordingPhase("SetHitRecordLocalShaderBindings"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: ShaderBindingTable cannot be nullptr.");
        return;
    }

    if (NumBindings > 0 && !Bindings)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: Bindings is nullptr but NumBindings=%u.", NumBindings);
        return;
    }

    const FRHIShaderBindingTableDesc& TableDesc = ShaderBindingTable->GetDesc();
    uint32 RecordCount = 0;
    switch (RecordKind)
    {
        case ERayTracingShaderRecordKind::RayGeneration:
            RecordCount = TableDesc.NumRayGenerationShaders;
            break;

        case ERayTracingShaderRecordKind::Miss:
            RecordCount = TableDesc.NumMissShaders;
            break;

        case ERayTracingShaderRecordKind::Callable:
            RecordCount = TableDesc.NumCallableShaders;
            break;

        case ERayTracingShaderRecordKind::HitGroup:
            RecordCount = TableDesc.NumHitGroupRecords;
            break;

        default:
            RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: invalid record kind.");
            return;
    }

    if (RecordIndex >= RecordCount)
    {
        RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: RecordIndex %u exceeds record count %u.", RecordIndex, RecordCount);
        return;
    }

    if (!RHI::bSupportsShaderBindingTableDescriptors)
    {
        for (uint32 i = 0; i < NumBindings; ++i)
        {
            if (Bindings[i].Type != ERayTracingLocalBindingType::ConstantBuffer)
            {
                RHI_VALIDATION_ERROR("SetHitRecordLocalShaderBindings: this backend's records may only hold buffers; texture/typed-view/sampler local records are not allowed (record %u, kind %s). Use the global bindless heap.", RecordIndex, ToString(RecordKind));
                return;
            }
        }
    }

    FRHIValidationShaderBindingTable* ValidationShaderBindingTable = static_cast<FRHIValidationShaderBindingTable*>(ShaderBindingTable);
    CommandContext->SetHitRecordLocalShaderBindings(ValidationShaderBindingTable->GetRHI(), RecordKind, RecordIndex, Bindings, NumBindings);
    ValidationShaderBindingTable->MarkDirty();
}

void FRHIValidationCommandContext::BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ValidateRecordingPhase("BuildShaderBindingTable"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("BuildShaderBindingTable: ShaderBindingTable cannot be nullptr.");
        return;
    }

    FRHIValidationShaderBindingTable* ValidationShaderBindingTable = static_cast<FRHIValidationShaderBindingTable*>(ShaderBindingTable);
    CommandContext->BuildShaderBindingTable(ValidationShaderBindingTable->GetRHI());
    ValidationShaderBindingTable->MarkBuilt();
}

void FRHIValidationCommandContext::ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (!ValidateRecordingPhase("ResetShaderBindingTable"))
    {
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("ResetShaderBindingTable: ShaderBindingTable cannot be nullptr.");
        return;
    }

    FRHIValidationShaderBindingTable* ValidationShaderBindingTable = static_cast<FRHIValidationShaderBindingTable*>(ShaderBindingTable);
    CommandContext->ResetShaderBindingTable(ValidationShaderBindingTable->GetRHI());
    ValidationShaderBindingTable->MarkDirty();
}

void FRHIValidationCommandContext::DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth)
{
    if (ContextPhase != ECommandContextPhase::Recording || !RHI::bSupportsRayTracing)
    {
        RHI_VALIDATION_ERROR("DispatchRays requires ray-tracing support and a recording context outside a render pass.");
        return;
    }

    if (!ShaderBindingTable)
    {
        RHI_VALIDATION_ERROR("DispatchRays: ShaderBindingTable cannot be nullptr.");
        return;
    }

    if (!RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRays requires SetRayTracingPipelineState first.");
        return;
    }

    if (ShaderBindingTable->GetDesc().Pipeline != RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRays: ShaderBindingTable was created for a different ray-tracing pipeline.");
        return;
    }

    FRHIValidationShaderBindingTable* ValidationShaderBindingTable = static_cast<FRHIValidationShaderBindingTable*>(ShaderBindingTable);
    if (!ValidationShaderBindingTable->IsBuilt())
    {
        RHI_VALIDATION_ERROR("DispatchRays: ShaderBindingTable must be built after its last mutation.");
        return;
    }

    const uint32 NumHitGroupExports = RayTracingPipelineState->GetNumExportNames(ERayTracingShaderRecordKind::HitGroup);
    if (NumHitGroupExports > 0 && ShaderBindingTable->GetDesc().NumHitGroupRecords == 0)
    {
        RHI_VALIDATION_ERROR("DispatchRays: ShaderBindingTable has no hit-group records while the bound pipeline declares %u hit-group export(s). Any ray that hits geometry would read a shader record from a null table.", NumHitGroupExports);
        return;
    }

    if (Width == 0 || Height == 0 || Depth == 0)
    {
        RHI_VALIDATION_ERROR("DispatchRays: dispatch dimensions must be non-zero (%u, %u, %u).", Width, Height, Depth);
        return;
    }

    CommandContext->DispatchRays(ValidationShaderBindingTable->GetRHI(), Width, Height, Depth);
}

void FRHIValidationCommandContext::DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    if (ContextPhase != ECommandContextPhase::Recording || !RHI::bSupportsRayTracing || !RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect requires a ray-tracing pipeline and a recording context outside a render pass.");
        return;
    }

    if (!RHI::bSupportsDispatchRaysIndirect)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: indirect ray dispatch is not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (!ShaderBindingTable || !ArgumentBuffer)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: ShaderBindingTable and ArgumentBuffer must both be non-null.");
        return;
    }

    if (ShaderBindingTable->GetDesc().Pipeline != RayTracingPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: ShaderBindingTable was created for a different ray-tracing pipeline.");
        return;
    }

    FRHIValidationShaderBindingTable* ValidationShaderBindingTable = static_cast<FRHIValidationShaderBindingTable*>(ShaderBindingTable);
    if (!ValidationShaderBindingTable->IsBuilt())
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: ShaderBindingTable must be built after its last mutation.");
        return;
    }

    const FRHIShaderBindingTableAddressInfo AddressInfo = ShaderBindingTable->GetAddressInfo();
    if (AddressInfo.RayGeneration.StartAddress == 0 || AddressInfo.RayGeneration.SizeInBytes == 0)
    {
        RHI_VALIDATION_ERROR("DispatchRaysIndirect: ShaderBindingTable has no valid ray-generation address range.");
        return;
    }

    if (!ValidateIndirectArguments<FRHIDispatchRaysIndirectParameters>(
        "DispatchRaysIndirect", ArgumentBuffer, ArgumentBufferOffset, 1))
    {
        return;
    }

    CommandContext->DispatchRaysIndirect(ValidationShaderBindingTable->GetRHI(), ArgumentBuffer, ArgumentBufferOffset);
}

void FRHIValidationCommandContext::BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
    if (!ValidateRecordingPhase("BuildOpacityMicromap"))
    {
        return;
    }

    if (!RHI::bSupportsOpacityMicromap)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: opacity micromaps are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (!OpacityMicromap)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: OpacityMicromap cannot be nullptr.");
        return;
    }

    if (!BuildDesc.OMMDescriptorBuffer)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: BuildDesc.OMMDescriptorBuffer cannot be nullptr.");
        return;
    }

    if (BuildDesc.NumOpacityMicromaps == 0)
    {
        RHI_VALIDATION_ERROR("BuildOpacityMicromap: BuildDesc.NumOpacityMicromaps must be non-zero.");
        return;
    }

    if (!BuildDesc.HistogramEntries.IsEmpty())
    {
        uint64 HistogramTotal = 0;
        for (const FRHIOpacityMicromapHistogramEntry& Entry : BuildDesc.HistogramEntries)
        {
            HistogramTotal += Entry.Count;
        }

        if (HistogramTotal != BuildDesc.NumOpacityMicromaps)
        {
            RHI_VALIDATION_ERROR("BuildOpacityMicromap: histogram entry counts must sum to BuildDesc.NumOpacityMicromaps.");
            return;
        }
    }

    CommandContext->BuildOpacityMicromap(OpacityMicromap, BuildDesc);
}

void FRHIValidationCommandContext::ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations)
{
    if (!ValidateRecordingPhase("ExecuteIndirectRayTracingAccelerationStructureOperations"))
    {
        return;
    }

    if (!RHI::bSupportsIndirectAccelerationStructureOperations)
    {
        RHI_VALIDATION_ERROR("ExecuteIndirectRayTracingAccelerationStructureOperations: indirect AS operations are not supported on this backend (see RHI.DumpRayTracingCaps).");
        return;
    }

    if (NumOperations > 0 && !Operations)
    {
        RHI_VALIDATION_ERROR("ExecuteIndirectRayTracingAccelerationStructureOperations: Operations is nullptr but NumOperations=%u.", NumOperations);
        return;
    }

    CommandContext->ExecuteIndirectRayTracingAccelerationStructureOperations(Operations, NumOperations);
}

void FRHIValidationCommandContext::WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources)
{
    if (!ValidateRecordingPhase("WriteAccelerationStructurePostBuildInfo") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: ray tracing is unsupported.");
        }

        return;
    }

    if (!DstBuffer)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: DstBuffer cannot be nullptr.");
        return;
    }

    if (NumSources == 0 || !Sources)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: at least one source acceleration structure is required.");
        return;
    }

    if (DstOffset >= DstBuffer->GetDesc().Size)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: destination offset exceeds buffer size.");
        return;
    }

    if (InfoType == EAccelerationStructurePostBuildInfoType::CompactedSize)
    {
        for (uint32 Index = 0; Index < NumSources; ++Index)
        {
            if (!Sources[Index])
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: source %u cannot be nullptr.", Index);
                return;
            }

            if (!IsEnumFlagSet(Sources[Index]->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: CompactedSize requires source %u to be built with AllowCompaction.", Index);
                return;
            }
        }
    }
    else
    {
        for (uint32 Index = 0; Index < NumSources; ++Index)
        {
            if (!Sources[Index])
            {
                RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: source %u cannot be nullptr.", Index);
                return;
            }
        }
    }

    if (InfoType == EAccelerationStructurePostBuildInfoType::ToolsVisualization && !RHI::bSupportsToolsVisualization)
    {
        RHI_VALIDATION_ERROR("WriteAccelerationStructurePostBuildInfo: ToolsVisualization requires RHI::bSupportsToolsVisualization.");
        return;
    }

    CommandContext->WriteAccelerationStructurePostBuildInfo(DstBuffer, DstOffset, InfoType, Sources, NumSources);
}

void FRHIValidationCommandContext::CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode)
{
    if (!ValidateRecordingPhase("CopyAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: ray tracing is unsupported.");
        }

        return;
    }

    if (!Destination || !Source)
    {
        RHI_VALIDATION_ERROR("CopyAccelerationStructure: Destination and Source must both be non-null.");
        return;
    }

    if (CopyMode == EAccelerationStructureCopyMode::Compact)
    {
        if (!IsEnumFlagSet(Source->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: Compact copy requires the source to be built with AllowCompaction.");
            return;
        }

        if (Destination == Source)
        {
            RHI_VALIDATION_ERROR("CopyAccelerationStructure: Compact copy requires a destination distinct from the source. Use CompactAccelerationStructure for in-place compaction.");
            return;
        }
    }

    if (CopyMode == EAccelerationStructureCopyMode::ToolsVisualizationDecode && !RHI::bSupportsToolsVisualization)
    {
        RHI_VALIDATION_ERROR("CopyAccelerationStructure: ToolsVisualizationDecode requires RHI::bSupportsToolsVisualization.");
        return;
    }

    CommandContext->CopyAccelerationStructure(Destination, Source, CopyMode);
}

void FRHIValidationCommandContext::CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes)
{
    if (!ValidateRecordingPhase("CompactAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("CompactAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!AccelerationStructure)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: AccelerationStructure must be non-null.");
        return;
    }

    if (!IsEnumFlagSet(AccelerationStructure->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: requires the structure to be built with AllowCompaction.");
        return;
    }

    if (CompactedSizeInBytes == 0)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: compacted size must be non-zero.");
        return;
    }

    if (CompactedSizeInBytes == 0)
    {
        RHI_VALIDATION_ERROR("CompactAccelerationStructure: CompactedSizeInBytes must be non-zero (from a CompactedSize post-build query).");
        return;
    }

    CommandContext->CompactAccelerationStructure(AccelerationStructure, CompactedSizeInBytes);
}

void FRHIValidationCommandContext::SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset)
{
    if (!ValidateRecordingPhase("SerializeAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("SerializeAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!DstBuffer || !Source)
    {
        RHI_VALIDATION_ERROR("SerializeAccelerationStructure: DstBuffer and Source must both be non-null.");
        return;
    }

    CommandContext->SerializeAccelerationStructure(Source, DstBuffer, DstOffset);
}

void FRHIValidationCommandContext::DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset)
{
    if (!ValidateRecordingPhase("DeserializeAccelerationStructure") || !RHI::bSupportsRayTracing)
    {
        if (!RHI::bSupportsRayTracing)
        {
            RHI_VALIDATION_ERROR("DeserializeAccelerationStructure: ray tracing is unsupported.");
        }
        return;
    }

    if (!Destination || !SourceBuffer)
    {
        RHI_VALIDATION_ERROR("DeserializeAccelerationStructure: Destination and SourceBuffer must both be non-null.");
        return;
    }

    CommandContext->DeserializeAccelerationStructure(Destination, SourceBuffer, SourceOffset);
}

FRHIValidationCommandContext::FOpenSplitKey FRHIValidationCommandContext::MakeSplitKey(const FRHITransitionBarrierDesc& Desc)
{
    FOpenSplitKey Key;
    if (Desc.IsTexture())
    {
        Key.Resource        = Desc.Texture.Resource;
        Key.FirstMipLevel   = Desc.Texture.Subresources.FirstMipLevel;
        Key.NumMipLevels    = Desc.Texture.Subresources.NumMipLevels;
        Key.FirstArraySlice = Desc.Texture.Subresources.FirstArraySlice;
        Key.NumArraySlices  = Desc.Texture.Subresources.NumArraySlices;
    }
    else
    {
        Key.Resource        = Desc.Buffer.Resource;
        Key.FirstMipLevel   = 0;
        Key.NumMipLevels    = RHI_ALL_MIP_LEVELS;
        Key.FirstArraySlice = 0;
        Key.NumArraySlices  = RHI_ALL_ARRAY_SLICES;
    }

    return Key;
}

int32 FRHIValidationCommandContext::FindOpenSplit(const FOpenSplitKey& Key) const
{
    for (int32 Index = 0; Index < OpenSplits.Size(); Index++)
    {
        if (OpenSplits[Index].Key == Key)
        {
            return Index;
        }
    }

    return -1;
}

bool FRHIValidationCommandContext::ValidateNoOpenSplit(const void* Resource, const CHAR* Caller) const
{
    for (const FOpenSplit& Split : OpenSplits)
    {
        if (Split.Key.Resource == Resource)
        {
            RHI_VALIDATION_ERROR("Invalid to call %s on a resource with a split barrier still in flight. "
                "Complete the split with a matching EndOnly barrier first", Caller);
            return false;
        }
    }

    return true;
}

bool FRHIValidationCommandContext::ValidateTransitionBarrierDesc(const FRHITransitionBarrierDesc& Desc)
{
    const bool bIsSplitBegin = Desc.IsSplitBegin();
    const bool bIsSplitEnd   = Desc.IsSplitEnd();

    if (bIsSplitBegin && bIsSplitEnd)
    {
        RHI_VALIDATION_ERROR("A transition barrier cannot set both BeginOnly and EndOnly");
        return false;
    }

    // The after-state-only factories encode themselves as BeforeState == AfterState
    const bool bInferBeforeState = (Desc.BeforeState == Desc.AfterState);

    ERHIResourceStateTrackingMode TrackingMode = ERHIResourceStateTrackingMode::Tracked;
    const void*                   Resource     = nullptr;

    if (Desc.IsTexture())
    {
        FRHITexture* Texture = Desc.Texture.Resource;
        if (!Texture)
        {
            RHI_VALIDATION_ERROR("Invalid to call TransitionBarrier when the texture is nullptr");
            return false;
        }

        const FRHITextureDesc& TextureDesc  = Texture->GetDesc();
        const ETextureUsageFlags Usage      = TextureDesc.UsageFlags;

        Resource     = Texture;
        TrackingMode = TextureDesc.TrackingMode;

        const bool bCopyDest   = IsEnumFlagSet(Desc.AfterState, ERHIResourceState::CopyDest)   || IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::CopyDest);
        const bool bCopySource = IsEnumFlagSet(Desc.AfterState, ERHIResourceState::CopySource) || IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::CopySource);

        if (bCopyDest && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopyDest))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopyDest requires ETextureUsageFlags::CopyDest");
            return false;
        }

        if (bCopySource && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopySource))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopySource requires ETextureUsageFlags::CopySource");
            return false;
        }

        const FRHITextureSubresourceRange& Subresources = Desc.Texture.Subresources;
        const uint32 NumLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

        if (Subresources.NumMipLevels != RHI_ALL_MIP_LEVELS &&
            (Subresources.FirstMipLevel + Subresources.NumMipLevels) > TextureDesc.NumMipLevels)
        {
            RHI_VALIDATION_ERROR("TransitionBarrier mip range [%u, %u) exceeds the texture's %u mip levels",
                Subresources.FirstMipLevel, Subresources.FirstMipLevel + Subresources.NumMipLevels, TextureDesc.NumMipLevels);
            return false;
        }

        if (Subresources.NumArraySlices != RHI_ALL_ARRAY_SLICES &&
            (Subresources.FirstArraySlice + Subresources.NumArraySlices) > NumLayers)
        {
            RHI_VALIDATION_ERROR("TransitionBarrier array-slice range [%u, %u) exceeds the texture's %u layers",
                Subresources.FirstArraySlice, Subresources.FirstArraySlice + Subresources.NumArraySlices, NumLayers);
            return false;
        }
    }
    else
    {
        FRHIBuffer* Buffer = Desc.Buffer.Resource;
        if (!Buffer)
        {
            RHI_VALIDATION_ERROR("Invalid to call TransitionBarrier when the buffer is nullptr");
            return false;
        }

        const FRHIBufferDesc& BufferDesc = Buffer->GetDesc();

        Resource     = Buffer;
        TrackingMode = BufferDesc.TrackingMode;

        const bool bUsesCopyDest   = IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::CopyDest)   || IsEnumFlagSet(Desc.AfterState, ERHIResourceState::CopyDest);
        const bool bUsesCopySource = IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::CopySource) || IsEnumFlagSet(Desc.AfterState, ERHIResourceState::CopySource);

        if (bUsesCopyDest && !IsBufferValidAsCopyDestination(BufferDesc))
        {
            RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopyDest requires EBufferFlags::CopyDest or ReadBack memory");
            return false;
        }

        if (bUsesCopySource && !IsBufferValidAsCopySource(BufferDesc))
        {
            RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopySource requires EBufferFlags::CopySource");
            return false;
        }

        const bool bUsesIndirectArgument =
            IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::IndirectArgument) ||
            IsEnumFlagSet(Desc.AfterState, ERHIResourceState::IndirectArgument);

        if (bUsesIndirectArgument && !BufferDesc.IsIndirectArguments())
        {
            RHI_VALIDATION_ERROR("Transitioning a buffer to/from IndirectArgument requires EBufferFlags::IndirectArguments");
            return false;
        }

        const FBufferRegion& Range = Desc.Buffer.Range;
        if (Range.Size != RHI_WHOLE_SIZE && (Range.Offset + Range.Size) > BufferDesc.Size)
        {
            RHI_VALIDATION_ERROR("TransitionBarrier byte range [%llu, %llu) exceeds the buffer's size of %llu",
                Range.Offset, Range.Offset + Range.Size, BufferDesc.Size);
            return false;
        }
    }

    if (TrackingMode == ERHIResourceStateTrackingMode::Manual && bInferBeforeState)
    {
        RHI_VALIDATION_ERROR("A Manual resource has no tracked state to infer from, so TransitionBarrier requires "
            "the two-state form. Use a Create* factory that takes both a before-state and an after-state");
        return false;
    }

    if (Desc.IsTrackingModeChange())
    {
        if (Desc.IsBuffer())
        {
            RHI_VALIDATION_ERROR("Tracking-mode changes are only implemented for textures");
            return false;
        }

        if (!Desc.Texture.Subresources.IsAllSubresources())
        {
            RHI_VALIDATION_ERROR("A tracking-mode change must cover the whole resource, not a subresource range");
            return false;
        }

        if (Desc.IsSplit())
        {
            RHI_VALIDATION_ERROR("A tracking-mode change cannot ride on a split-barrier half");
            return false;
        }

        if (Desc.NewTrackingMode == TrackingMode)
        {
            RHI_VALIDATION_ERROR("Redundant tracking-mode change: the resource is already %s", ToString(TrackingMode));
            return false;
        }
    }

    if (Desc.IsSplit() && TrackingMode != ERHIResourceStateTrackingMode::Tracked)
    {
        RHI_VALIDATION_ERROR("Split barriers are only valid on Tracked resources, but this one is %s", ToString(TrackingMode));
        return false;
    }

    if (Desc.IsSplit() && bInferBeforeState)
    {
        RHI_VALIDATION_ERROR("A split barrier needs a concrete before-state at record time, so the after-state-only form is rejected");
        return false;
    }

    const FOpenSplitKey Key        = MakeSplitKey(Desc);
    const int32         SplitIndex = FindOpenSplit(Key);

    if (bIsSplitBegin)
    {
        if (SplitIndex >= 0)
        {
            RHI_VALIDATION_ERROR("A second split-barrier begin was recorded over a range that already has one in flight");
            return false;
        }

        OpenSplits.Emplace(FOpenSplit{ Key, Desc.BeforeState, Desc.AfterState });
    }
    else if (bIsSplitEnd)
    {
        if (SplitIndex < 0)
        {
            RHI_VALIDATION_ERROR("A split-barrier end has no matching begin on this context. Both halves must be recorded on the same context");
            return false;
        }

        const FOpenSplit& Split = OpenSplits[SplitIndex];
        if (Split.BeforeState != Desc.BeforeState || Split.AfterState != Desc.AfterState)
        {
            RHI_VALIDATION_ERROR("A split-barrier end must carry the same states as its begin. Begin was %s -> %s, end is %s -> %s",
                ToString(Split.BeforeState), ToString(Split.AfterState), ToString(Desc.BeforeState), ToString(Desc.AfterState));
            return false;
        }

        OpenSplits.RemoveAt(SplitIndex);
    }
    else if (!ValidateNoOpenSplit(Resource, "TransitionBarrier"))
    {
        return false;
    }

    return true;
}

void FRHIValidationCommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
    if (!ValidateRecordingPhase("TransitionBarrier"))
    {
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionBarrier when inside a render-pass");
        return;
    }

    for (const FRHITransitionBarrierDesc& Desc : TransitionDescs)
    {
        if (!ValidateTransitionBarrierDesc(Desc))
        {
            return;
        }

        CommandContext->TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc>(&Desc, 1));
    }
}

bool FRHIValidationCommandContext::ValidateUnorderedAccessBarrierDesc(const FRHIUnorderedAccessBarrierDesc& Desc)
{
    if (Desc.IsTexture())
    {
        FRHITexture* Texture = Desc.Texture.Resource;
        if (!Texture)
        {
            RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBarrier when the texture is nullptr");
            return false;
        }

        if (!Texture->GetDesc().IsUnorderedAccessTexture())
        {
            RHI_VALIDATION_ERROR("UnorderedAccessBarrier requires UnorderedAccessTexture usage.");
            return false;
        }

        const FRHITextureDesc&             TextureDesc  = Texture->GetDesc();
        const FRHITextureSubresourceRange& Subresources = Desc.Texture.Subresources;

        const uint32 NumLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

        if (Subresources.NumMipLevels != RHI_ALL_MIP_LEVELS &&
            (Subresources.FirstMipLevel + Subresources.NumMipLevels) > TextureDesc.NumMipLevels)
        {
            RHI_VALIDATION_ERROR("UnorderedAccessBarrier mip range [%u, %u) exceeds the texture's %u mip levels",
                Subresources.FirstMipLevel, Subresources.FirstMipLevel + Subresources.NumMipLevels, TextureDesc.NumMipLevels);
            return false;
        }

        if (Subresources.NumArraySlices != RHI_ALL_ARRAY_SLICES &&
            (Subresources.FirstArraySlice + Subresources.NumArraySlices) > NumLayers)
        {
            RHI_VALIDATION_ERROR("UnorderedAccessBarrier array-slice range [%u, %u) exceeds the texture's %u layers",
                Subresources.FirstArraySlice, Subresources.FirstArraySlice + Subresources.NumArraySlices, NumLayers);
            return false;
        }

        return ValidateNoOpenSplit(Texture, "UnorderedAccessBarrier");
    }

    FRHIBuffer* Buffer = Desc.Buffer.Resource;
    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBarrier when the buffer is nullptr");
        return false;
    }

    if (!Buffer->GetDesc().IsUnorderedAccessBuffer())
    {
        RHI_VALIDATION_ERROR("UnorderedAccessBarrier requires UnorderedAccessBuffer usage.");
        return false;
    }

    const FBufferRegion& Range = Desc.Buffer.Range;
    if (Range.Size != RHI_WHOLE_SIZE && (Range.Offset + Range.Size) > Buffer->GetDesc().Size)
    {
        RHI_VALIDATION_ERROR("UnorderedAccessBarrier byte range [%llu, %llu) exceeds the buffer's size of %llu",
            Range.Offset, Range.Offset + Range.Size, Buffer->GetDesc().Size);
        return false;
    }

    return ValidateNoOpenSplit(Buffer, "UnorderedAccessBarrier");
}

void FRHIValidationCommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
    if (!ValidateRecordingPhase("UnorderedAccessBarrier"))
    {
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBarrier when inside a render-pass");
        return;
    }

    for (const FRHIUnorderedAccessBarrierDesc& Desc : BarrierDescs)
    {
        if (!ValidateUnorderedAccessBarrierDesc(Desc))
        {
            return;
        }
    }

    CommandContext->UnorderedAccessBarrier(BarrierDescs);
}

void FRHIValidationCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call Draw before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || VertexCount == 0)
    {
        RHI_VALIDATION_ERROR("Draw requires a graphics pipeline and non-zero vertex count.");
        return;
    }

    CommandContext->Draw(VertexCount, StartVertexLocation);
}

void FRHIValidationCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexed before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || IndexCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawIndexed requires a graphics pipeline and non-zero index count.");
        return;
    }

    CommandContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRHIValidationCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawInstanced before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || VertexCountPerInstance == 0 || InstanceCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawInstanced requires a graphics pipeline and non-zero vertex/instance counts.");
        return;
    }

    CommandContext->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call DrawIndexedInstanced before entering a render-pass");
        return;
    }

    if (!GraphicsPipelineState || IndexCountPerInstance == 0 || InstanceCount == 0)
    {
        RHI_VALIDATION_ERROR("DrawIndexedInstanced requires a graphics pipeline and non-zero index/instance counts.");
        return;
    }

    CommandContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FRHIValidationCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    if (ContextPhase != ECommandContextPhase::Recording)
    {
        RHI_VALIDATION_ERROR("Dispatch requires a recording context outside a render pass.");
        return;
    }

    if (!ComputePipelineState || (WorkGroupsX == 0 && WorkGroupsY == 0 && WorkGroupsZ == 0))
    {
        RHI_VALIDATION_ERROR("Dispatch requires a compute pipeline and at least one non-zero work-group dimension.");
        return;
    }

    CommandContext->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FRHIValidationCommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("DispatchMesh requires an active render pass.");
        return;
    }

    if (!MeshletPipelineState || (ThreadGroupCountX == 0 && ThreadGroupCountY == 0 && ThreadGroupCountZ == 0))
    {
        RHI_VALIDATION_ERROR("DispatchMesh requires a meshlet pipeline and at least one non-zero group dimension.");
        return;
    }

    CommandContext->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FRHIValidationCommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !GraphicsPipelineState)
    {
        RHI_VALIDATION_ERROR("DrawIndirect requires an active render pass and graphics pipeline.");
        return;
    }

    if (!RHI::bSupportsDrawIndirect || CommandCount == 0 || CommandCount > RHI::MaxDrawIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DrawIndirect is unsupported or CommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDrawIndirectParameters>("DrawIndirect", ArgumentBuffer, ArgumentBufferOffset, CommandCount))
    {
        CommandContext->DrawIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }
}

void FRHIValidationCommandContext::DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !GraphicsPipelineState)
    {
        RHI_VALIDATION_ERROR("DrawIndirectCount requires an active render pass and graphics pipeline.");
        return;
    }

    if (!RHI::bSupportsDrawIndirect || !RHI::bSupportsDrawIndirectCount || MaxCommandCount == 0 || MaxCommandCount > RHI::MaxDrawIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DrawIndirectCount is unsupported or MaxCommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDrawIndirectParameters>("DrawIndirectCount", ArgumentBuffer, ArgumentBufferOffset, MaxCommandCount) &&
        ValidateIndirectCountBuffer("DrawIndirectCount", CountBuffer, CountBufferOffset))
    {
        CommandContext->DrawIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }
}

void FRHIValidationCommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !GraphicsPipelineState)
    {
        RHI_VALIDATION_ERROR("DrawIndexedIndirect requires an active render pass and graphics pipeline.");
        return;
    }

    if (!RHI::bSupportsDrawIndirect || CommandCount == 0 || CommandCount > RHI::MaxDrawIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DrawIndexedIndirect is unsupported or CommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDrawIndexedIndirectParameters>("DrawIndexedIndirect", ArgumentBuffer, ArgumentBufferOffset, CommandCount))
    {
        CommandContext->DrawIndexedIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }
}

void FRHIValidationCommandContext::DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !GraphicsPipelineState)
    {
        RHI_VALIDATION_ERROR("DrawIndexedIndirectCount requires an active render pass and graphics pipeline.");
        return;
    }

    if (!RHI::bSupportsDrawIndirect || !RHI::bSupportsDrawIndirectCount || MaxCommandCount == 0 || MaxCommandCount > RHI::MaxDrawIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DrawIndexedIndirectCount is unsupported or MaxCommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDrawIndexedIndirectParameters>("DrawIndexedIndirectCount", ArgumentBuffer, ArgumentBufferOffset, MaxCommandCount) &&
        ValidateIndirectCountBuffer("DrawIndexedIndirectCount", CountBuffer, CountBufferOffset))
    {
        CommandContext->DrawIndexedIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }
}

void FRHIValidationCommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    if (ContextPhase != ECommandContextPhase::Recording || !ComputePipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchIndirect requires a recording context outside a render pass and a compute pipeline.");
        return;
    }

    if (!RHI::bSupportsDispatchIndirect)
    {
        RHI_VALIDATION_ERROR("DispatchIndirect is unsupported on this backend.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDispatchIndirectParameters>("DispatchIndirect", ArgumentBuffer, ArgumentBufferOffset, 1))
    {
        CommandContext->DispatchIndirect(ArgumentBuffer, ArgumentBufferOffset);
    }
}

void FRHIValidationCommandContext::DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !MeshletPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchMeshIndirect requires an active render pass and meshlet pipeline.");
        return;
    }

    if (!RHI::bSupportsDispatchMeshIndirect || CommandCount == 0 || CommandCount > RHI::MaxDispatchMeshIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DispatchMeshIndirect is unsupported or CommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDispatchMeshIndirectParameters>("DispatchMeshIndirect", ArgumentBuffer, ArgumentBufferOffset, CommandCount))
    {
        CommandContext->DispatchMeshIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }
}

void FRHIValidationCommandContext::DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    if (ContextPhase != ECommandContextPhase::InsideRenderPass || !MeshletPipelineState)
    {
        RHI_VALIDATION_ERROR("DispatchMeshIndirectCount requires an active render pass and meshlet pipeline.");
        return;
    }

    if (!RHI::bSupportsDispatchMeshIndirect || !RHI::bSupportsDispatchMeshIndirectCount || MaxCommandCount == 0 || MaxCommandCount > RHI::MaxDispatchMeshIndirectCommandCount)
    {
        RHI_VALIDATION_ERROR("DispatchMeshIndirectCount is unsupported or MaxCommandCount is outside the advertised limit.");
        return;
    }

    if (ValidateIndirectArguments<FRHIDispatchMeshIndirectParameters>("DispatchMeshIndirectCount", ArgumentBuffer, ArgumentBufferOffset, MaxCommandCount) &&
        ValidateIndirectCountBuffer("DispatchMeshIndirectCount", CountBuffer, CountBufferOffset))
    {
        CommandContext->DispatchMeshIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }
}

void FRHIValidationCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call PresentSwapChain when SwapChain is nullptr");
        return;
    }

    CommandContext->PresentSwapChain(SwapChain, bVerticalSync);
}

void FRHIValidationCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    if (!SwapChain)
    {
        RHI_VALIDATION_ERROR("Invalid to call ResizeSwapChain when SwapChain is nullptr");
        return;
    }

    CommandContext->ResizeSwapChain(SwapChain, Width, Height, Format, ColorSpace);
}

void FRHIValidationCommandContext::ClearState()
{
    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call ClearState when inside a render-pass");
        return;
    }

    if (!ActiveQueries.IsEmpty())
    {
        RHI_VALIDATION_ERROR("ClearState cannot be called while queries are active.");
        return;
    }

    CommandContext->ClearState();

    GraphicsPipelineState   = nullptr;
    ComputePipelineState    = nullptr;
    MeshletPipelineState    = nullptr;
    RayTracingPipelineState = nullptr;
}

void FRHIValidationCommandContext::Flush()
{
    CommandContext->Flush();
}

void FRHIValidationCommandContext::PushEvent(const StringView& Name)
{
    CommandContext->PushEvent(Name);
}

void FRHIValidationCommandContext::PopEvent()
{
    CommandContext->PopEvent();
}

void* FRHIValidationCommandContext::GetRHINativeCommandList()
{
    return CommandContext->GetRHINativeCommandList();
}
