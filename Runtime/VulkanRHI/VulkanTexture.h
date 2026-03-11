#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanResourceState.h"

class FVulkanSwapChain;
class FVulkanCommandContext;

typedef TSharedRef<FVulkanSwapChain>               FVulkanSwapChainRef;
typedef TSharedRef<class FVulkanTexture>           FVulkanTextureRef;
typedef TSharedRef<class FVulkanBackBufferTexture> FVulkanBackBufferTextureRef;

class FVulkanTexture : public FRHITexture, public FVulkanGenericResource
{
    friend class FVulkanBackBufferTexture;

public:
    static FVulkanTexture* Cast(FRHITexture* Texture);
    static FVulkanTexture* Cast(FVulkanCommandContext* InCommandContext, FRHITexture* Texture);

public:
    FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureInfo& InTextureInfo);
    virtual ~FVulkanTexture();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override { return reinterpret_cast<void*>(GetVkImage()); }
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    FVulkanResourceView* GetOrCreateImageView(const FVulkanHashableImageView& RenderTargetView);
    void DestroyImageViews();

    void SetVkImage(VkImage InImage);
    
    FVulkanImageLayoutState&       GetImageLayoutState()       { return TrackedState; }
    const FVulkanImageLayoutState& GetImageLayoutState() const { return TrackedState; }

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

    FString                       DebugName;
    VkImage                       Image;
    VkImageCreateInfo             CreateInfo;
    FVulkanImageLayoutState       TrackedState;
    FVulkanShaderResourceViewRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRef UnorderedAccessView;
    TArray<FVulkanResourceView*>  ImageViews;
    FImageViewMap                 ImageViewMap;
};

class FVulkanBackBufferTexture : public FVulkanTexture
{
public:
    FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanSwapChain* InSwapChain, const FRHITextureInfo& InTextureInfo);
    virtual ~FVulkanBackBufferTexture();

    void ResizeBackBuffer(int32 InWidth, int32 InHeight);
    FVulkanTexture* GetCurrentBackBufferTexture(FVulkanCommandContext* InCommandContext);
    
    FVulkanSwapChain* GetSwapChain() const
    {
        return SwapChain;
    }
    
    void SetSwapChain(FVulkanSwapChain* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FVulkanSwapChain* SwapChain;
};
