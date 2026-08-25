#pragma once
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12ResourceViews.h"

class FD3D12SwapChainRHI;
class FD3D12CommandContext;

typedef TSharedRef<class FD3D12TextureRHI> FD3D12TextureRHIRef;

class FD3D12TextureRHI : public FRHITexture, public FD3D12ResourceBase
{
    friend class FD3D12SwapChainRHI;

public:
    FD3D12TextureRHI(FD3D12Device* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FD3D12TextureRHI();
    
    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final;
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final;
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(FD3D12CommandContext* InCommandContext, ERHIResourceState InInitialAccess, const IRHITextureData* InInitialData);

    void SetResourceStateTrackingMode(ERHIResourceStateTrackingMode InTrackingMode)
    {
        Desc.TrackingMode = InTrackingMode;
    }

    DXGI_FORMAT GetDXGIFormat() const 
    { 
        return ResourceStorage.GetResource() ? ResourceStorage.GetResource()->GetDesc().Format : DXGI_FORMAT_UNKNOWN; 
    }

private:
    bool InitializeSamplerFeedbackMap(ERHIResourceState InInitialAccess);
    bool InitializeSwapChainTexture();

    void SetResource(FD3D12Resource* InResource) 
    { 
        ResourceStorage.ReleaseResource();
        
        if (InResource)
        {
            ResourceStorage.InitStandalone(InResource);
        }
    }

    void SetSwapChainResource(FD3D12Resource* InResource, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    {
        Desc.Format   = InFormat;
        Desc.Extent.X = static_cast<int32>(InWidth);
        Desc.Extent.Y = static_cast<int32>(InHeight);

        SetResource(InResource);
    }

    FD3D12ShaderResourceViewRHIRef  ShaderResourceView;
    FD3D12UnorderedAccessViewRHIRef UnorderedAccessView;
    FD3D12RenderTargetViewRHIRef    RenderTargetView;
    FD3D12DepthStencilViewRHIRef    DepthStencilView;
};
