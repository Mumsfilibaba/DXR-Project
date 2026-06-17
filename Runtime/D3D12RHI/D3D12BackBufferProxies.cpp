#include "D3D12RHI/D3D12BackBufferProxies.h"
#include "D3D12RHI/D3D12SwapChain.h"

FD3D12BackBufferProxyTextureRHI::FD3D12BackBufferProxyTextureRHI(FD3D12SwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc)
    : FD3D12TextureBase(InTextureDesc)
    , SwapChain(InSwapChain)
    , ProxyRenderTargetView(nullptr)
    , ProxyUnorderedAccessView(nullptr)
{
}

FD3D12BackBufferProxyTextureRHI::~FD3D12BackBufferProxyTextureRHI()
{
    SwapChain = nullptr;
}

void FD3D12BackBufferProxyTextureRHI::Resize(uint32 InWidth, uint32 InHeight)
{
    Desc.Extent.X = InWidth;
    Desc.Extent.Y = InHeight;
}

void FD3D12BackBufferProxyTextureRHI::SetProxyRenderTargetView(FD3D12BackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView)
{
    if (InProxyRenderTargetView)
    {
        InProxyRenderTargetView->AddRef();
    }

    ProxyRenderTargetView = InProxyRenderTargetView;
}

void FD3D12BackBufferProxyTextureRHI::SetProxyUnorderedAccessView(FD3D12BackBufferProxyUnorderedAccessViewRHI* InProxyUnorderedAccessView)
{
    if (InProxyUnorderedAccessView)
    {
        InProxyUnorderedAccessView->AddRef();
    }

    ProxyUnorderedAccessView = InProxyUnorderedAccessView;
}

FD3D12TextureRHI* FD3D12BackBufferProxyTextureRHI::GetTextureInterface() const
{
    return SwapChain ? SwapChain->GetCurrentBackBuffer() : nullptr;
}

void* FD3D12BackBufferProxyTextureRHI::GetRHINativeResource() const
{
    if (FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        return CurrentBackBuffer->GetRHINativeResource();
    }
    else
    {
        return nullptr;
    }
}

FRHIShaderResourceView* FD3D12BackBufferProxyTextureRHI::GetShaderResourceView() const
{
    return nullptr;
}

FRHIUnorderedAccessView* FD3D12BackBufferProxyTextureRHI::GetUnorderedAccessView() const
{
    return (SwapChain && SwapChain->GetDesc().IsUnorderedAccess()) ? ProxyUnorderedAccessView.Get() : nullptr;
}

FRHIRenderTargetView* FD3D12BackBufferProxyTextureRHI::GetRenderTargetView() const
{
    return (SwapChain && SwapChain->GetDesc().IsRenderTarget()) ? ProxyRenderTargetView.Get() : nullptr;
}

FRHIDepthStencilView* FD3D12BackBufferProxyTextureRHI::GetDepthStencilView() const
{
    return nullptr;
}

FRHIDescriptorHandle FD3D12BackBufferProxyTextureRHI::GetBindlessUAVHandle() const
{
    FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface();
    return CurrentBackBuffer ? CurrentBackBuffer->GetBindlessUAVHandle() : FRHIDescriptorHandle();
}

FRHIDescriptorHandle FD3D12BackBufferProxyTextureRHI::GetBindlessSRVHandle() const
{
    FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface();
    return CurrentBackBuffer ? CurrentBackBuffer->GetBindlessSRVHandle() : FRHIDescriptorHandle();
}

void FD3D12BackBufferProxyTextureRHI::SetDebugName(const String& InName)
{
    if (!SwapChain)
    {
        return;
    }

    const uint32 NumBackBuffers = SwapChain->GetBackBufferCount();
    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        if (FD3D12TextureRHI* BackBuffer = SwapChain->GetBackBufferAtIndex(Index))
        {
            BackBuffer->SetDebugName(InName);
        }
    }
}

void FD3D12BackBufferProxyTextureRHI::GetDebugName(String& OutDebugName) const
{
    if (FD3D12TextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        CurrentBackBuffer->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}

FD3D12BackBufferProxyRenderTargetViewRHI::FD3D12BackBufferProxyRenderTargetViewRHI(FD3D12SwapChainRHI* InSwapChain, FD3D12BackBufferProxyTextureRHI* InProxyTexture)
    : FD3D12RenderTargetViewBase(InProxyTexture, FRHIRenderTargetViewDesc::CreateTexture2D(InProxyTexture ? InProxyTexture->GetDesc().Format : EFormat::Unknown, 0))
    , SwapChain(InSwapChain)
{
}

FD3D12BackBufferProxyRenderTargetViewRHI::~FD3D12BackBufferProxyRenderTargetViewRHI()
{
    SwapChain = nullptr;
}

void* FD3D12BackBufferProxyRenderTargetViewRHI::GetRHINativeHandle() const
{
    FD3D12RenderTargetViewRHI* CurrentRTV = GetRenderTargetViewInterface();
    return CurrentRTV ? CurrentRTV->GetRHINativeHandle() : nullptr;
}

FD3D12RenderTargetViewRHI* FD3D12BackBufferProxyRenderTargetViewRHI::GetRenderTargetViewInterface() const
{
    return SwapChain ? SwapChain->GetCurrentBackBufferRenderTargetView() : nullptr;
}

FD3D12BackBufferProxyUnorderedAccessViewRHI::FD3D12BackBufferProxyUnorderedAccessViewRHI(FD3D12SwapChainRHI* InSwapChain, FD3D12BackBufferProxyTextureRHI* InProxyTexture)
    : FD3D12UnorderedAccessViewBase(InProxyTexture, FRHIUnorderedAccessViewDesc::CreateTexture2D(InProxyTexture ? InProxyTexture->GetDesc().Format : EFormat::Unknown, 0))
    , SwapChain(InSwapChain)
{
}

FD3D12BackBufferProxyUnorderedAccessViewRHI::~FD3D12BackBufferProxyUnorderedAccessViewRHI()
{
    SwapChain = nullptr;
}

void* FD3D12BackBufferProxyUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    FD3D12UnorderedAccessViewRHI* Current = GetUnorderedAccessViewInterface();
    return Current ? Current->GetRHINativeHandle() : nullptr;
}

FRHIDescriptorHandle FD3D12BackBufferProxyUnorderedAccessViewRHI::GetBindlessHandle() const
{
    FD3D12UnorderedAccessViewRHI* Current = GetUnorderedAccessViewInterface();
    return Current ? Current->GetBindlessHandle() : FRHIDescriptorHandle();
}

FD3D12UnorderedAccessViewRHI* FD3D12BackBufferProxyUnorderedAccessViewRHI::GetUnorderedAccessViewInterface() const
{
    return SwapChain ? SwapChain->GetCurrentBackBufferUnorderedAccessView() : nullptr;
}
