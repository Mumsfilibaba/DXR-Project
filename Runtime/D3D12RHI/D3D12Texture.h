#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

#ifdef COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture 

class CD3D12Texture : public CD3D12DeviceChild
{
public:

    CD3D12Texture(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Resource(nullptr)
        , ShaderResourceView(nullptr)
    { }

    void SetResource(CD3D12Resource* InResource) { Resource = InResource; }

    void SetShaderResourceView(CD3D12ShaderResourceView* InShaderResourceView) { ShaderResourceView = InShaderResourceView; }

    CD3D12Resource* GetD3D12Resource() const { return Resource.Get(); }

    CD3D12ShaderResourceView* GetD3D12ShaderResourceView() const { return ShaderResourceView.Get(); }

    DXGI_FORMAT GetDXGIFormat() const { return Resource ? Resource->GetDesc().Format : DXGI_FORMAT_UNKNOWN; }

protected:
    TSharedRef<CD3D12Resource>           Resource;
    TSharedRef<CD3D12ShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture2D

class CD3D12Texture2D : public CRHITexture2D, public CD3D12Texture
{
public:
    
    CD3D12Texture2D(CD3D12Device* InDevice, const CRHITexture2DInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , CD3D12Texture(InDevice)
        , RenderTargetView(nullptr)
        , DepthStencilView(nullptr)
        , UnorderedAccessView(nullptr)
    { }

public:

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

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture() override final
    {
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const override final 
    { 
        return CRHIDescriptorHandle();
    }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

private:
    TSharedRef<CD3D12RenderTargetView>    RenderTargetView;
    TSharedRef<CD3D12DepthStencilView>    DepthStencilView;
    TSharedRef<CD3D12UnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture2DArray

class CD3D12Texture2DArray : public CRHITexture2DArray, public CD3D12Texture
{
public:

    CD3D12Texture2DArray(CD3D12Device* InDevice, const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2DArray(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture() override final
    {
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const override final
    { 
        return CRHIDescriptorHandle();
    }

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
// CD3D12TextureCube

class CD3D12TextureCube : public CRHITextureCube, public CD3D12Texture
{
public:

    CD3D12TextureCube(CD3D12Device* InDevice, const CRHITextureCubeInitializer& Initializer)
        : CRHITextureCube(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture() override final
    {
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const override final 
    { 
        return CRHIDescriptorHandle(); 
    }

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
// CD3D12TextureCubeArray

class CD3D12TextureCubeArray : public CRHITextureCubeArray, public CD3D12Texture
{
public:
    
    CD3D12TextureCubeArray(CD3D12Device* InDevice, const CRHITextureCubeArrayInitializer& Initializer)
        : CRHITextureCubeArray(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture() override final
    {
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const override final 
    { 
        return CRHIDescriptorHandle();
    }

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
// CD3D12Texture3D

class CD3D12Texture3D final : public CRHITexture3D, public CD3D12Texture
{
public:

    CD3D12Texture3D(CD3D12Device* InDevice, const CRHITexture3DInitializer& Initializer)
        : CRHITexture3D(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        return D3D12Resource ? reinterpret_cast<void*>(D3D12Resource->GetResource()) : nullptr;
    }

    virtual void* GetRHIBaseTexture() override final
    {
        CD3D12Texture* D3D12Texture = static_cast<CD3D12Texture*>(this);
        return reinterpret_cast<void*>(D3D12Texture);
    }

    virtual CRHIShaderResourceView* GetDefaultShaderResourceView() const override final
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = GetD3D12ShaderResourceView();
        return static_cast<CRHIShaderResourceView*>(D3D12ShaderResourceView);
    }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const override final 
    { 
        return CRHIDescriptorHandle(); 
    }

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