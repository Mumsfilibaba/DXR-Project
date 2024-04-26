#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanLoader.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanImageView>           FVulkanImageViewRef;
typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

class FVulkanImageView : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanImageView(FVulkanDevice* InDevice);
    ~FVulkanImageView();

    bool CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat InFormat, VkImageViewCreateFlags InFlags, const VkImageSubresourceRange& InSubresourceRange);
    void DestroyView();

    VkImage GetVkImage() const { return Image; }
    VkImageView GetVkImageView() const { return ImageView; }
    const VkImageSubresourceRange& GetSubresourceRange() const { return SubresourceRange; }
    VkFormat GetVkFormat() const { return Format; }
    
private:
    VkImageSubresourceRange SubresourceRange;
    VkImage                 Image;
    VkImageView             ImageView;
    VkFormat                Format;
    VkImageViewCreateFlags  Flags;
};

class FVulkanShaderResourceView : public FRHIShaderResourceView, public FVulkanDeviceChild
{
public:
    enum class EType
    {
        None = 0,
        Texture,
        Buffer,
    };

    FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool CreateTextureView(const FRHITextureSRVDesc& InDesc);
    bool CreateBufferView(const FRHIBufferSRVDesc& InDesc);

    bool HasImageView() const
    {
        return ImageView != nullptr;
    }

    EType GetType() const
    {
        return Type;
    }

    VkImage GetVkImage() const { return HasImageView() ? ImageView->GetVkImage() : VK_NULL_HANDLE; }
    VkImageView GetVkImageView() const { return HasImageView() ? ImageView->GetVkImageView() : VK_NULL_HANDLE; }
    VkBuffer GetVkBuffer() const { return HasImageView() ? VK_NULL_HANDLE : BufferInfo.buffer; }
    
    const VkImageSubresourceRange& GetImageSubresourceRange() const { return ImageSubresourceRange; }
    const VkDescriptorBufferInfo&  GetDescriptorBufferInfo()  const { return BufferInfo; }

private:
    // Easy way to keep track of the type of SRV
    EType Type;

    // Buffer ResourceView
    VkDescriptorBufferInfo       BufferInfo;

    // Texture ResourceView
    TSharedRef<FVulkanImageView> ImageView;
    VkImageSubresourceRange      ImageSubresourceRange;
};

class FVulkanUnorderedAccessView : public FRHIUnorderedAccessView, public FVulkanDeviceChild
{
public:
    enum class EType
    {
        None = 0,
        Texture,
        Buffer,
    };

    FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool CreateTextureView(const FRHITextureUAVDesc& InDesc);
    bool CreateBufferView(const FRHIBufferUAVDesc& InDesc);

    bool HasImageView() const
    {
        return ImageView != nullptr;
    }

    EType GetType() const
    {
        return Type;
    }

    VkBuffer GetVkBuffer() const { return HasImageView() ? VK_NULL_HANDLE : BufferInfo.buffer; }
    VkImage GetVkImage() const { return HasImageView() ? ImageView->GetVkImage() : VK_NULL_HANDLE; }
    VkImageView GetVkImageView() const { return HasImageView() ? ImageView->GetVkImageView() : VK_NULL_HANDLE; }
    
    const VkImageSubresourceRange& GetImageSubresourceRange() const { return ImageSubresourceRange; }
    const VkDescriptorBufferInfo&  GetDescriptorBufferInfo()  const { return BufferInfo; }

private:
    // Easy way to keep track of the type of SRV
    EType Type;

    // Buffer ResourceView
    VkDescriptorBufferInfo       BufferInfo;

    // Texture ResourceView
    TSharedRef<FVulkanImageView> ImageView;
    VkImageSubresourceRange      ImageSubresourceRange;
};
