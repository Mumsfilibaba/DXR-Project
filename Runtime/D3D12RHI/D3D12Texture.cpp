#include "D3D12Texture.h"
#include "D3D12CoreInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture 

CD3D12Texture::CD3D12Texture(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , ShaderResourceView(nullptr)
{ }

CD3D12RenderTargetView* CD3D12Texture::GetOrCreateRTV(const CRHIRenderTargetView& RTVInitializer)
{
    CD3D12Resource* D3D12Resource = GetD3D12Resource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    D3D12_ERROR_COND(ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, "Texture does not allow RenderTargetViews");

    const uint32 Subresource = D3D12CalcSubresource( RTVInitializer.MipLevel
                                                   , RTVInitializer.ArrayIndex
                                                   , 0
                                                   , ResourceDesc.MipLevels
                                                   , ResourceDesc.DepthOrArraySize);

    const DXGI_FORMAT DXGIFormat = ConvertFormat(RTVInitializer.Format);
    if (Subresource < uint32(RenderTargetViews.Size()))
    {
        CD3D12RenderTargetView* ExistingView = RenderTargetViews[Subresource].Get();
        if (ExistingView)
        {
            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = ExistingView->GetDesc();
            D3D12_WARNING_COND(RTVDesc.Format == DXGIFormat, "A RenderTargetView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        RenderTargetViews.Resize(Subresource + 1);
    }

    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Format = ConvertFormat(RTVInitializer.Format);
    D3D12_ERROR_COND(Desc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for RenderTargetViews");

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                Desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                Desc.Texture2DMSArray.ArraySize       = 1;
                Desc.Texture2DMSArray.FirstArraySlice = RTVInitializer.ArrayIndex;
            }
            else
            {
                Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = RTVInitializer.MipLevel;
                Desc.Texture2DArray.ArraySize       = 1;
                Desc.Texture2DArray.FirstArraySlice = RTVInitializer.ArrayIndex;
                Desc.Texture2DArray.PlaneSlice      = 0;
            }
        }
        else 
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipSlice   = RTVInitializer.MipLevel;
                Desc.Texture2D.PlaneSlice = 0;
            }
        }
    }
    else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        Desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = RTVInitializer.MipLevel;
        Desc.Texture3D.FirstWSlice = RTVInitializer.ArrayIndex;
        Desc.Texture3D.WSize       = 1;
    }
    else
    {
        D3D12_ERROR("ResourceDimension (=%s) does not support RenderTargetViews", ToString(ResourceDesc.Dimension));
    }

    CD3D12CoreInterface* D3D12CoreInterface = GetDevice()->GetCoreInterface();

    TSharedRef<CD3D12RenderTargetView> D3D12View = dbg_new CD3D12RenderTargetView(GetDevice(), D3D12CoreInterface->GetRenderTargetOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, Desc))
    {
        return nullptr;
    }
    else
    {
        RenderTargetViews[Subresource] = D3D12View;
        return D3D12View.ReleaseOwnership();
    }
}

CD3D12DepthStencilView* CD3D12Texture::GetOrCreateDSV(const CRHIDepthStencilView& DSVInitializer)
{
    CD3D12Resource* D3D12Resource = GetD3D12Resource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    D3D12_RESOURCE_DESC ResourceDesc = D3D12Resource->GetDesc();
    D3D12_ERROR_COND(ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, "Texture does not allow DepthStencilViews");

    const uint32 Subresource = D3D12CalcSubresource( DSVInitializer.MipLevel
                                                   , DSVInitializer.ArrayIndex
                                                   , 0
                                                   , ResourceDesc.MipLevels
                                                   , ResourceDesc.DepthOrArraySize);

    const DXGI_FORMAT DXGIFormat = ConvertFormat(DSVInitializer.Format);
    if (Subresource < uint32(DepthStencilViews.Size()))
    {
        CD3D12DepthStencilView* ExistingView = DepthStencilViews[Subresource].Get();
        if (ExistingView)
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = ExistingView->GetDesc();
            D3D12_WARNING_COND(DSVDesc.Format == DXGIFormat, "A DepthStencilView for this subresource already exists with another format");
            return ExistingView;
        }
    }
    else
    {
        DepthStencilViews.Resize(Subresource + 1);
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Format = ConvertFormat(DSVInitializer.Format);
    D3D12_ERROR_COND(Desc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for DepthStencilViews");

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (ResourceDesc.DepthOrArraySize > 1)
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                Desc.Texture2DMSArray.ArraySize       = 1;
                Desc.Texture2DMSArray.FirstArraySlice = DSVInitializer.ArrayIndex;
            }
            else
            {
                Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = DSVInitializer.MipLevel;
                Desc.Texture2DArray.ArraySize       = 1;
                Desc.Texture2DArray.FirstArraySlice = DSVInitializer.ArrayIndex;
            }
        }
        else 
        {
            if (ResourceDesc.SampleDesc.Count > 1)
            {
                Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                Desc.ViewDimension        = D3D12_DSV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipSlice   = DSVInitializer.MipLevel;
            }
        }
    }
    else
    {
        D3D12_ERROR("ResourceDimension (=%s) does not support DepthStencilViews", ToString(ResourceDesc.Dimension));
    }

    CD3D12CoreInterface* D3D12CoreInterface = GetDevice()->GetCoreInterface();

    TSharedRef<CD3D12DepthStencilView> D3D12View = dbg_new CD3D12DepthStencilView(GetDevice(), D3D12CoreInterface->GetDepthStencilOfflineDescriptorHeap());
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (!D3D12View->CreateView(D3D12Resource, Desc))
    {
        return nullptr;
    }
    else
    {
        DepthStencilViews[Subresource] = D3D12View;
        return D3D12View.ReleaseOwnership();
    }
}