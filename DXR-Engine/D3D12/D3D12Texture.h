#pragma once
#include "RHICore/RHIResources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

constexpr uint32 TEXTURE_CUBE_FACE_COUNT = 6;

class CD3D12BaseTexture : public CD3D12DeviceChild
{
public:
    CD3D12BaseTexture( CD3D12Device* InDevice )
        : CD3D12DeviceChild( InDevice )
        , Resource( nullptr )
    {
    }

    FORCEINLINE void SetResource( D3D12Resource* InResource )
    {
        Resource = InResource;
    }

    FORCEINLINE void SetShaderResourceView( CD3D12ShaderResourceView* InShaderResourceView )
    {
        ShaderResourceView = InShaderResourceView;
    }

    FORCEINLINE DXGI_FORMAT GetNativeFormat() const
    {
        return Resource->GetDesc().Format;
    }

    FORCEINLINE D3D12Resource* GetResource()
    {
        return Resource.Get();
    }

protected:
    TSharedRef<D3D12Resource>           Resource;
    TSharedRef<CD3D12ShaderResourceView> ShaderResourceView;
};

class CD3D12BaseTexture2D : public CRHITexture2D, public CD3D12BaseTexture
{
public:
    CD3D12BaseTexture2D(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : CRHITexture2D( InFormat, SizeX, SizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue )
        , CD3D12BaseTexture( InDevice )
        , RenderTargetView( nullptr )
        , DepthStencilView( nullptr )
        , UnorderedAccessView( nullptr )
    {
    }

    virtual CRHIRenderTargetView* GetRenderTargetView() const override
    {
        return RenderTargetView.Get();
    }
    virtual CRHIDepthStencilView* GetDepthStencilView() const override
    {
        return DepthStencilView.Get();
    }
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override
    {
        return UnorderedAccessView.Get();
    }

    FORCEINLINE void SetRenderTargetView( CD3D12RenderTargetView* InRenderTargetView )
    {
        RenderTargetView = InRenderTargetView;
    }

    FORCEINLINE void SetDepthStencilView( CD3D12DepthStencilView* InDepthStencilView )
    {
        DepthStencilView = InDepthStencilView;
    }

    FORCEINLINE void SetUnorderedAccessView( CD3D12UnorderedAccessView* InUnorderedAccessView )
    {
        UnorderedAccessView = InUnorderedAccessView;
    }

    FORCEINLINE CD3D12RenderTargetView* GetD3D12RenderTargetView() const
    {
        return RenderTargetView.Get();
    }

    FORCEINLINE void SetSize( uint32 InWidth, uint32 InHeight )
    {
        CRHITexture2D::SetSize( InWidth, InHeight );
    }

private:
    TSharedRef<CD3D12RenderTargetView> RenderTargetView;
    TSharedRef<CD3D12DepthStencilView> DepthStencilView;
    TSharedRef<CD3D12UnorderedAccessView> UnorderedAccessView;
};

class CD3D12BaseTexture2DArray : public CRHITexture2DArray, public CD3D12BaseTexture
{
public:
    CD3D12BaseTexture2DArray(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : CRHITexture2DArray( InFormat, SizeX, SizeY, InNumMips, InNumSamples, SizeZ, InFlags, InOptimalClearValue )
        , CD3D12BaseTexture( InDevice )
    {
    }
};

class CD3D12BaseTextureCube : public CRHITextureCube, public CD3D12BaseTexture
{
public:
    CD3D12BaseTextureCube(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : CRHITextureCube( InFormat, SizeX, InNumMips, InFlags, InOptimalClearValue )
        , CD3D12BaseTexture( InDevice )
    {
    }
};

class CD3D12BaseTextureCubeArray : public CRHITextureCubeArray, public CD3D12BaseTexture
{
public:
    CD3D12BaseTextureCubeArray(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : CRHITextureCubeArray( InFormat, SizeX, InNumMips, SizeZ, InFlags, InOptimalClearValue )
        , CD3D12BaseTexture( InDevice )
    {
    }
};

class CD3D12BaseTexture3D : public CRHITexture3D, public CD3D12BaseTexture
{
public:
    CD3D12BaseTexture3D(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : CRHITexture3D( InFormat, SizeX, SizeY, SizeZ, InNumMips, InFlags, InOptimalClearValue )
        , CD3D12BaseTexture( InDevice )
    {
    }
};

template<typename TBaseTexture>
class TD3D12BaseTexture : public TBaseTexture
{
public:
    TD3D12BaseTexture(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : TBaseTexture( InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InOptimalClearValue )
    {
    }

    virtual void SetName( const CString& InName ) override
    {
        CRHIResource::SetName( InName );
        CD3D12BaseTexture::Resource->SetName( InName );
    }

    virtual void* GetNativeResource() const override
    {
        return reinterpret_cast<void*>(CD3D12BaseTexture::Resource->GetResource());
    }

    virtual class CRHIShaderResourceView* GetShaderResourceView() const
    {
        return CD3D12BaseTexture::ShaderResourceView.Get();
    }

    virtual bool IsValid() const override
    {
        return CD3D12BaseTexture::Resource->GetResource() != nullptr;
    }
};

using CD3D12Texture2D = TD3D12BaseTexture<CD3D12BaseTexture2D>;
using CD3D12Texture2DArray = TD3D12BaseTexture<CD3D12BaseTexture2DArray>;
using CD3D12TextureCube = TD3D12BaseTexture<CD3D12BaseTextureCube>;
using CD3D12TextureCubeArray = TD3D12BaseTexture<CD3D12BaseTextureCubeArray>;
using CD3D12Texture3D = TD3D12BaseTexture<CD3D12BaseTexture3D>;

inline CD3D12BaseTexture* D3D12TextureCast( CRHITexture* Texture )
{
    Assert( Texture != nullptr );

    if ( Texture->AsTexture2D() != nullptr )
    {
        return static_cast<CD3D12Texture2D*>(Texture);
    }
    else if ( Texture->AsTexture2DArray() != nullptr )
    {
        return static_cast<CD3D12Texture2DArray*>(Texture);
    }
    else if ( Texture->AsTextureCube() != nullptr )
    {
        return static_cast<CD3D12TextureCube*>(Texture);
    }
    else if ( Texture->AsTextureCubeArray() != nullptr )
    {
        return static_cast<CD3D12TextureCubeArray*>(Texture);
    }
    else if ( Texture->AsTexture3D() != nullptr )
    {
        return static_cast<CD3D12Texture3D*>(Texture);
    }
    else
    {
        return nullptr;
    }
}

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif