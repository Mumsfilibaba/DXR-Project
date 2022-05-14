#pragma once
#include "MetalViews.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class CMetalViewport;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture

class CMetalTexture : public CMetalObject
{
public:
     
    CMetalTexture(CMetalDeviceContext* InDeviceContext)
        : CMetalObject(InDeviceContext)
        , Viewport(nullptr)
        , ShaderResourceView(nullptr)
    { }
    
public:
    
    void SetMTLTexture(id<MTLTexture> InTexture) { Texture = InTexture; }
    
    void SetViewport(CMetalViewport* InViewport) { Viewport = InViewport; }
    
    id<MTLTexture> GetMTLTexture() const { return Texture; }
    
    CMetalViewport* GetViewport() const { return Viewport; }
    
    CMetalShaderResourceView* GetMetalShaderResourceView() const { return ShaderResourceView.Get(); }

private:
    id<MTLTexture> Texture;
    
    CMetalViewport* Viewport;
    
    TSharedRef<CMetalShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2D

class CMetalTexture2D : public CMetalTexture, public CRHITexture2D
{
public:

    CMetalTexture2D(CMetalDeviceContext* InDeviceContext, const CRHITexture2DInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , CRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new CMetalUnorderedAccessView(InDeviceContext, this))
    { }

public:
    
    CMetalUnorderedAccessView* GetMetalUnorderedAccessView() const { return UnorderedAccessView.Get(); }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }

    virtual CRHIUnorderedAccessView* GetUnorderedAccessView() const override { return GetMetalUnorderedAccessView(); }

private:
    TSharedRef<CMetalUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2DArray

class CMetalTexture2DArray : public CMetalTexture, public CRHITexture2DArray
{
public:

    CMetalTexture2DArray(CMetalDeviceContext* InDeviceContext, const CRHITexture2DArrayInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , CRHITexture2DArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCube

class CMetalTextureCube : public CMetalTexture, public CRHITextureCube
{
public:

    CMetalTextureCube(CMetalDeviceContext* InDeviceContext, const CRHITextureCubeInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , CRHITextureCube(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCubeArray

class CMetalTextureCubeArray : public CMetalTexture, public CRHITextureCubeArray
{
public:
    
    CMetalTextureCubeArray(CMetalDeviceContext* InDeviceContext, const CRHITextureCubeArrayInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , CRHITextureCubeArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture3D

class CMetalTexture3D : public CMetalTexture, public CRHITexture3D
{
public:
    
    CMetalTexture3D(CMetalDeviceContext* InDeviceContext, const CRHITexture3DInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , CRHITexture3D(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual CRHIDescriptorHandle GetBindlessSRVHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetMetalTexture

inline CMetalTexture* GetMetalTexture(CRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CMetalTexture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

#pragma clang diagnostic pop
