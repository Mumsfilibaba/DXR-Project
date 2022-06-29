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
        , Texture(nil)
        , Viewport(nullptr)
        , ShaderResourceView(nullptr)
    { }
    
    ~CMetalTexture()
    {
        NSSafeRelease(Texture);
    }
    
public:
    
    void SetMTLTexture(id<MTLTexture> InTexture) { Texture = [InTexture retain]; }
    
    void SetViewport(CMetalViewport* InViewport) { Viewport = InViewport; }
    
    id<MTLTexture> GetMTLTexture() const { return Texture; }
    
    CMetalViewport* GetViewport() const { return Viewport; }
    
    CMetalShaderResourceView* GetMetalShaderResourceView() const { return ShaderResourceView.Get(); }

private:
    id<MTLTexture>  Texture;
    CMetalViewport* Viewport;
    
    TSharedRef<CMetalShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2D

class CMetalTexture2D : public CMetalTexture, public FRHITexture2D
{
public:

    CMetalTexture2D(CMetalDeviceContext* InDeviceContext, const FRHITexture2DInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , FRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new CMetalUnorderedAccessView(InDeviceContext, this))
    { }

public:
    
    CMetalUnorderedAccessView* GetMetalUnorderedAccessView() const { return UnorderedAccessView.Get(); }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override { return GetMetalUnorderedAccessView(); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLTexture> TextureHandle = GetMTLTexture();
            if (TextureHandle)
            {
                TextureHandle.label = InName.GetNSString();
            }
        }
    }

private:
    TSharedRef<CMetalUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture2DArray

class CMetalTexture2DArray : public CMetalTexture, public FRHITexture2DArray
{
public:

    CMetalTexture2DArray(CMetalDeviceContext* InDeviceContext, const FRHITexture2DArrayInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , FRHITexture2DArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLTexture> TextureHandle = GetMTLTexture();
            if (TextureHandle)
            {
                TextureHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCube

class CMetalTextureCube : public CMetalTexture, public FRHITextureCube
{
public:

    CMetalTextureCube(CMetalDeviceContext* InDeviceContext, const FRHITextureCubeInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , FRHITextureCube(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLTexture> TextureHandle = GetMTLTexture();
            if (TextureHandle)
            {
                TextureHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTextureCubeArray

class CMetalTextureCubeArray : public CMetalTexture, public FRHITextureCubeArray
{
public:
    
    CMetalTextureCubeArray(CMetalDeviceContext* InDeviceContext, const FRHITextureCubeArrayInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , FRHITextureCubeArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLTexture> TextureHandle = GetMTLTexture();
            if (TextureHandle)
            {
                TextureHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTexture3D

class CMetalTexture3D : public CMetalTexture, public FRHITexture3D
{
public:
    
    CMetalTexture3D(CMetalDeviceContext* InDeviceContext, const FRHITexture3DInitializer& Initializer)
        : CMetalTexture(InDeviceContext)
        , FRHITexture3D(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<CMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLTexture> TextureHandle = GetMTLTexture();
            if (TextureHandle)
            {
                TextureHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetMetalTexture

inline CMetalTexture* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<CMetalTexture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

#pragma clang diagnostic pop
