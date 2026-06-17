#pragma once
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanResourceViews.h"

class FVulkanSwapChainRHI;
class FVulkanBackBufferProxyTextureRHI;
class FVulkanBackBufferProxyRenderTargetViewRHI;
class FVulkanBackBufferProxyUnorderedAccessViewRHI;

typedef TSharedRef<FVulkanBackBufferProxyTextureRHI>             FVulkanBackBufferProxyTextureRHIRef;
typedef TSharedRef<FVulkanBackBufferProxyRenderTargetViewRHI>    FVulkanBackBufferProxyRenderTargetViewRHIRef;
typedef TSharedRef<FVulkanBackBufferProxyUnorderedAccessViewRHI> FVulkanBackBufferProxyUnorderedAccessViewRHIRef;

class FVulkanBackBufferProxyTextureRHI : public FVulkanTextureBase
{
public:
    FVulkanBackBufferProxyTextureRHI(FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanBackBufferProxyTextureRHI();

    // FVulkanTextureBase Interface
    virtual FVulkanTextureRHI* GetTextureInterface() const override final;

    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;

    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final;
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    void Resize(uint32 InWidth, uint32 InHeight);

    void SetProxyRenderTargetView(FVulkanBackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView);
    void SetProxyUnorderedAccessView(FVulkanBackBufferProxyUnorderedAccessViewRHI* InProxyUnorderedAccessView);

    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FVulkanSwapChainRHI*                                     SwapChain;
    TSharedRef<FVulkanBackBufferProxyRenderTargetViewRHI>    ProxyRenderTargetView;
    TSharedRef<FVulkanBackBufferProxyUnorderedAccessViewRHI> ProxyUnorderedAccessView;
};

class FVulkanBackBufferProxyRenderTargetViewRHI : public FVulkanRenderTargetViewBase
{
public:
    FVulkanBackBufferProxyRenderTargetViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FVulkanBackBufferProxyRenderTargetViewRHI();

    // FVulkanRenderTargetViewBase Interface
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }
    
    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FVulkanSwapChainRHI* SwapChain;
};

class FVulkanBackBufferProxyUnorderedAccessViewRHI : public FVulkanUnorderedAccessViewBase
{
public:
    FVulkanBackBufferProxyUnorderedAccessViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FVulkanBackBufferProxyUnorderedAccessViewRHI();

    // FVulkanUnorderedAccessViewBase Interface
    virtual FVulkanUnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const override final;

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }
    
    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FVulkanSwapChainRHI* SwapChain;
};
