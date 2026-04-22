#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanResourceState.h"

class FVulkanSwapChainRHI;
class FVulkanCommandContext;

typedef TSharedRef<FVulkanSwapChainRHI>               FVulkanSwapChainRHIRef;
typedef TSharedRef<class FVulkanTextureRHI>           FVulkanTextureRHIRef;
typedef TSharedRef<class FVulkanBackBufferTexture> FVulkanBackBufferTextureRef;

class FVulkanTextureRHI : public FRHITexture, public FVulkanResource
{
    friend class FVulkanBackBufferTexture;

public:
    FVulkanTextureRHI(FVulkanDevice* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanTextureRHI();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override { return reinterpret_cast<void*>(GetVkImage()); }
    
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    FVulkanResourceView* GetOrCreateImageView(const FVulkanHashableImageView& RenderTargetView);
    void DestroyImageViews();

    void SetVkImage(VkImage InImage);
    
    FVulkanImageLayoutState&       GetImageLayoutState()       { return ImageLayoutState; }
    const FVulkanImageLayoutState& GetImageLayoutState() const { return ImageLayoutState; }

    VkImage GetVkImage() const
    {
        return Image;
    }
    
    const VkImageCreateInfo& GetVkImageCreateInfo() const
    {
        return CreateInfo;
    }

    VkFormat GetVkFormat() const
    {
        return CreateInfo.format;
    }
    
protected:
    using FImageViewMap = TMap<FVulkanHashableImageView, FVulkanResourceView*>;

    FString                          DebugName;
    VkImage                          Image;
    VkImageCreateInfo                CreateInfo;
    FVulkanImageLayoutState          ImageLayoutState;
    FVulkanShaderResourceViewRHIRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRHIRef UnorderedAccessView;
    TArray<FVulkanResourceView*>     ImageViews;
    FImageViewMap                    ImageViewMap;
};

class FVulkanBackBufferTexture : public FVulkanTextureRHI
{
public:
    FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanBackBufferTexture();

    void ResizeBackBuffer(int32 InWidth, int32 InHeight);
    FVulkanTextureRHI* GetCurrentBackBufferTexture(FVulkanCommandContext* InCommandContext);
    
    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }
    
    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FVulkanSwapChainRHI* SwapChain;
};
