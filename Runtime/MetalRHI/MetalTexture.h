#pragma once
#include "MetalViews.h"
#include "MetalObject.h"
#include "MetalRefCounted.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalViewport;

typedef TSharedRef<class FMetalTexture> FMetalTextureRef;

class FMetalTexture : public FRHITexture, public FMetalObject, public FMetalRefCounted
{
public:
    FMetalTexture(FMetalDeviceContext* InDeviceContext, const FRHITextureDesc& InDesc);
    virtual ~FMetalTexture();

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    virtual int32 AddRef() override final { return FMetalRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FMetalRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FMetalRefCounted::GetRefCount(); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<FMetalTexture*>(this)); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }

    virtual FRHIShaderResourceView* GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) override final;
    
    virtual FString GetDebugName() const override final;

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

FORCEINLINE FMetalTexture* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? reinterpret_cast<FMetalTexture*>(Texture->GetRHIBaseTexture()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
