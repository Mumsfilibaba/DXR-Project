#pragma once
#include "RHI/RHIResources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

#define TEXTURE_CUBE_FACE_COUNT (6)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTexture

class CD3D12BaseTexture : public CD3D12DeviceObject
{
public:
    CD3D12BaseTexture(CD3D12Device* InDevice)
        : CD3D12DeviceObject(InDevice)
        , Resource(nullptr)
    {
    }

    FORCEINLINE void SetResource(CD3D12Resource* InResource)
    {
        Resource = InResource;
    }

    FORCEINLINE void SetShaderResourceView(CD3D12ShaderResourceView* InShaderResourceView)
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
    TSharedRef<CD3D12Resource> Resource;
    // Default ShaderResourceView created at creation 
    TSharedRef<CD3D12ShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTexture2D

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
        const SClearValue& InOptimalClearValue)
        : CRHITexture2D(InFormat, SizeX, SizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
        , CD3D12BaseTexture(InDevice)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
        , UnorderedAccessView(nullptr)
    {
    }

    virtual CRHIRenderTargetView*    GetRenderTargetView() const override { return RenderTargetView.Get(); }
    virtual CRHIDepthStencilView*    GetDepthStencilView() const override { return DepthStencilView.Get(); }
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

    FORCEINLINE void SetRenderTargetView(CD3D12RenderTargetView* InRenderTargetView)
    {
        RenderTargetView = InRenderTargetView;
    }

    FORCEINLINE void SetDepthStencilView(CD3D12DepthStencilView* InDepthStencilView)
    {
        DepthStencilView = InDepthStencilView;
    }

    FORCEINLINE void SetUnorderedAccessView(CD3D12UnorderedAccessView* InUnorderedAccessView)
    {
        UnorderedAccessView = InUnorderedAccessView;
    }

    FORCEINLINE CD3D12RenderTargetView* GetD3D12RenderTargetView() const
    {
        return RenderTargetView.Get();
    }

    FORCEINLINE void SetSize(uint32 InWidth, uint32 InHeight)
    {
        CRHITexture2D::SetSize(InWidth, InHeight);
    }

private:
    /** Default RenderTargetView created at creation */ 
    TSharedRef<CD3D12RenderTargetView> RenderTargetView;
    /** Default DepthStencilView created at creation */ 
    TSharedRef<CD3D12DepthStencilView> DepthStencilView;
    /** Default UnorderedAccessView created at creation */
    TSharedRef<CD3D12UnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTexture2DArray

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
        const SClearValue& InOptimalClearValue)
        : CRHITexture2DArray(InFormat, SizeX, SizeY, InNumMips, InNumSamples, SizeZ, InFlags, InOptimalClearValue)
        , CD3D12BaseTexture(InDevice)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTextureCube

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
        const SClearValue& InOptimalClearValue)
        : CRHITextureCube(InFormat, SizeX, InNumMips, InFlags, InOptimalClearValue)
        , CD3D12BaseTexture(InDevice)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTextureCubeArray

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
        const SClearValue& InOptimalClearValue)
        : CRHITextureCubeArray(InFormat, SizeX, InNumMips, SizeZ, InFlags, InOptimalClearValue)
        , CD3D12BaseTexture(InDevice)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BaseTexture3D

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
        const SClearValue& InOptimalClearValue)
        : CRHITexture3D(InFormat, SizeX, SizeY, SizeZ, InNumMips, InFlags, InOptimalClearValue)
        , CD3D12BaseTexture(InDevice)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12BaseTexture

template<typename BaseTextureType>
class TD3D12BaseTexture : public BaseTextureType
{
public:
    TD3D12BaseTexture(
        CD3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const SClearValue& InOptimalClearValue)
        : BaseTextureType(InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
    {
    }

    virtual void SetName(const String& InName) override
    {
        // Save the debug string for fast lookup
        CRHIResource::SetName(InName);

        // Set the native resource name
        CD3D12BaseTexture::Resource->SetName(InName);
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Texture types

using CD3D12Texture2D        = TD3D12BaseTexture<CD3D12BaseTexture2D>;
using CD3D12Texture2DArray   = TD3D12BaseTexture<CD3D12BaseTexture2DArray>;
using CD3D12TextureCube      = TD3D12BaseTexture<CD3D12BaseTextureCube>;
using CD3D12TextureCubeArray = TD3D12BaseTexture<CD3D12BaseTextureCubeArray>;
using CD3D12Texture3D        = TD3D12BaseTexture<CD3D12BaseTexture3D>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12TextureCast

inline CD3D12BaseTexture* D3D12TextureCast(CRHITexture* Texture)
{
    if (Texture)
    {
        if (Texture->AsTexture2D())
        {
            return static_cast<CD3D12Texture2D*>(Texture);
        }
        else if (Texture->AsTexture2DArray())
        {
            return static_cast<CD3D12Texture2DArray*>(Texture);
        }
        else if (Texture->AsTextureCube())
        {
            return static_cast<CD3D12TextureCube*>(Texture);
        }
        else if (Texture->AsTextureCubeArray())
        {
            return static_cast<CD3D12TextureCubeArray*>(Texture);
        }
        else if (Texture->AsTexture3D())
        {
            return static_cast<CD3D12Texture3D*>(Texture);
        }
    }

    return nullptr;
}

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif