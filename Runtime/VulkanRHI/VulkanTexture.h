#pragma once
#include "VulkanResourceViews.h"
#include "VulkanMemory.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"

class FVulkanViewport;

typedef TSharedRef<FVulkanViewport>                FVulkanViewportRef;
typedef TSharedRef<class FVulkanTexture>           FVulkanTextureRef;
typedef TSharedRef<class FVulkanBackBufferTexture> FVulkanBackBufferTextureRef;

class FVulkanTexture : public FRHITexture, public FVulkanDeviceChild
{
public:
    FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureInfo& InTextureInfo);
    virtual ~FVulkanTexture();

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    virtual void* GetRHIBaseTexture() override { return reinterpret_cast<void*>(static_cast<FVulkanTexture*>(this)); }
    virtual void* GetRHIBaseResource() const override { return reinterpret_cast<void*>(GetVkImage()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    FVulkanResourceView* GetOrCreateImageView(const FVulkanHashableImageView& RenderTargetView);
    void DestroyImageViews();

    void SetVkImage(VkImage InImage);
    
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
        return Format;
    }
    
    // TODO: Solve in a cleaner way and remove this function
    void Resize(uint32 InWidth, uint32 InHeight)
    {
        Info.Extent.x = InWidth;
        Info.Extent.y = InHeight;
    }

protected:
    VkImage                       Image;
    VkImageType                   ImageType;
    VkFormat                      Format;
    FVulkanMemoryAllocation       MemoryAllocation;
    VkImageCreateInfo             CreateInfo;
    FVulkanShaderResourceViewRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRef UnorderedAccessView;
    TArray<FVulkanResourceView*>  ImageViews;
    FString                       DebugName;
    TMap<FVulkanHashableImageView, FVulkanResourceView*> ImageViewMap;
};

class FVulkanBackBufferTexture : public FVulkanTexture
{
public:
    FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanViewport* InViewport, const FRHITextureInfo& InTextureInfo);
    virtual ~FVulkanBackBufferTexture();

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(GetCurrentBackBufferTexture()); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkImage()); }

    void ResizeBackBuffer(int32 InWidth, int32 InHeight);
    FVulkanTexture* GetCurrentBackBufferTexture();
    
    FVulkanViewport* GetViewport() const
    {
        return Viewport;
    }
    
    void SetViewport(FVulkanViewport* InViewport)
    {
        Viewport = InViewport;
    }

private:
    FVulkanViewport* Viewport;
};


FORCEINLINE FVulkanTexture* GetVulkanTexture(FRHITexture* Texture)
{
    if (Texture)
    {
        FVulkanTexture* VulkanTexture = nullptr;
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            FVulkanBackBufferTexture* BackBuffer = static_cast<FVulkanBackBufferTexture*>(Texture);
            VulkanTexture = BackBuffer->GetCurrentBackBufferTexture();
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTexture*>(Texture);
        }

        return VulkanTexture;
    }

    return nullptr;
}

FORCEINLINE VkImage GetVkImage(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Texture);
    return VulkanTexture ? VulkanTexture->GetVkImage() : VK_NULL_HANDLE;
}


struct FVulkanTextureHelper
{
    static uint32 CalculateTextureRowPitch(VkFormat Format, uint32 Width);
    static uint32 CalculateTextureNumRows(VkFormat Format, uint32 Height);
    static uint64 CalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height);
};
