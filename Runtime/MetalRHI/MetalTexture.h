#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalDeviceChild.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalSwapChain;

typedef TSharedRef<class FMetalTexture> FMetalTextureRef;

class FMetalTexture : public FRHITexture, public FMetalDeviceChild
{
public:
    FMetalTexture(FMetalDeviceContext* InDeviceContext, const FRHITextureDesc& InTextureDesc);
    virtual ~FMetalTexture();

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetMTLTexture()); }
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final { return RenderTargetView.Get(); }
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final { return DepthStencilView.Get(); }

    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    id<MTLTexture> GetMTLTexture() const;

    void SetDrawableTexture(id<MTLTexture> InTexture) 
    {
        [Texture release];
        Texture = [InTexture retain];
    }

    void SetSwapChain(FMetalSwapChain* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FMetalShaderResourceView* GetMetalShaderResourceView() const
    {
        return ShaderResourceView.Get();
    }

protected:
    id<MTLTexture>  Texture;
    FMetalSwapChain* SwapChain;
    TSharedRef<FMetalShaderResourceView> ShaderResourceView;
    TSharedRef<FMetalRenderTargetView>   RenderTargetView;
    TSharedRef<FMetalDepthStencilView>   DepthStencilView;
};

FORCEINLINE FMetalTexture* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? static_cast<FMetalTexture*>(Texture) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
