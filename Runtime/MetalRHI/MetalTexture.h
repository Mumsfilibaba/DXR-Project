#pragma once
#include "MetalViews.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalViewport;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalTexture

class FMetalTexture : public FMetalObject
{
public:
     
    FMetalTexture(FMetalDeviceContext* InDeviceContext)
        : FMetalObject(InDeviceContext)
        , Texture(nil)
        , Viewport(nullptr)
        , ShaderResourceView(nullptr)
    { }
    
    ~FMetalTexture()
    {
        NSSafeRelease(Texture);
    }
    
public:
    
    void SetMTLTexture(id<MTLTexture> InTexture) { Texture = [InTexture retain]; }
    
    void SetViewport(FMetalViewport* InViewport) { Viewport = InViewport; }
    
    id<MTLTexture> GetMTLTexture() const { return Texture; }
    
    FMetalShaderResourceView* GetMetalShaderResourceView() const { return ShaderResourceView.Get(); }
    FMetalViewport*           GetViewport()                const { return Viewport; }

private:
    id<MTLTexture>  Texture;
    FMetalViewport* Viewport;
    
    TSharedRef<FMetalShaderResourceView> ShaderResourceView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalTexture2D

class FMetalTexture2D : public FMetalTexture, public FRHITexture2D
{
public:

    FMetalTexture2D(FMetalDeviceContext* InDeviceContext, const FRHITexture2DInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture2D(Initializer)
        , UnorderedAccessView(dbg_new FMetalUnorderedAccessView(InDeviceContext, this))
    { }

public:
    
    FMetalUnorderedAccessView* GetMetalUnorderedAccessView() const { return UnorderedAccessView.Get(); }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual void* GetRHIBaseTexture()  override final { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override { return GetMetalUnorderedAccessView(); }
    
    virtual void SetName(const FString& InName) override final
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
    TSharedRef<FMetalUnorderedAccessView> UnorderedAccessView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalTexture2DArray

class FMetalTexture2DArray : public FMetalTexture, public FRHITexture2DArray
{
public:

    FMetalTexture2DArray(FMetalDeviceContext* InDeviceContext, const FRHITexture2DArrayInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture2DArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const FString& InName) override final
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
// FMetalTextureCube

class FMetalTextureCube : public FMetalTexture, public FRHITextureCube
{
public:

    FMetalTextureCube(FMetalDeviceContext* InDeviceContext, const FRHITextureCubeInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITextureCube(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const FString& InName) override final
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
// FMetalTextureCubeArray

class FMetalTextureCubeArray : public FMetalTexture, public FRHITextureCubeArray
{
public:
    
    FMetalTextureCubeArray(FMetalDeviceContext* InDeviceContext, const FRHITextureCubeArrayInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITextureCubeArray(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const FString& InName) override final
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
// FMetalTexture3D

class FMetalTexture3D : public FMetalTexture, public FRHITexture3D
{
public:
    
    FMetalTexture3D(FMetalDeviceContext* InDeviceContext, const FRHITexture3DInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture3D(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual void* GetRHIBaseTexture()  override final       { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return GetMetalShaderResourceView(); }
    virtual FRHIDescriptorHandle    GetBindlessSRVHandle()  const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetName(const FString& InName) override final
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

inline FMetalTexture* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<FMetalTexture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

#pragma clang diagnostic pop
