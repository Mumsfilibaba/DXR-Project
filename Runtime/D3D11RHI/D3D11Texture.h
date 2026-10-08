#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11Resource.h"
#include "D3D11RHI/D3D11ResourceViews.h"

class FD3D11SwapChainRHI;

typedef TSharedRef<class FD3D11TextureRHI> FD3D11TextureRHIRef;

class FD3D11TextureRHI : public FRHITexture, public FD3D11Resource, public FD3D11DeviceChild
{
    friend class FD3D11SwapChainRHI;

public:
    FD3D11TextureRHI(FD3D11Device* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FD3D11TextureRHI();

    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final { return Resource.Get(); }

    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final { return RenderTargetView.Get(); }
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final { return DepthStencilView.Get(); }

    virtual void SetDebugName(const String& InName) override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(ERHIResourceState InInitialState, const IRHITextureData* InInitialData);

    DXGI_FORMAT GetDXGIFormat() const
    {
        return ConvertFormat(Desc.Format);
    }

private:
    bool CreateResource(DXGI_FORMAT ResourceFormat, const D3D11_SUBRESOURCE_DATA* InitialData);

    bool InitializeSwapChainTexture(const TComPtr<ID3D11Texture2D>& InBackBuffer, EFormat InFormat, uint32 InWidth, uint32 InHeight);
    void ReleaseSwapChainTexture();

    FD3D11ShaderResourceViewRHIRef  ShaderResourceView;
    FD3D11UnorderedAccessViewRHIRef UnorderedAccessView;
    FD3D11RenderTargetViewRHIRef    RenderTargetView;
    FD3D11DepthStencilViewRHIRef    DepthStencilView;
};
