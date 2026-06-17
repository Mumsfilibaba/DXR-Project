#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12SwapChainRHI;
class FD3D12BackBufferProxyTextureRHI;
class FD3D12BackBufferProxyRenderTargetViewRHI;
class FD3D12BackBufferProxyUnorderedAccessViewRHI;

typedef TSharedRef<FD3D12BackBufferProxyTextureRHI>             FD3D12BackBufferProxyTextureRHIRef;
typedef TSharedRef<FD3D12BackBufferProxyRenderTargetViewRHI>    FD3D12BackBufferProxyRenderTargetViewRHIRef;
typedef TSharedRef<FD3D12BackBufferProxyUnorderedAccessViewRHI> FD3D12BackBufferProxyUnorderedAccessViewRHIRef;

class FD3D12BackBufferProxyTextureRHI : public FD3D12TextureBase
{
public:
    FD3D12BackBufferProxyTextureRHI(FD3D12SwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc);
    virtual ~FD3D12BackBufferProxyTextureRHI();

    // FD3D12TextureBase Interface
    virtual FD3D12TextureRHI* GetTextureInterface() const override final;

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

    void SetProxyRenderTargetView(FD3D12BackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView);
    void SetProxyUnorderedAccessView(class FD3D12BackBufferProxyUnorderedAccessViewRHI* InProxyUnorderedAccessView);

    FD3D12SwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

    void SetSwapChain(FD3D12SwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FD3D12SwapChainRHI*                            SwapChain;
    FD3D12BackBufferProxyRenderTargetViewRHIRef    ProxyRenderTargetView;
    FD3D12BackBufferProxyUnorderedAccessViewRHIRef ProxyUnorderedAccessView;
};

class FD3D12BackBufferProxyRenderTargetViewRHI : public FD3D12RenderTargetViewBase
{
public:
    FD3D12BackBufferProxyRenderTargetViewRHI(FD3D12SwapChainRHI* InSwapChain, FD3D12BackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FD3D12BackBufferProxyRenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    // FD3D12RenderTargetViewBase Interface
    virtual FD3D12RenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    void SetSwapChain(FD3D12SwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FD3D12SwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FD3D12SwapChainRHI* SwapChain;
};

class FD3D12BackBufferProxyUnorderedAccessViewRHI : public FD3D12UnorderedAccessViewBase
{
public:
    FD3D12BackBufferProxyUnorderedAccessViewRHI(FD3D12SwapChainRHI* InSwapChain, FD3D12BackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FD3D12BackBufferProxyUnorderedAccessViewRHI();

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // FD3D12UnorderedAccessViewBase Interface
    virtual FD3D12UnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const override final;

    void SetSwapChain(FD3D12SwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FD3D12SwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FD3D12SwapChainRHI* SwapChain;
};
