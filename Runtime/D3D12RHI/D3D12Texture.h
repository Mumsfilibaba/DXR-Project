#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12SwapChain;
class FD3D12CommandContext;

typedef TSharedRef<class FD3D12Texture>           FD3D12TextureRef;
typedef TSharedRef<class FD3D12BackBufferTexture> FD3D12BackBufferTextureRef;

class FD3D12Texture : public FRHITexture, public FD3D12GenericResource
{
public:
    static FD3D12Texture* Cast(FRHITexture* Texture);

public:
    FD3D12Texture(FD3D12Device* InDevice, const FRHITextureInfo& InTextureInfo);
    virtual ~FD3D12Texture();

    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override { return reinterpret_cast<void*>(ResourceStorage.GetResource()); }
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    FD3D12RenderTargetView* GetOrCreateRenderTargetView(const FRHIRenderTargetView& RenderTargetView);
    FD3D12DepthStencilView* GetOrCreateDepthStencilView(const FRHIDepthStencilView& DepthStencilView);
    void DestroyRenderTargetViews();
    void DestroyDepthStencilViews();

    DXGI_FORMAT GetDXGIFormat() const 
    { 
        return ResourceStorage.GetResource() ? ResourceStorage.GetResource()->GetDesc().Format : DXGI_FORMAT_UNKNOWN; 
    }

    void SetShaderResourceView(FD3D12ShaderResourceView* InShaderResourceView)
    { 
        ShaderResourceView = InShaderResourceView; 
    }
    
    void SetUnorderedAccessView(FD3D12UnorderedAccessView* InUnorderedAccessView) 
    { 
        UnorderedAccessView = InUnorderedAccessView; 
    }
    
    void SetResource(FD3D12Resource* InResource) 
    { 
        ResourceStorage.ReleaseResource();
        
        if (InResource)
        {
            ResourceStorage.InitStandalone(InResource);
        }

        RenderTargetViews.Clear();
        DepthStencilViews.Clear();
		RenderTargetViewMap.Clear();
		DepthStencilViewMap.Clear();
    }

protected:
    using FRenderTargetViewMap = TMap<FD3D12HashableTextureView, FD3D12RenderTargetViewRef>;
    using FDepthStencilViewMap = TMap<FD3D12HashableTextureView, FD3D12DepthStencilViewRef>;

    FD3D12ShaderResourceViewRef       ShaderResourceView;
    FD3D12UnorderedAccessViewRef      UnorderedAccessView;
    TArray<FD3D12RenderTargetViewRef> RenderTargetViews;
    TArray<FD3D12DepthStencilViewRef> DepthStencilViews;
    FRenderTargetViewMap              RenderTargetViewMap;
    FDepthStencilViewMap              DepthStencilViewMap;
};

class FD3D12BackBufferTexture : public FD3D12Texture
{
public:
    FD3D12BackBufferTexture(FD3D12Device* InDevice, FD3D12SwapChain* InSwapChain, const FRHITextureInfo& InTextureInfo);
    virtual ~FD3D12BackBufferTexture();

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override final
    {
        FD3D12Texture* CurrentBackBuffer = GetCurrentBackBufferTexture();
        return CurrentBackBuffer ? reinterpret_cast<void*>(CurrentBackBuffer->GetResource()) : nullptr;
    }

    void Resize(uint32 InWidth, uint32 InHeight);
    FD3D12Texture* GetCurrentBackBufferTexture() const;

    FD3D12SwapChain* GetSwapChain() const
    { 
        return SwapChain;
    }

    void SetSwapChain(FD3D12SwapChain* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FD3D12SwapChain* SwapChain;
};
