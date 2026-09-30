#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalRelocatable.h"
#include "MetalRHI/MetalResource.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalSwapChainRHI;
class FMetalUploadBatch;

typedef TSharedRef<class FMetalTextureRHI> FMetalTextureRef;

class FMetalTextureRHI : public FRHITexture, public FMetalDeviceChild, public FMetalRelocatable
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
    bool CreateDefaultViews();
    bool ResizeSwapChainTexture(const FRHITextureDesc& InTextureDesc);

    id<MTLTexture> GetMTLTexture() const;

    FORCEINLINE bool IsHeapPlaced() const
    {
        return ResourceStorage.IsPlacedResource();
    }

    FORCEINLINE const FMetalResourceStorage& GetResourceStorage() const
    {
        return ResourceStorage;
    }

    FORCEINLINE FMetalResidencyEntry* GetResidencyEntry() const
    {
        return ResourceStorage.GetResidencyEntry();
    }

    void SetSwapChain(FMetalSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

protected:

    // FMetalRelocatable Interface
    virtual FMetalResourceStorage& GetRelocatableStorage() override final;
    virtual void OnStorageSwapped() override final;

private:
    bool UploadInitialData(FMetalUploadBatch& UploadBatch, const IRHITextureData* InInitialData);

    id<MTLTexture>                           Texture;
    FMetalResourceStorage                    ResourceStorage;
    FMetalSwapChainRHI*                      SwapChain;
    TSharedRef<FMetalShaderResourceViewRHI>  ShaderResourceView;
    TSharedRef<FMetalUnorderedAccessViewRHI> UnorderedAccessView;
    TSharedRef<FMetalRenderTargetViewRHI>    RenderTargetView;
    TSharedRef<FMetalDepthStencilViewRHI>    DepthStencilView;
};

FORCEINLINE FMetalTextureRHI* GetMetalTexture(FRHITexture* Texture)
{
    return Texture ? static_cast<FMetalTextureRHI*>(Texture) : nullptr;
}

namespace MetalRHI
{
    void CreatePlacementInitPasses(id<MTLTexture> Texture, TArray<MTLRenderPassDescriptor*>& OutPasses);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
