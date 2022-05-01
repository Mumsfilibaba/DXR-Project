#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

#ifdef COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define TEXTURE_CUBE_FACE_COUNT (6)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture 

class CD3D12Texture : public CD3D12DeviceChild
{
public:

    CD3D12Texture(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Resource(nullptr)
    { }

    void SetResource(CD3D12Resource* InResource) { Resource = InResource; }

    void SetShaderResourceView(CD3D12ShaderResourceView* InShaderResourceView) { ShaderResourceView = InShaderResourceView; }

    CD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

    CD3D12ShaderResourceView* GetD3D12ShaderResourceView() const { return ShaderResourceView.Get(); }

    DXGI_FORMAT GetDXGIFormat() const { return Resource->GetDesc().Format; }

protected:
    TSharedRef<CD3D12Resource>           Resource;
    TSharedRef<CD3D12ShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIBaseTexture2D

class CD3D12RHIBaseTexture2D : public CRHITexture2D, public CD3D12Texture
{
public:
    
    CD3D12RHIBaseTexture2D( CD3D12Device* InDevice
                          , EFormat InFormat
                          , uint32 SizeX
                          , uint32 SizeY
                          , uint32 SizeZ
                          , uint32 InNumMips
                          , uint32 InNumSamples
                          , ETextureUsageFlags InFlags
                          , const SClearValue& InClearValue)
        : CRHITexture2D(InFormat, SizeX, SizeY, InNumMips, InNumSamples, InFlags, InClearValue)
        , CD3D12Texture(InDevice)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
        , UnorderedAccessView(nullptr)
    { }

    void SetRenderTargetView(CD3D12RenderTargetView* InRenderTargetView) { RenderTargetView = InRenderTargetView; }

    void SetDepthStencilView(CD3D12DepthStencilView* InDepthStencilView) { DepthStencilView = InDepthStencilView; }

    void SetUnorderedAccessView(CD3D12UnorderedAccessView* InUnorderedAccessView) { UnorderedAccessView = InUnorderedAccessView; }

    CD3D12RenderTargetView* GetD3D12RenderTargetView() const { return RenderTargetView.Get(); }

    void SetSize(uint16 InWidth, uint16 InHeight)
    {
        Width  = InWidth;
        Height = InHeight;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture2D Interface

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final { return RenderTargetView.Get(); }

    virtual CRHIDepthStencilView* GetDepthStencilView() const override final { return DepthStencilView.Get(); }
    
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }

private:
    TSharedRef<CD3D12RenderTargetView>    RenderTargetView;
    TSharedRef<CD3D12DepthStencilView>    DepthStencilView;
    TSharedRef<CD3D12UnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIBaseTexture2DArray

class CD3D12RHIBaseTexture2DArray : public CRHITexture2DArray, public CD3D12Texture
{
public:

    CD3D12RHIBaseTexture2DArray( CD3D12Device* InDevice
                               , EFormat InFormat
                               , uint32 SizeX
                               , uint32 SizeY
                               , uint32 SizeZ
                               , uint32 InNumMips
                               , uint32 InNumSamples
                               , ETextureUsageFlags InFlags
                               , const SClearValue& InClearValue)
        : CRHITexture2DArray(InFormat, SizeX, SizeY, InNumMips, InNumSamples, SizeZ, InFlags, InClearValue)
        , CD3D12Texture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIBaseTextureCube

class CD3D12RHIBaseTextureCube : public CRHITextureCube, public CD3D12Texture
{
public:

    CD3D12RHIBaseTextureCube( CD3D12Device* InDevice
                             , EFormat InFormat
                             , uint32 SizeX
                             , uint32 SizeY
                             , uint32 SizeZ
                             , uint32 InNumMips
                             , uint32 InNumSamples
                             , ETextureUsageFlags InFlags
                             , const SClearValue& InClearValue)
        : CRHITextureCube(InFormat, SizeX, InNumMips, InFlags, InClearValue)
        , CD3D12Texture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIBaseTextureCubeArray

class CD3D12RHIBaseTextureCubeArray : public CRHITextureCubeArray, public CD3D12Texture
{
public:
    
    CD3D12RHIBaseTextureCubeArray( CD3D12Device* InDevice
                                 , EFormat InFormat
                                 , uint32 SizeX
                                 , uint32 SizeY
                                 , uint32 SizeZ
                                 , uint32 InNumMips
                                 , uint32 InNumSamples
                                 , ETextureUsageFlags InFlags
                                 , const SClearValue& InClearValue)
        : CRHITextureCubeArray(InFormat, SizeX, InNumMips, SizeZ, InFlags, InClearValue)
        , CD3D12Texture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIBaseTexture3D

class CD3D12RHIBaseTexture3D : public CRHITexture3D, public CD3D12Texture
{
public:

    CD3D12RHIBaseTexture3D( CD3D12Device* InDevice
                          , EFormat InFormat
                          , uint32 SizeX
                          , uint32 SizeY
                          , uint32 SizeZ
                          , uint32 InNumMips
                          , uint32 InNumSamples
                          , ETextureUsageFlags InFlags
                          , const SClearValue& InClearValue)
        : CRHITexture3D(InFormat, SizeX, SizeY, SizeZ, InNumMips, InFlags, InClearValue)
        , CD3D12Texture(InDevice)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12RHIBaseTexture

template<typename BaseTextureType>
class TD3D12RHIBaseTexture : public BaseTextureType
{
public:

    TD3D12RHIBaseTexture(CD3D12Device* InDevice
                         , EFormat InFormat
                         , uint32 SizeX
                         , uint32 SizeY
                         , uint32 SizeZ
                         , uint32 InNumMips
                         , uint32 InNumSamples
                         , ETextureUsageFlags InFlags
                         , const SClearValue& InClearValue)
        : BaseTextureType(InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InClearValue)
    { }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const 
    { 
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture()
    { 
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessHandle() const { return CRHIDescriptorHandle(); }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Texture types

using CD3D12RHITexture2D        = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture2D>;
using CD3D12RHITexture2DArray   = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture2DArray>;
using CD3D12RHITextureCube      = TD3D12RHIBaseTexture<CD3D12RHIBaseTextureCube>;
using CD3D12RHITextureCubeArray = TD3D12RHIBaseTexture<CD3D12RHIBaseTextureCubeArray>;
using CD3D12RHITexture3D        = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture3D>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12TextureCast

inline CD3D12Texture* D3D12TextureCast(CRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CD3D12Texture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

inline CD3D12Resource* D3D12ResourceCast(CRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CD3D12Resource*>(Texture->GetRHIBaseResource()) : nullptr;
}

#ifdef COMPILER_MSVC
    #pragma warning(pop)
#endif