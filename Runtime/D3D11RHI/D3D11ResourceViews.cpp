#include "D3D11RHI/D3D11ResourceViews.h"
#include "D3D11RHI/D3D11Device.h"

template<typename ViewType>
static FD3D11SubresourceRange GetMipLevelsRange(FRHIResource* Resource, const ViewType& View)
{
    if constexpr (requires { View.ArraySize; })
    {
        return FD3D11SubresourceRange(Resource, View.MostDetailedMip, View.MipLevels, View.FirstArraySlice, View.ArraySize);
    }
    else
    {
        return FD3D11SubresourceRange(Resource, View.MostDetailedMip, View.MipLevels);
    }
}

template<typename ViewType>
static FD3D11SubresourceRange GetMipSliceRange(FRHIResource* Resource, const ViewType& View)
{
    if constexpr (requires { View.ArraySize; })
    {
        return FD3D11SubresourceRange(Resource, View.MipSlice, 1, View.FirstArraySlice, View.ArraySize);
    }
    else
    {
        return FD3D11SubresourceRange(Resource, View.MipSlice, 1);
    }
}

static FD3D11SubresourceRange GetViewSubresourceRange(FRHIResource* Resource, const D3D11_SHADER_RESOURCE_VIEW_DESC& Desc)
{
    const D3D11_TEXCUBE_ARRAY_SRV& CubeArray = Desc.TextureCubeArray;
    switch (Desc.ViewDimension)
    {
        case D3D11_SRV_DIMENSION_TEXTURE1D:        return GetMipLevelsRange(Resource, Desc.Texture1D);
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:   return GetMipLevelsRange(Resource, Desc.Texture1DArray);
        case D3D11_SRV_DIMENSION_TEXTURE2D:        return GetMipLevelsRange(Resource, Desc.Texture2D);
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:   return GetMipLevelsRange(Resource, Desc.Texture2DArray);
        case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY: return FD3D11SubresourceRange(Resource, 0, 1, Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize);
        case D3D11_SRV_DIMENSION_TEXTURE3D:        return GetMipLevelsRange(Resource, Desc.Texture3D);
        case D3D11_SRV_DIMENSION_TEXTURECUBE:      return GetMipLevelsRange(Resource, Desc.TextureCube);
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY: return FD3D11SubresourceRange(Resource, CubeArray.MostDetailedMip, CubeArray.MipLevels, CubeArray.First2DArrayFace, CubeArray.NumCubes * 6);
        default:                                   return FD3D11SubresourceRange(Resource);
    }
}

static FD3D11SubresourceRange GetViewSubresourceRange(FRHIResource* Resource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& Desc)
{
    switch (Desc.ViewDimension)
    {
        case D3D11_UAV_DIMENSION_TEXTURE1D:      return GetMipSliceRange(Resource, Desc.Texture1D);
        case D3D11_UAV_DIMENSION_TEXTURE1DARRAY: return GetMipSliceRange(Resource, Desc.Texture1DArray);
        case D3D11_UAV_DIMENSION_TEXTURE2D:      return GetMipSliceRange(Resource, Desc.Texture2D);
        case D3D11_UAV_DIMENSION_TEXTURE2DARRAY: return GetMipSliceRange(Resource, Desc.Texture2DArray);
        case D3D11_UAV_DIMENSION_TEXTURE3D:      return GetMipSliceRange(Resource, Desc.Texture3D);
        default:                                 return FD3D11SubresourceRange(Resource);
    }
}

static FD3D11SubresourceRange GetViewSubresourceRange(FRHIResource* Resource, const D3D11_RENDER_TARGET_VIEW_DESC& Desc)
{
    switch (Desc.ViewDimension)
    {
        case D3D11_RTV_DIMENSION_TEXTURE1D:        return GetMipSliceRange(Resource, Desc.Texture1D);
        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:   return GetMipSliceRange(Resource, Desc.Texture1DArray);
        case D3D11_RTV_DIMENSION_TEXTURE2D:        return GetMipSliceRange(Resource, Desc.Texture2D);
        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:   return GetMipSliceRange(Resource, Desc.Texture2DArray);
        case D3D11_RTV_DIMENSION_TEXTURE2DMS:      return FD3D11SubresourceRange(Resource, 0, 1);
        case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY: return FD3D11SubresourceRange(Resource, 0, 1, Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize);
        case D3D11_RTV_DIMENSION_TEXTURE3D:        return GetMipSliceRange(Resource, Desc.Texture3D);
        default:                                   return FD3D11SubresourceRange(Resource);
    }
}

static FD3D11SubresourceRange GetViewSubresourceRange(FRHIResource* Resource, const D3D11_DEPTH_STENCIL_VIEW_DESC& Desc)
{
    switch (Desc.ViewDimension)
    {
        case D3D11_DSV_DIMENSION_TEXTURE1D:        return GetMipSliceRange(Resource, Desc.Texture1D);
        case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:   return GetMipSliceRange(Resource, Desc.Texture1DArray);
        case D3D11_DSV_DIMENSION_TEXTURE2D:        return GetMipSliceRange(Resource, Desc.Texture2D);
        case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:   return GetMipSliceRange(Resource, Desc.Texture2DArray);
        case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY: return FD3D11SubresourceRange(Resource, 0, 1, Desc.Texture2DMSArray.FirstArraySlice, Desc.Texture2DMSArray.ArraySize);
        default:                                   return FD3D11SubresourceRange(Resource, 0, 1);
    }
}

FD3D11ShaderResourceViewRHI::FD3D11ShaderResourceViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
    , SubresourceRange()
    , ViewDimension(D3D11_SRV_DIMENSION_UNKNOWN)
{
}

FD3D11ShaderResourceViewRHI::~FD3D11ShaderResourceViewRHI() = default;

bool FD3D11ShaderResourceViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_SHADER_RESOURCE_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateShaderResourceView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11ShaderResourceViewRHI]: FAILED to create ShaderResourceView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    SubresourceRange = GetViewSubresourceRange(GetResource(), InDesc);
    ViewDimension    = InDesc.ViewDimension;
    return true;
}

FD3D11UnorderedAccessViewRHI::FD3D11UnorderedAccessViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FRHIUnorderedAccessView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
    , SubresourceRange()
{
}

FD3D11UnorderedAccessViewRHI::~FD3D11UnorderedAccessViewRHI() = default;

bool FD3D11UnorderedAccessViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateUnorderedAccessView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11UnorderedAccessViewRHI]: FAILED to create UnorderedAccessView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    SubresourceRange = GetViewSubresourceRange(GetResource(), InDesc);
    return true;
}

FD3D11RenderTargetViewRHI::FD3D11RenderTargetViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc)
    : FRHIRenderTargetView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , View(nullptr)
    , SubresourceRange()
{
}

FD3D11RenderTargetViewRHI::~FD3D11RenderTargetViewRHI() = default;

bool FD3D11RenderTargetViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_RENDER_TARGET_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateRenderTargetView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11RenderTargetViewRHI]: FAILED to create RenderTargetView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    SubresourceRange = GetViewSubresourceRange(GetResource(), InDesc);
    return true;
}

FD3D11DepthStencilViewRHI::FD3D11DepthStencilViewRHI(FD3D11Device* InDevice, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc)
    : FRHIDepthStencilView(InResource, InRHIDesc)
    , FD3D11DeviceChild(InDevice)
    , D3D11Desc()
    , View(nullptr)
    , SubresourceRange()
{
}

FD3D11DepthStencilViewRHI::~FD3D11DepthStencilViewRHI() = default;

bool FD3D11DepthStencilViewRHI::Initialize(ID3D11Resource* InResource, const D3D11_DEPTH_STENCIL_VIEW_DESC& InDesc)
{
    View.Reset();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateDepthStencilView(InResource, &InDesc, &View);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11DepthStencilViewRHI]: FAILED to create DepthStencilView (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    D3D11Desc        = InDesc;
    SubresourceRange = GetViewSubresourceRange(GetResource(), InDesc);
    return true;
}
