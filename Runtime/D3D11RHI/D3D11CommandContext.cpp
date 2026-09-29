#include "D3D11RHI/D3D11CommandContext.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11SwapChain.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static UINT GetDepthStencilClearFlags(ID3D11DepthStencilView* View)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC ViewDesc;
    View->GetDesc(&ViewDesc);
    return D3D11_CLEAR_DEPTH | (IsStencilFormat(ViewDesc.Format) ? D3D11_CLEAR_STENCIL : 0);
}

FD3D11CommandContext::FD3D11CommandContext(FD3D11Device* InDevice)
    : IRHICommandContext()
    , FD3D11DeviceChild(InDevice)
    , Annotation(nullptr)
{
}

FD3D11CommandContext::~FD3D11CommandContext() = default;

bool FD3D11CommandContext::Initialize()
{
    GetD3D11Context()->QueryInterface(IID_PPV_ARGS(&Annotation));
    return true;
}

ID3D11DeviceContext* FD3D11CommandContext::GetD3D11Context() const
{
    return GetDevice()->GetD3D11Context();
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
    CHECK(RenderTargetView != nullptr);

    if (ID3D11RenderTargetView* D3D11View = static_cast<ID3D11RenderTargetView*>(RenderTargetView->GetRHINativeHandle()))
    {
        GetD3D11Context()->ClearRenderTargetView(D3D11View, ClearColor.XYZW);
    }
}

void FD3D11CommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    CHECK(DepthStencilView != nullptr);

    if (ID3D11DepthStencilView* D3D11View = static_cast<ID3D11DepthStencilView*>(DepthStencilView->GetRHINativeHandle()))
    {
        GetD3D11Context()->ClearDepthStencilView(D3D11View, GetDepthStencilClearFlags(D3D11View), Depth, Stencil);
    }
}

void FD3D11CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
}

void FD3D11CommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
}

void FD3D11CommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    ID3D11DeviceContext*  D3D11Context  = GetD3D11Context();
    ID3D11DeviceContext1* D3D11Context1 = GetDevice()->GetD3D11Context1();

    ID3D11RenderTargetView* RenderTargetViews[D3D11_MAX_RENDER_TARGET_COUNT] = {};

    const uint32 NumRenderTargets = Math::Min<uint32>(BeginRenderPassDesc.NumRenderTargets, D3D11_MAX_RENDER_TARGET_COUNT);
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetAttachment& CurrentAttachment = BeginRenderPassDesc.RenderTargets[Index];
        if (!CurrentAttachment.View)
        {
            continue;
        }

        ID3D11RenderTargetView* D3D11View = static_cast<ID3D11RenderTargetView*>(CurrentAttachment.View->GetRHINativeHandle());
        RenderTargetViews[Index] = D3D11View;

        if (!D3D11View)
        {
            continue;
        }

        if (CurrentAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            D3D11Context->ClearRenderTargetView(D3D11View, CurrentAttachment.ClearValue.RGBA);
        }
        else if (CurrentAttachment.LoadAction == EAttachmentLoadAction::DontCare && D3D11Context1)
        {
            D3D11Context1->DiscardView(D3D11View);
        }
    }

    const FRHIDepthStencilAttachment& CurrentDSAttachment = BeginRenderPassDesc.DepthStencilAttachment;

    ID3D11DepthStencilView* DepthStencilView = CurrentDSAttachment.View ? static_cast<ID3D11DepthStencilView*>(CurrentDSAttachment.View->GetRHINativeHandle()) : nullptr;
    if (DepthStencilView)
    {
        if (CurrentDSAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            const UINT ClearFlags = GetDepthStencilClearFlags(DepthStencilView);
            D3D11Context->ClearDepthStencilView(DepthStencilView, ClearFlags, CurrentDSAttachment.ClearValue.Depth, static_cast<UINT8>(CurrentDSAttachment.ClearValue.Stencil));
        }
        else if (CurrentDSAttachment.LoadAction == EAttachmentLoadAction::DontCare && D3D11Context1)
        {
            D3D11Context1->DiscardView(DepthStencilView);
        }
    }

    D3D11Context->OMSetRenderTargets(NumRenderTargets, RenderTargetViews, DepthStencilView);
}

void FD3D11CommandContext::EndRenderPass()
{
}

void FD3D11CommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    D3D11_VIEWPORT Viewport = {};
    Viewport.Width    = ViewportRegion.Width;
    Viewport.Height   = ViewportRegion.Height;
    Viewport.TopLeftX = ViewportRegion.PositionX;
    Viewport.TopLeftY = ViewportRegion.PositionY;
    Viewport.MaxDepth = ViewportRegion.MaxDepth;
    Viewport.MinDepth = ViewportRegion.MinDepth;

    GetD3D11Context()->RSSetViewports(1, &Viewport);
}

void FD3D11CommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    D3D11_RECT ScissorRect = {};
    ScissorRect.left   = LONG(ScissorRegion.PositionX);
    ScissorRect.right  = LONG(ScissorRegion.PositionX) + LONG(ScissorRegion.Width);
    ScissorRect.top    = LONG(ScissorRegion.PositionY);
    ScissorRect.bottom = LONG(ScissorRegion.PositionY) + LONG(ScissorRegion.Height);

    GetD3D11Context()->RSSetScissorRects(1, &ScissorRect);
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
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->Present(bVerticalSync);
}

void FD3D11CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->Resize(Width, Height, Format, ColorSpace);
}

void FD3D11CommandContext::SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata)
{
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->SetHDRMetadata(Metadata);
}

void FD3D11CommandContext::PushEvent(const StringView& Name)
{
#if D3D11_ENABLE_ANNOTATIONS
    if (Annotation)
    {
        Annotation->BeginEvent(*CharToWide(Name));
    }
#endif
}

void FD3D11CommandContext::PopEvent()
{
#if D3D11_ENABLE_ANNOTATIONS
    if (Annotation)
    {
        Annotation->EndEvent();
    }
#endif
}

void FD3D11CommandContext::ClearState()
{
    GetD3D11Context()->ClearState();
}

void FD3D11CommandContext::Flush()
{
    GetD3D11Context()->Flush();
}

void* FD3D11CommandContext::GetRHINativeCommandList()
{
    return GetD3D11Context();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
