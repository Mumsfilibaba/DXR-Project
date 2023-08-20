#pragma once
#include "D3D12Resource.h"
#include "D3D12ResourceViews.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FD3D12Viewport;

typedef TSharedRef<class FD3D12Texture>           FD3D12TextureRef;
typedef TSharedRef<class FD3D12BackBufferTexture> FD3D12BackBufferTextureRef;


class FD3D12Texture : public FRHITexture, public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Texture(FD3D12Device* InDevice, const FRHITextureDesc& InDesc);
    virtual ~FD3D12Texture() = default;

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    virtual int32 AddRef() override final { return FD3D12RefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FD3D12RefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

    virtual void* GetRHIBaseTexture() override { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }
    
    virtual void* GetRHIBaseResource() const override { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIShaderResourceView* GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;
    
    virtual FString GetName() const override final;

    FD3D12RenderTargetView* GetOrCreateRTV(const FRHIRenderTargetView& RenderTargetView);

    FD3D12DepthStencilView* GetOrCreateDSV(const FRHIDepthStencilView& DepthStencilView);

    void DestroyRTVs() { RenderTargetViews.Clear(); }

    void DestroyDSVs() { DepthStencilViews.Clear(); }

    FD3D12Resource* GetD3D12Resource() const 
    { 
        return Resource.Get(); 
    }

    DXGI_FORMAT GetDXGIFormat() const 
    { 
        return Resource ? Resource->GetDesc().Format : DXGI_FORMAT_UNKNOWN; 
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
        Resource = InResource; 
        RenderTargetViews.Clear();
        DepthStencilViews.Clear();
    }

protected:
    FD3D12ResourceRef                 Resource;
    FD3D12ShaderResourceViewRef       ShaderResourceView;
    FD3D12UnorderedAccessViewRef      UnorderedAccessView;

    TArray<FD3D12RenderTargetViewRef> RenderTargetViews;
    TArray<FD3D12DepthStencilViewRef> DepthStencilViews;
};


class FD3D12BackBufferTexture : public FD3D12Texture
{
public:
    FD3D12BackBufferTexture(FD3D12Device* InDevice, FD3D12Viewport* InViewport, const FRHITextureDesc& InDesc)
        : FD3D12Texture(InDevice, InDesc)
        , Viewport(InViewport)
    {
    }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(GetCurrentBackBufferTexture()); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    FD3D12Texture* GetCurrentBackBufferTexture();

    FD3D12Viewport* GetViewport() const
    { 
        return Viewport;
    }

    void SetViewport(FD3D12Viewport* InViewport)
    {
        Viewport = InViewport;
    }

private:
    FD3D12Viewport* Viewport;
};


FORCEINLINE FD3D12Texture* GetD3D12Texture(FRHITexture* Texture)
{
    if (Texture)
    {
        FD3D12Texture* D3D12Texture = nullptr;
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            FD3D12BackBufferTexture* BackBuffer = static_cast<FD3D12BackBufferTexture*>(Texture);
            D3D12Texture = BackBuffer->GetCurrentBackBufferTexture();
        }
        else
        {
            D3D12Texture = static_cast<FD3D12Texture*>(Texture);
        }

        return D3D12Texture;
    }

    return nullptr;
}

FORCEINLINE FD3D12Resource* GetD3D12Resource(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    return D3D12Texture ? D3D12Texture->GetD3D12Resource() : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING