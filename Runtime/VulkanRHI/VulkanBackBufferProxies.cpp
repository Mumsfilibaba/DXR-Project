#include "VulkanRHI/VulkanBackBufferProxies.h"
#include "VulkanRHI/VulkanSwapChain.h"

FVulkanBackBufferProxyTextureRHI::FVulkanBackBufferProxyTextureRHI(FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc)
    : FVulkanTextureBase(InTextureDesc)
    , SwapChain(InSwapChain)
    , ProxyRenderTargetView(nullptr)
    , ProxyUnorderedAccessView(nullptr)
{
}

FVulkanBackBufferProxyTextureRHI::~FVulkanBackBufferProxyTextureRHI()
{
    SwapChain = nullptr;
}

void FVulkanBackBufferProxyTextureRHI::Resize(uint32 InWidth, uint32 InHeight)
{
    Desc.Extent.X = InWidth;
    Desc.Extent.Y = InHeight;
}

void FVulkanBackBufferProxyTextureRHI::SetProxyRenderTargetView(FVulkanBackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView)
{
    if (InProxyRenderTargetView)
    {
        InProxyRenderTargetView->AddRef();
    }

    ProxyRenderTargetView = InProxyRenderTargetView;
}

void FVulkanBackBufferProxyTextureRHI::SetProxyUnorderedAccessView(FVulkanBackBufferProxyUnorderedAccessViewRHI* InProxyUnorderedAccessView)
{
    if (InProxyUnorderedAccessView)
    {
        InProxyUnorderedAccessView->AddRef();
    }

    ProxyUnorderedAccessView = InProxyUnorderedAccessView;
}

FVulkanTextureRHI* FVulkanBackBufferProxyTextureRHI::GetTextureInterface() const
{
    if (!SwapChain)
    {
        return nullptr;
    }

    SwapChain->NotifyBackBufferAccessed();
    return SwapChain->GetCurrentBackBuffer();
}

void* FVulkanBackBufferProxyTextureRHI::GetRHINativeResource() const
{
    if (FVulkanTextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        return CurrentBackBuffer->GetRHINativeResource();
    }

    return nullptr;
}

FRHIShaderResourceView* FVulkanBackBufferProxyTextureRHI::GetShaderResourceView() const
{
    return nullptr;
}

FRHIUnorderedAccessView* FVulkanBackBufferProxyTextureRHI::GetUnorderedAccessView() const
{
    if (SwapChain && SwapChain->GetDesc().IsUnorderedAccess())
    {
        return ProxyUnorderedAccessView.Get();
    }
    return nullptr;
}

FRHIRenderTargetView* FVulkanBackBufferProxyTextureRHI::GetRenderTargetView() const
{
    return ProxyRenderTargetView.Get();
}

FRHIDepthStencilView* FVulkanBackBufferProxyTextureRHI::GetDepthStencilView() const
{
    return nullptr;
}

FRHIDescriptorHandle FVulkanBackBufferProxyTextureRHI::GetBindlessUAVHandle() const
{
    return FRHIDescriptorHandle();
}

FRHIDescriptorHandle FVulkanBackBufferProxyTextureRHI::GetBindlessSRVHandle() const
{
    return FRHIDescriptorHandle();
}

void FVulkanBackBufferProxyTextureRHI::SetDebugName(const String& InName)
{
    if (!SwapChain)
    {
        return;
    }

    const uint32 NumBackBuffers = SwapChain->GetNumBackBuffers();
    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        if (FVulkanTextureRHI* BackBuffer = SwapChain->GetBackBufferAtIndex(Index))
        {
            BackBuffer->SetDebugName(InName);
        }
    }
}

void FVulkanBackBufferProxyTextureRHI::GetDebugName(String& OutDebugName) const
{
    if (FVulkanTextureRHI* CurrentBackBuffer = GetTextureInterface())
    {
        CurrentBackBuffer->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}

FVulkanBackBufferProxyRenderTargetViewRHI::FVulkanBackBufferProxyRenderTargetViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture)
    : FVulkanRenderTargetViewBase(InProxyTexture, FRHIRenderTargetViewDesc::CreateTexture2D(InProxyTexture ? InProxyTexture->GetDesc().Format : EFormat::Unknown, 0))
    , SwapChain(InSwapChain)
{
}

FVulkanBackBufferProxyRenderTargetViewRHI::~FVulkanBackBufferProxyRenderTargetViewRHI()
{
    SwapChain = nullptr;
}

FVulkanRenderTargetViewRHI* FVulkanBackBufferProxyRenderTargetViewRHI::GetRenderTargetViewInterface() const
{
    if (!SwapChain)
    {
        return nullptr;
    }

    SwapChain->NotifyBackBufferAccessed();
    return SwapChain->GetCurrentBackBufferRenderTargetView();
}

void* FVulkanBackBufferProxyRenderTargetViewRHI::GetRHINativeHandle() const
{
    FVulkanRenderTargetViewRHI* CurrentRTV = GetRenderTargetViewInterface();
    return CurrentRTV ? CurrentRTV->GetRHINativeHandle() : nullptr;
}

FVulkanBackBufferProxyUnorderedAccessViewRHI::FVulkanBackBufferProxyUnorderedAccessViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture)
    : FVulkanUnorderedAccessViewBase(InProxyTexture, FRHIUnorderedAccessViewDesc::CreateTexture2D(InProxyTexture ? InProxyTexture->GetDesc().Format : EFormat::Unknown, 0))
    , SwapChain(InSwapChain)
{
}

FVulkanBackBufferProxyUnorderedAccessViewRHI::~FVulkanBackBufferProxyUnorderedAccessViewRHI()
{
    SwapChain = nullptr;
}

FVulkanUnorderedAccessViewRHI* FVulkanBackBufferProxyUnorderedAccessViewRHI::GetUnorderedAccessViewInterface() const
{
    if (!SwapChain)
    {
        return nullptr;
    }

    SwapChain->NotifyBackBufferAccessed();
    return SwapChain->GetCurrentBackBufferUnorderedAccessView();
}

void* FVulkanBackBufferProxyUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    FVulkanUnorderedAccessViewRHI* CurrentUAV = GetUnorderedAccessViewInterface();
    return CurrentUAV ? CurrentUAV->GetRHINativeHandle() : nullptr;
}

FRHIDescriptorHandle FVulkanBackBufferProxyUnorderedAccessViewRHI::GetBindlessHandle() const
{
    FVulkanUnorderedAccessViewRHI* CurrentUAV = GetUnorderedAccessViewInterface();
    return CurrentUAV ? CurrentUAV->GetBindlessHandle() : FRHIDescriptorHandle();
}
