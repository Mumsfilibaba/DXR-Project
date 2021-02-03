#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

class D3D12BaseTexture : public D3D12DeviceChild
{
public:
    D3D12BaseTexture(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , DxResource(InDevice)
    {
    }

    void SetResource(const D3D12Resource& InResource)
    {
        DxResource = InResource;
    }

    void SetShaderResourceView(D3D12ShaderResourceView* InShaderResourceView)
    {
        ShaderResourceView = MakeSharedRef<D3D12ShaderResourceView>(InShaderResourceView);
    }

    DXGI_FORMAT GetNativeFormat() const { return DxResource.GetDesc().Format; }

    D3D12Resource* GetResource() { return &DxResource; }
    const D3D12Resource* GetResource() const { return &DxResource; }

protected:
    TSharedRef<D3D12ShaderResourceView> ShaderResourceView;
    D3D12Resource DxResource;
};

class D3D12BaseTexture2D : public Texture2D, public D3D12BaseTexture
{
public:
    D3D12BaseTexture2D(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : Texture2D(InFormat, SizeX, SizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
        VALIDATE(SizeZ == 1);
    }

    void SetRenderTargetView(D3D12RenderTargetView* InRenderTargetView)
    {
        RenderTargetView = MakeSharedRef<D3D12RenderTargetView>(InRenderTargetView);
    }

    void SetDepthStencilView(D3D12DepthStencilView* InDepthStencilView)
    {
        DepthStencilView = MakeSharedRef<D3D12DepthStencilView>(InDepthStencilView);
    }

    void SetUnorderedAccessView(D3D12UnorderedAccessView* InUnorderedAccessView)
    {
        UnorderedAccessView = MakeSharedRef<D3D12UnorderedAccessView>(InUnorderedAccessView);
    }

private:
    TSharedRef<D3D12RenderTargetView> RenderTargetView;
    TSharedRef<D3D12DepthStencilView> DepthStencilView;
    TSharedRef<D3D12UnorderedAccessView> UnorderedAccessView;
};

class D3D12BaseTexture2DArray : public Texture2DArray, public D3D12BaseTexture
{
public:
    D3D12BaseTexture2DArray(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : Texture2DArray(InFormat, SizeX, SizeY, InNumMips, InNumSamples, SizeZ, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
    }
};

class D3D12BaseTextureCube : public TextureCube, public D3D12BaseTexture
{
public:
    D3D12BaseTextureCube(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TextureCube(InFormat, SizeX, InNumMips, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
        VALIDATE(SizeX == SizeY);
        VALIDATE(SizeZ == TEXTURE_CUBE_FACE_COUNT);
        VALIDATE(InNumSamples == 1);
    }
};

class D3D12BaseTextureCubeArray : public TextureCubeArray, public D3D12BaseTexture
{
public:
    D3D12BaseTextureCubeArray(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TextureCubeArray(InFormat, SizeX, InNumMips, SizeZ, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
        VALIDATE(SizeX == SizeY);
        VALIDATE(SizeZ % TEXTURE_CUBE_FACE_COUNT == 0);
        VALIDATE(InNumSamples == 1);
    }
};

class D3D12BaseTexture3D : public Texture3D, public D3D12BaseTexture
{
public:
    D3D12BaseTexture3D(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : Texture3D(InFormat, SizeX, SizeY, SizeZ, InNumMips, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
        VALIDATE(InNumSamples == 1);
    }
};

template<typename TBaseTexture>
class TD3D12BaseTexture : public TBaseTexture
{
public:
    TD3D12BaseTexture(
        D3D12Device* InDevice,
        EFormat InFormat,
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 InNumMips,
        UInt32 InNumSamples,
        UInt32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TBaseTexture(InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
    {
    }

    ~TD3D12BaseTexture() = default;

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);
        DxResource.SetName(InName);
    }

    virtual void* GetNativeResource() const override
    {
        return reinterpret_cast<void*>(DxResource.GetResource());
    }

    virtual class ShaderResourceView* GetShaderResourceView() const
    {
        return ShaderResourceView.Get();
    }

    virtual Bool IsValid() const override
    {
        return DxResource.GetResource() != nullptr;
    }
};

using D3D12Texture2D        = TD3D12BaseTexture<D3D12BaseTexture2D>;
using D3D12Texture2DArray   = TD3D12BaseTexture<D3D12BaseTexture2DArray>;
using D3D12TextureCube      = TD3D12BaseTexture<D3D12BaseTextureCube>;
using D3D12TextureCubeArray = TD3D12BaseTexture<D3D12BaseTextureCubeArray>;
using D3D12Texture3D        = TD3D12BaseTexture<D3D12BaseTexture3D>;

inline D3D12BaseTexture* D3D12TextureCast(Texture* Texture)
{
    VALIDATE(Texture != nullptr);

    if (Texture->AsTexture2D() != nullptr)
    {
        return static_cast<D3D12Texture2D*>(Texture);
    }
    else if (Texture->AsTexture2DArray() != nullptr)
    {
        return static_cast<D3D12Texture2DArray*>(Texture);
    }
    else if (Texture->AsTextureCube() != nullptr)
    {
        return static_cast<D3D12TextureCube*>(Texture);
    }
    else if (Texture->AsTextureCubeArray() != nullptr)
    {
        return static_cast<D3D12TextureCubeArray*>(Texture);
    }
    else if (Texture->AsTexture3D() != nullptr)
    {
        return static_cast<D3D12Texture3D*>(Texture);
    }
    else
    {
        return nullptr;
    }
}