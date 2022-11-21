#pragma once
#include "MetalViews.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalViewport;

typedef TSharedRef<class FMetalTexture>          FMetalTextureRef;
typedef TSharedRef<class FMetalTexture2D>        FMetalTexture2DRef;
typedef TSharedRef<class FMetalTexture2DArray>   FMetalTexture2DArrayRef;
typedef TSharedRef<class FMetalTextureCube>      FMetalTextureCubeRef;
typedef TSharedRef<class FMetalTextureCubeArray> FMetalTextureCubeArrayRef;
typedef TSharedRef<class FMetalTexture3D>        FMetalTexture3DRef;

class FMetalTexture 
    : public FMetalObject
{
public:
    explicit FMetalTexture(FMetalDeviceContext* InDeviceContext);
    ~FMetalTexture();

    id<MTLTexture> GetMTLTexture() const;
    
    void SetDrawableTexture(id<MTLTexture> InTexture) 
    { 
        NSSafeRelease(Texture);
        Texture = [InTexture retain];
    }

    void SetViewport(FMetalViewport* InViewport)
    {
        Viewport = InViewport;
    }

    FMetalShaderResourceView* GetMetalShaderResourceView() const
    {
        return ShaderResourceView.Get();
    }

protected:
    id<MTLTexture>  Texture;
    FMetalViewport* Viewport;

    TSharedRef<FMetalShaderResourceView> ShaderResourceView;
};

class FMetalTexture2D 
    : public FMetalTexture
    , public FRHITexture
{
public:
    FMetalTexture2D(FMetalDeviceContext* InDeviceContext, const FRHITexture2DInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture(Initializer)
        , UnorderedAccessView(new FMetalUnorderedAccessView(InDeviceContext, this))
    { }

    FMetalUnorderedAccessView* GetMetalUnorderedAccessView() const { return UnorderedAccessView.Get(); }

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


class FMetalTexture2DArray 
    : public FMetalTexture
    , public FRHITexture
{
public:
    FMetalTexture2DArray(FMetalDeviceContext* InDeviceContext, const FRHITexture2DArrayInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture(Initializer)
    { }

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


class FMetalTextureCube 
    : public FMetalTexture
    , public FRHITextureCube
{
public:
    FMetalTextureCube(FMetalDeviceContext* InDeviceContext, const FRHITextureCubeInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITextureCube(Initializer)
    { }

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


class FMetalTextureCubeArray 
    : public FMetalTexture
    , public FRHITextureCubeArray
{
public:
    FMetalTextureCubeArray(FMetalDeviceContext* InDeviceContext, const FRHITextureCubeArrayInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITextureCubeArray(Initializer)
    { }

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


class FMetalTexture3D 
    : public FMetalTexture
    , public FRHITexture3D
{
public:
    FMetalTexture3D(FMetalDeviceContext* InDeviceContext, const FRHITexture3DInitializer& Initializer)
        : FMetalTexture(InDeviceContext)
        , FRHITexture3D(Initializer)
    { }

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


FORCEINLINE FMetalTexture* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<FMetalTexture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

#pragma clang diagnostic pop
