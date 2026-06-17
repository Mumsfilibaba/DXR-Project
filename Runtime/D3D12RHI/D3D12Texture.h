#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12SwapChainRHI;
class FD3D12CommandContext;

typedef TSharedRef<class FD3D12TextureRHI> FD3D12TextureRHIRef;

class FD3D12TextureBase : public FRHITexture
{
protected:
    explicit FD3D12TextureBase(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
    {
    }

    virtual ~FD3D12TextureBase() = default;

public:
    virtual FD3D12TextureRHI* GetTextureInterface() const = 0;
};

class FD3D12TextureRHI : public FD3D12TextureBase, public FD3D12ResourceBase
{
public:
    FD3D12TextureRHI(FD3D12Device* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FD3D12TextureRHI();
    
    // FD3D12TextureBase Interface
    virtual FD3D12TextureRHI* GetTextureInterface() const override;
    
    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final;
    
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final;
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final;

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);
    
    DXGI_FORMAT GetDXGIFormat() const 
    { 
        return ResourceStorage.GetResource() ? ResourceStorage.GetResource()->GetDesc().Format : DXGI_FORMAT_UNKNOWN; 
    }
    
    void SetResource(FD3D12Resource* InResource) 
    { 
        ResourceStorage.ReleaseResource();
        
        if (InResource)
        {
            ResourceStorage.InitStandalone(InResource);
        }
    }

protected:
    FD3D12ShaderResourceViewRHIRef  ShaderResourceView;
    FD3D12UnorderedAccessViewRHIRef UnorderedAccessView;
    FD3D12RenderTargetViewRHIRef    RenderTargetView;
    FD3D12DepthStencilViewRHIRef    DepthStencilView;
};
