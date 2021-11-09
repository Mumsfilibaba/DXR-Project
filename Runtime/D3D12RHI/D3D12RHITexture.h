#pragma once
#include "RHI/RHIResources.h"

#include "D3D12Resource.h"
#include "D3D12RHIViews.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define TEXTURE_CUBE_FACE_COUNT (6)

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12BaseTexture : public CD3D12DeviceChild
{
public:
    CD3D12BaseTexture( CD3D12Device* InDevice )
        : CD3D12DeviceChild( InDevice )
        , Resource( nullptr )
    {
    }

    FORCEINLINE void SetResource( CD3D12Resource* InResource )
    {
        Resource = InResource;
    }

    FORCEINLINE void SetShaderResourceView( CD3D12RHIShaderResourceView* InShaderResourceView )
    {
        ShaderResourceView = InShaderResourceView;
    }

    FORCEINLINE DXGI_FORMAT GetNativeFormat() const
    {
        return Resource->GetDesc().Format;
    }

    FORCEINLINE CD3D12Resource* GetResource()
    {
        return Resource.Get();
    }

protected:

    // Native resource storing the texture
    TSharedRef<CD3D12Resource>              Resource;

    // Default ShaderResourceView created at creation 
    TSharedRef<CD3D12RHIShaderResourceView> ShaderResourceView;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseTexture2D : public CRHITexture2D, public CD3D12BaseTexture
{
public:
    CD3D12RHIBaseTexture2D(
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

    virtual CRHIRenderTargetView* GetRenderTargetView() const override { return RenderTargetView.Get(); }
    virtual CRHIDepthStencilView* GetDepthStencilView() const override { return DepthStencilView.Get(); }
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

    FORCEINLINE void SetRenderTargetView( CD3D12RenderTargetView* InRenderTargetView )
    {
        RenderTargetView = InRenderTargetView;
    }

    FORCEINLINE void SetDepthStencilView( CD3D12DepthStencilView* InDepthStencilView )
    {
        DepthStencilView = InDepthStencilView;
    }

    FORCEINLINE void SetUnorderedAccessView( CD3D12RHIUnorderedAccessView* InUnorderedAccessView )
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

    // Default RenderTargetView created at creation 
    TSharedRef<CD3D12RenderTargetView> RenderTargetView;
    // Default DepthStencilView created at creation 
    TSharedRef<CD3D12DepthStencilView> DepthStencilView;
    // Default UnorderedAccessView created at creation 
    TSharedRef<CD3D12RHIUnorderedAccessView> UnorderedAccessView;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseTexture2DArray : public CRHITexture2DArray, public CD3D12BaseTexture
{
public:
    CD3D12RHIBaseTexture2DArray(
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

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseTextureCube : public CRHITextureCube, public CD3D12BaseTexture
{
public:
    CD3D12RHIBaseTextureCube(
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

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseTextureCubeArray : public CRHITextureCubeArray, public CD3D12BaseTexture
{
public:
    CD3D12RHIBaseTextureCubeArray(
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

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHIBaseTexture3D : public CRHITexture3D, public CD3D12BaseTexture
{
public:
    CD3D12RHIBaseTexture3D(
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

///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename BaseTextureType>
class TD3D12RHIBaseTexture : public BaseTextureType
{
public:
    TD3D12RHIBaseTexture(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue )
        : BaseTextureType( InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InOptimalClearValue )
    {
    }

    virtual void SetName( const CString& InName ) override
    {
        // Save the debug string for fast lookup
        CRHIResource::SetName( InName );

        // Set the native resource name
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

///////////////////////////////////////////////////////////////////////////////////////////////////

using CD3D12RHITexture2D = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture2D>;
using CD3D12RHITexture2DArray = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture2DArray>;
using CD3D12RHITextureCube = TD3D12RHIBaseTexture<CD3D12RHIBaseTextureCube>;
using CD3D12RHITextureCubeArray = TD3D12RHIBaseTexture<CD3D12RHIBaseTextureCubeArray>;
using CD3D12RHITexture3D = TD3D12RHIBaseTexture<CD3D12RHIBaseTexture3D>;

///////////////////////////////////////////////////////////////////////////////////////////////////

inline CD3D12BaseTexture* D3D12TextureCast( CRHITexture* Texture )
{
    Assert( Texture != nullptr );

    if ( Texture->AsTexture2D() != nullptr )
    {
        return static_cast<CD3D12RHITexture2D*>(Texture);
    }
    else if ( Texture->AsTexture2DArray() != nullptr )
    {
        return static_cast<CD3D12RHITexture2DArray*>(Texture);
    }
    else if ( Texture->AsTextureCube() != nullptr )
    {
        return static_cast<CD3D12RHITextureCube*>(Texture);
    }
    else if ( Texture->AsTextureCubeArray() != nullptr )
    {
        return static_cast<CD3D12RHITextureCubeArray*>(Texture);
    }
    else if ( Texture->AsTexture3D() != nullptr )
    {
        return static_cast<CD3D12RHITexture3D*>(Texture);
    }
    else
    {
        return nullptr;
    }
}

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif