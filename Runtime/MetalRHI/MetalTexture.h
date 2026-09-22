#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalResource.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalSwapChainRHI;

typedef TSharedRef<class FMetalTextureRHI> FMetalTextureRef;

class FMetalTextureRHI : public FRHITexture, public FMetalDeviceChild
{
public:
    FMetalTextureRHI(FMetalDevice* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FMetalTextureRHI();

    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final;
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final;
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Initialize(ERHIResourceState InInitialAccess, const IRHITextureData* InInitialData);
    
    id<MTLTexture> GetMTLTexture() const;

    FORCEINLINE bool IsHeapPlaced() const
    {
        return ResourceStorage.IsPlacedResource();
    }

    FORCEINLINE FMetalQueue* GetLastUsedQueue() const
    {
        return LastUsedQueue;
    }

    FORCEINLINE uint64 GetLastUsedValue() const
    {
        return LastUsedValue;
    }

    void StampLastUse(FMetalQueue* InQueue, uint64 InValue);

    void SetSwapChain(FMetalSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FMetalShaderResourceViewRHI* GetMetalShaderResourceView() const
    {
        return ShaderResourceView.Get();
    }

    FMetalUnorderedAccessViewRHI* GetMetalUnorderedAccessView() const
    {
        return UnorderedAccessView.Get();
    }

    bool CreateDefaultViews();

private:
    bool UploadInitialData(const IRHITextureData* InInitialData);

    id<MTLTexture>                           Texture;
    FMetalResourceStorage                    ResourceStorage;
    FMetalSwapChainRHI*                      SwapChain;
    TSharedRef<FMetalShaderResourceViewRHI>  ShaderResourceView;
    TSharedRef<FMetalUnorderedAccessViewRHI> UnorderedAccessView;
    TSharedRef<FMetalRenderTargetViewRHI>    RenderTargetView;
    TSharedRef<FMetalDepthStencilViewRHI>    DepthStencilView;
    FMetalQueue*                             LastUsedQueue;
    uint64                                   LastUsedValue;
};

FORCEINLINE FMetalTextureRHI* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? static_cast<FMetalTextureRHI*>(Texture) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
