#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12Resource.h"
#include "D3D12Views.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

constexpr uint32 TEXTURE_CUBE_FACE_COUNT = 6;

class D3D12BaseTexture : public D3D12DeviceChild
{
public:
    D3D12BaseTexture(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , Resource(nullptr)
    {
    }

    void SetResource(D3D12Resource* InResource) { Resource = InResource; }
    void SetShaderResourceView(D3D12ShaderResourceView* InShaderResourceView) { ShaderResourceView = InShaderResourceView; }

    DXGI_FORMAT GetNativeFormat() const { return Resource->GetDesc().Format; }

    D3D12Resource* GetResource() { return Resource.Get(); }

protected:
    TRef<D3D12Resource>           Resource;
    TRef<D3D12ShaderResourceView> ShaderResourceView;
};

class D3D12BaseTexture2D : public Texture2D, public D3D12BaseTexture
{
public:
    D3D12BaseTexture2D(
        D3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const ClearValue& InOptimalClearValue)
        : Texture2D(InFormat, SizeX, SizeY, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
        , UnorderedAccessView(nullptr)
    {
    }

    virtual RenderTargetView*    GetRenderTargetView() const override    { return RenderTargetView.Get(); }
    virtual DepthStencilView*    GetDepthStencilView() const override    { return DepthStencilView.Get(); }
    virtual UnorderedAccessView* GetUnorderedAccessView() const override { return UnorderedAccessView.Get(); }

    void SetRenderTargetView(D3D12RenderTargetView* InRenderTargetView)          { RenderTargetView = InRenderTargetView; }
    void SetDepthStencilView(D3D12DepthStencilView* InDepthStencilView)          { DepthStencilView = InDepthStencilView; }
    void SetUnorderedAccessView(D3D12UnorderedAccessView* InUnorderedAccessView) { UnorderedAccessView = InUnorderedAccessView; }

    D3D12RenderTargetView* GetD3D12RenderTargetView() const { return RenderTargetView.Get(); }

    void SetSize(uint32 InWidth, uint32 InHeight)
    {
        Texture2D::SetSize(InWidth, InHeight);
    }

private:
    TRef<D3D12RenderTargetView> RenderTargetView;
    TRef<D3D12DepthStencilView> DepthStencilView;
    TRef<D3D12UnorderedAccessView> UnorderedAccessView;
};

class D3D12BaseTexture2DArray : public Texture2DArray, public D3D12BaseTexture
{
public:
    D3D12BaseTexture2DArray(
        D3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
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
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TextureCube(InFormat, SizeX, InNumMips, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
    }
};

class D3D12BaseTextureCubeArray : public TextureCubeArray, public D3D12BaseTexture
{
public:
    D3D12BaseTextureCubeArray(
        D3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TextureCubeArray(InFormat, SizeX, InNumMips, SizeZ, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
    }
};

class D3D12BaseTexture3D : public Texture3D, public D3D12BaseTexture
{
public:
    D3D12BaseTexture3D(
        D3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const ClearValue& InOptimalClearValue)
        : Texture3D(InFormat, SizeX, SizeY, SizeZ, InNumMips, InFlags, InOptimalClearValue)
        , D3D12BaseTexture(InDevice)
    {
    }
};

template<typename TBaseTexture>
class TD3D12BaseTexture : public TBaseTexture
{
public:
    TD3D12BaseTexture(
        D3D12Device* InDevice,
        EFormat InFormat,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 InNumMips,
        uint32 InNumSamples,
        uint32 InFlags,
        const ClearValue& InOptimalClearValue)
        : TBaseTexture(InDevice, InFormat, SizeX, SizeY, SizeZ, InNumMips, InNumSamples, InFlags, InOptimalClearValue)
    {
    }

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);
        D3D12BaseTexture::Resource->SetName(InName);
    }

    virtual void* GetNativeResource() const override
    {
        return reinterpret_cast<void*>(D3D12BaseTexture::Resource->GetResource());
    }

    virtual class ShaderResourceView* GetShaderResourceView() const
    {
        return D3D12BaseTexture::ShaderResourceView.Get();
    }

    virtual bool IsValid() const override
    {
        return D3D12BaseTexture::Resource->GetResource() != nullptr;
    }
};

using D3D12Texture2D        = TD3D12BaseTexture<D3D12BaseTexture2D>;
using D3D12Texture2DArray   = TD3D12BaseTexture<D3D12BaseTexture2DArray>;
using D3D12TextureCube      = TD3D12BaseTexture<D3D12BaseTextureCube>;
using D3D12TextureCubeArray = TD3D12BaseTexture<D3D12BaseTextureCubeArray>;
using D3D12Texture3D        = TD3D12BaseTexture<D3D12BaseTexture3D>;

inline D3D12BaseTexture* D3D12TextureCast(Texture* Texture)
{
    Assert(Texture != nullptr);

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

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif