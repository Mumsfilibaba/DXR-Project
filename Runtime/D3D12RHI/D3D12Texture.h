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

typedef TSharedRef<class FD3D12Texture>          FD3D12TextureRef;
typedef TSharedRef<class FD3D12Texture2D>        FD3D12Texture2DRef;
typedef TSharedRef<class FD3D12Texture2DArray>   FD3D12Texture2DArrayRef;
typedef TSharedRef<class FD3D12TextureCube>      FD3D12TextureCubeRef;
typedef TSharedRef<class FD3D12TextureCubeArray> FD3D12TextureCubeArrayRef;
typedef TSharedRef<class FD3D12Texture3D>        FD3D12Texture3DRef;

typedef TSharedRef<FD3D12RenderTargetView>   FD3D12RenderTargetViewRef;
typedef TSharedRef<FD3D12DepthStencilView>   FD3D12DepthStencilViewRef;
typedef TSharedRef<FD3D12ShaderResourceView> FD3D12ShaderResourceViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Texture 

class FD3D12Texture : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:

    FD3D12Texture(FD3D12Device* InDevice);
    ~FD3D12Texture() = default;

public:

    FD3D12RenderTargetView* GetOrCreateRTV(const FRHIRenderTargetView& RTVInitializer);
    FD3D12DepthStencilView* GetOrCreateDSV(const FRHIDepthStencilView& DSVInitializer);

    void DestroyRTVs() { RenderTargetViews.Clear(); }
	void DestroyDSVs() { DepthStencilViews.Clear(); }

    void SetShaderResourceView(FD3D12ShaderResourceView* InShaderResourceView) 
    { 
        ShaderResourceView = InShaderResourceView; 
    }
    
    void SetResource(FD3D12Resource* InResource) 
    { 
        Resource = InResource; 
        RenderTargetViews.Clear();
        DepthStencilViews.Clear();
    }

    FD3D12Resource*           GetD3D12Resource()           const { return Resource.Get(); }
    FD3D12ShaderResourceView* GetD3D12ShaderResourceView() const { return ShaderResourceView.Get(); }

    DXGI_FORMAT               GetDXGIFormat() const { return Resource ? Resource->GetDesc().Format : DXGI_FORMAT_UNKNOWN; }

protected:
    FD3D12ResourceRef           Resource;
    FD3D12ShaderResourceViewRef ShaderResourceView;

    TArray<FD3D12RenderTargetViewRef> RenderTargetViews;
    TArray<FD3D12DepthStencilViewRef> DepthStencilViews;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Texture2D

class FD3D12Texture2D : public FRHITexture2D, public FD3D12Texture
{
public:
    
    explicit FD3D12Texture2D(FD3D12Device* InDevice, const FRHITexture2DInitializer& Initializer)
        : FRHITexture2D(Initializer)
        , FD3D12Texture(InDevice)
        , UnorderedAccessView(nullptr)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture2D Interface
    
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return GetD3D12ShaderResourceView(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }

public:

    void SetUnorderedAccessView(FD3D12UnorderedAccessView* InUnorderedAccessView) { UnorderedAccessView = InUnorderedAccessView; }

    void SetSize(uint16 InWidth, uint16 InHeight)
    {
        Width  = InWidth;
        Height = InHeight;
    }

private:
    FD3D12UnorderedAccessViewRef UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Texture2DArray

class FD3D12Texture2DArray : public FRHITexture2DArray, public FD3D12Texture
{
public:

    explicit FD3D12Texture2DArray(FD3D12Device* InDevice, const FRHITexture2DArrayInitializer& Initializer)
        : FRHITexture2DArray(Initializer)
        , FD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture2DArray Interface

    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12TextureCube

class FD3D12TextureCube : public FRHITextureCube, public FD3D12Texture
{
public:

    explicit FD3D12TextureCube(FD3D12Device* InDevice, const FRHITextureCubeInitializer& Initializer)
        : FRHITextureCube(Initializer)
        , FD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITextureCube Interface

    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12TextureCubeArray

class FD3D12TextureCubeArray : public FRHITextureCubeArray, public FD3D12Texture
{
public:
    
    explicit FD3D12TextureCubeArray(FD3D12Device* InDevice, const FRHITextureCubeArrayInitializer& Initializer)
        : FRHITextureCubeArray(Initializer)
        , FD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITextureCubeArray Interface

    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Texture3D

class FD3D12Texture3D final : public FRHITexture3D, public FD3D12Texture
{
public:

    explicit FD3D12Texture3D(FD3D12Device* InDevice, const FRHITexture3DInitializer& Initializer)
        : FRHITexture3D(Initializer)
        , FD3D12Texture(InDevice)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // IRefCounted Interface

    virtual int32 AddRef()      override final       { return FD3D12RefCounted::AddRef(); }
    virtual int32 Release()     override final       { return FD3D12RefCounted::Release(); }
    virtual int32 GetRefCount() const override final { return FD3D12RefCounted::GetRefCount(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture3D Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetD3D12Resource()); }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FD3D12Texture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetD3D12ShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
    {
        FD3D12Resource* D3D12Resource = GetD3D12Resource();
        if (D3D12Resource)
        {
            D3D12Resource->SetName(InName);
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetD3D12Texture

inline FD3D12Texture* GetD3D12Texture(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<FD3D12Texture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

inline FD3D12Resource* GetD3D12Resource(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<FD3D12Resource*>(Texture->GetRHIBaseResource()) : nullptr;
}

#ifdef COMPILER_MSVC
    #pragma warning(pop)
#endif