#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

#include "RHI/RHIResources.h"

#ifdef COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CD3D12Texture>      D3D12TextureRef;

typedef TSharedRef<CD3D12RenderTargetView>   D3D12RenderTargetViewRef;
typedef TSharedRef<CD3D12DepthStencilView>   D3D12DepthStencilViewRef;
typedef TSharedRef<CD3D12ShaderResourceView> D3D12ShaderResourceViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture 

class CD3D12Texture : public CD3D12DeviceChild
{
public:

    CD3D12Texture(FD3D12Device* InDevice);
    ~CD3D12Texture() = default;

    CD3D12RenderTargetView* GetOrCreateRTV(const CRHIRenderTargetView& RTVInitializer);
    CD3D12DepthStencilView* GetOrCreateDSV(const CRHIDepthStencilView& DSVInitializer);

    void DestroyRTVs()
    {
        RenderTargetViews.Clear();
    }

	void DestroyDSVs()
	{
        DepthStencilViews.Clear();
	}

    void SetShaderResourceView(CD3D12ShaderResourceView* InShaderResourceView) 
    { 
        ShaderResourceView = InShaderResourceView; 
    }
    
    void SetResource(CD3D12Resource* InResource) 
    { 
        Resource = InResource; 
        RenderTargetViews.Clear();
        DepthStencilViews.Clear();
    }

    CD3D12Resource*           GetD3D12Resource() const { return Resource.Get(); }
    CD3D12ShaderResourceView* GetD3D12ShaderResourceView() const { return ShaderResourceView.Get(); }

    DXGI_FORMAT GetDXGIFormat() const { return Resource ? Resource->GetDesc().Format : DXGI_FORMAT_UNKNOWN; }

protected:
    D3D12ResourceRef           Resource;
    D3D12ShaderResourceViewRef ShaderResourceView;

    TArray<D3D12RenderTargetViewRef> RenderTargetViews;
    TArray<D3D12DepthStencilViewRef> DepthStencilViews;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture2D

class CD3D12Texture2D : public CRHITexture2D, public CD3D12Texture
{
public:
    
    explicit CD3D12Texture2D(FD3D12Device* InDevice, const CRHITexture2DInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , CD3D12Texture(InDevice)
        , UnorderedAccessView(nullptr)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture2D Interface
    
    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CD3D12Texture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

    virtual void SetName(const String& InName) override final
    {
        CD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

public:

    void SetUnorderedAccessView(CD3D12UnorderedAccessView* InUnorderedAccessView) { UnorderedAccessView = InUnorderedAccessView; }

    void SetSize(uint16 InWidth, uint16 InHeight)
    {
        Width  = InWidth;
        Height = InHeight;
    }

private:
    TSharedRef<CD3D12UnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Texture2DArray

class CD3D12Texture2DArray : public CRHITexture2DArray, public CD3D12Texture
{
public:

    explicit CD3D12Texture2DArray(FD3D12Device* InDevice, const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2DArray(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture2DArray Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CD3D12Texture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

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

    explicit CD3D12TextureCube(FD3D12Device* InDevice, const CRHITextureCubeInitializer& Initializer)
        : CRHITextureCube(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITextureCube Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CD3D12Texture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

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
    
    explicit CD3D12TextureCubeArray(FD3D12Device* InDevice, const CRHITextureCubeArrayInitializer& Initializer)
        : CRHITextureCubeArray(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITextureCubeArray Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CD3D12Texture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

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

    explicit CD3D12Texture3D(FD3D12Device* InDevice, const CRHITexture3DInitializer& Initializer)
        : CRHITexture3D(Initializer)
        , CD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture3D Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CD3D12Texture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

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
// GetD3D12Texture

inline CD3D12Texture* GetD3D12Texture(CRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CD3D12Texture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

inline CD3D12Resource* GetD3D12Resource(CRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CD3D12Resource*>(Texture->GetRHIBaseResource()) : nullptr;
}

#ifdef COMPILER_MSVC
    #pragma warning(pop)
#endif