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

void FRHIValidationCommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    if (!ValidateRecordingPhase("TransitionTextureState"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionTextureState when Texture is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionTextureState when inside a render-pass");
		return;
	}

    {
        const ETextureUsageFlags Usage = Texture->GetDesc().UsageFlags;
        const bool bCopyDest = IsEnumFlagSet(TextureTransition.AfterState, EResourceAccess::CopyDest) ||
            IsEnumFlagSet(TextureTransition.BeforeState, EResourceAccess::CopyDest);
        const bool bCopySource = IsEnumFlagSet(TextureTransition.AfterState, EResourceAccess::CopySource) ||
            IsEnumFlagSet(TextureTransition.BeforeState, EResourceAccess::CopySource);
        
        if (bCopyDest && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopyDest))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopyDest requires ETextureUsageFlags::CopyDest");
            return;
        }

        if (bCopySource && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopySource))
        {
            RHI_VALIDATION_ERROR("Transitioning a texture to/from CopySource requires ETextureUsageFlags::CopySource");
            return;
        }
    }

    CommandContext->TransitionTextureState(Texture, TextureTransition);
}

void FRHIValidationCommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    if (!ValidateRecordingPhase("TransitionBufferState"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call TransitionBufferState when Buffer is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call TransitionBufferState when inside a render-pass");
		return;
	}

    const FRHIBufferDesc& BufferDesc = Buffer->GetDesc();

    const bool bUsesCopyDest   = IsEnumFlagSet(BeforeState, EResourceAccess::CopyDest) || IsEnumFlagSet(AfterState, EResourceAccess::CopyDest);
    const bool bUsesCopySource = IsEnumFlagSet(BeforeState, EResourceAccess::CopySource) || IsEnumFlagSet(AfterState, EResourceAccess::CopySource);

    if (bUsesCopyDest && !IsBufferValidAsCopyDestination(BufferDesc))
    {
        RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopyDest requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (bUsesCopySource && !IsBufferValidAsCopySource(BufferDesc))
    {
        RHI_VALIDATION_ERROR("Transitioning a buffer to/from CopySource requires EBufferFlags::CopySource");
        return;
    }

    const bool bUsesIndirectArgument =
        IsEnumFlagSet(BeforeState, EResourceAccess::IndirectArgument) ||
        IsEnumFlagSet(AfterState, EResourceAccess::IndirectArgument);
    if (bUsesIndirectArgument && !BufferDesc.IsIndirectArguments())
    {
        RHI_VALIDATION_ERROR("Transitioning a buffer to/from IndirectArgument requires EBufferFlags::IndirectArguments");
        return;
    }

    CommandContext->TransitionBufferState(Buffer, BeforeState, AfterState);
}

void FRHIValidationCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    if (!ValidateRecordingPhase("RequireTextureState"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireTextureState when Texture is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireTextureState when inside a render-pass");
        return;
    }

    {
        const ETextureUsageFlags Usage = Texture->GetDesc().UsageFlags;
        if (IsEnumFlagSet(RequiredState.State, EResourceAccess::CopyDest) && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopyDest))
        {
            RHI_VALIDATION_ERROR("Requiring a texture in CopyDest requires ETextureUsageFlags::CopyDest");
            return;
        }
        
        if (IsEnumFlagSet(RequiredState.State, EResourceAccess::CopySource) && !IsEnumFlagSet(Usage, ETextureUsageFlags::CopySource))
        {
            RHI_VALIDATION_ERROR("Requiring a texture in CopySource requires ETextureUsageFlags::CopySource");
            return;
        }
    }

    const FRHITextureDesc& TextureDesc = Texture->GetDesc();
    const uint32 NumLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);

    if ((RequiredState.MipLevel != RHI_ALL_MIP_LEVELS && RequiredState.MipLevel >= TextureDesc.NumMipLevels) ||
        (RequiredState.ArraySlice != RHI_ALL_ARRAY_SLICES && RequiredState.ArraySlice >= NumLayers))
    {
        RHI_VALIDATION_ERROR("RequireTextureState mip level or array slice exceeds the texture.");
        return;
    }

    CommandContext->RequireTextureState(Texture, RequiredState);
}

void FRHIValidationCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
    if (!ValidateRecordingPhase("RequireBufferState"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireBufferState when Buffer is nullptr");
        return;
    }

    if (ContextPhase == ECommandContextPhase::InsideRenderPass)
    {
        RHI_VALIDATION_ERROR("Invalid to call RequireBufferState when inside a render-pass");
        return;
    }

    if (IsEnumFlagSet(RequiredState, EResourceAccess::CopyDest) && !IsBufferValidAsCopyDestination(Buffer->GetDesc()))
    {
        RHI_VALIDATION_ERROR("Requiring a buffer in CopyDest requires EBufferFlags::CopyDest or ReadBack memory");
        return;
    }

    if (IsEnumFlagSet(RequiredState, EResourceAccess::CopySource) && !IsBufferValidAsCopySource(Buffer->GetDesc()))
    {
        RHI_VALIDATION_ERROR("Requiring a buffer in CopySource requires EBufferFlags::CopySource");
        return;
    }

    if (IsEnumFlagSet(RequiredState, EResourceAccess::IndirectArgument) && !Buffer->GetDesc().IsIndirectArguments())
    {
        RHI_VALIDATION_ERROR("Requiring a buffer in IndirectArgument requires EBufferFlags::IndirectArguments");
        return;
    }

    CommandContext->RequireBufferState(Buffer, RequiredState);
}

void FRHIValidationCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    if (!ValidateRecordingPhase("UnorderedAccessTextureBarrier"))
    {
        return;
    }

    if (!Texture)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessTextureBarrier when Texture is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessTextureBarrier when inside a render-pass");
		return;
	}

    if (!Texture->GetDesc().IsUnorderedAccessTexture())
    {
        RHI_VALIDATION_ERROR("UnorderedAccessTextureBarrier requires UnorderedAccessTexture usage.");
        return;
    }

    CommandContext->UnorderedAccessTextureBarrier(Texture);
}

void FRHIValidationCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    if (!ValidateRecordingPhase("UnorderedAccessBufferBarrier"))
    {
        return;
    }

    if (!Buffer)
    {
        RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBufferBarrier when Buffer is nullptr");
        return;
    }

	if (ContextPhase == ECommandContextPhase::InsideRenderPass)
	{
		RHI_VALIDATION_ERROR("Invalid to call UnorderedAccessBufferBarrier when inside a render-pass");
		return;
	}

    if (!Buffer->GetDesc().IsUnorderedAccessBuffer())
    {
        RHI_VALIDATION_ERROR("UnorderedAccessBufferBarrier requires UnorderedAccessBuffer usage.");
        return;
    }

    CommandContext->UnorderedAccessBufferBarrier(Buffer);
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
