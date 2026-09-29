#include "D3D11RHI/D3D11CommandContext.h"
#include "D3D11RHI/D3D11StubResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FD3D11CommandContext::FD3D11CommandContext(FD3D11Device* InDevice)
    : IRHICommandContext()
    , FD3D11DeviceChild(InDevice)
{
}

FD3D11CommandContext::~FD3D11CommandContext() = default;

bool FD3D11CommandContext::Initialize()
{
    return true;
}

void FD3D11CommandContext::BeginFrame()
{
}

void FD3D11CommandContext::EndFrame()
{
}

void FD3D11CommandContext::StartContext()
{
}

void FD3D11CommandContext::FinishContext()
{
}

void FD3D11CommandContext::BeginQuery(FRHIQuery* Query)
{
}

void FD3D11CommandContext::EndQuery(FRHIQuery* Query)
{
}

void FD3D11CommandContext::QueryTimestamp(FRHIQuery* Query)
{
}

void FD3D11CommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
}

void FD3D11CommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
}

void FD3D11CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
}

void FD3D11CommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
}

void FD3D11CommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
}

void FD3D11CommandContext::EndRenderPass()
{
}

void FD3D11CommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
}

void FD3D11CommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
}

void FD3D11CommandContext::SetBlendFactor(const Vector4& Color)
{
}

void FD3D11CommandContext::SetStencilRef(uint32 StencilRef)
{
}

void FD3D11CommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
}

void FD3D11CommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
}

void FD3D11CommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
}

void FD3D11CommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
}

void FD3D11CommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
}

void FD3D11CommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
}

void FD3D11CommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
}

void FD3D11CommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
}

void FD3D11CommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
}

void FD3D11CommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
}

void FD3D11CommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
}

void FD3D11CommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FD3D11CommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
}

void FD3D11CommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FD3D11CommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
}

void FD3D11CommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
}

void FD3D11CommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
}

void FD3D11CommandContext::WriteFence(FRHIFence* Fence)
{
}

void FD3D11CommandContext::DiscardContents(FRHITexture* Texture)
{
}

void FD3D11CommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
}

void FD3D11CommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
}

void FD3D11CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
}

void FD3D11CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
}

void FD3D11CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
}

void FD3D11CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
}

void FD3D11CommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void FD3D11CommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
}

void FD3D11CommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
}

void FD3D11CommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
}

void FD3D11CommandContext::AcquireNextBackBuffer(FRHISwapChain* SwapChain)
{
}

void FD3D11CommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
}

void FD3D11CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    if (FD3D11StubSwapChainRHI* StubSwapChain = static_cast<FD3D11StubSwapChainRHI*>(SwapChain))
    {
        StubSwapChain->Resize(Width, Height, Format);
    }
}

void FD3D11CommandContext::SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata)
{
    if (FD3D11StubSwapChainRHI* StubSwapChain = static_cast<FD3D11StubSwapChainRHI*>(SwapChain))
    {
        StubSwapChain->SetHDRMetadata(Metadata);
    }
}

void FD3D11CommandContext::PushEvent(const StringView& Name)
{
}

void FD3D11CommandContext::PopEvent()
{
}

void FD3D11CommandContext::ClearState()
{
}

void FD3D11CommandContext::Flush()
{
}

void* FD3D11CommandContext::GetRHINativeCommandList()
{
    return nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
