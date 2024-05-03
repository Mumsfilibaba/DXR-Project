#pragma once
#include "VulkanLoader.h"
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

class FVulkanResourceView : public FVulkanDeviceChild
{
public:
    enum class EType
    {
        None = 0,
        ImageView,
        Buffer,
    };

    FVulkanResourceView(FVulkanDevice* InDevice);
    virtual ~FVulkanResourceView();

    bool CreateImageView(const VkImageViewCreateInfo& CreateInfo);
    bool CreateBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange);
    void SetDebugName(const FString& InName);

    EType GetType() const
    {
        return Type;
    }

    VkImage GetVkImage() const { return ImageInfo.Image; }
    VkImageView GetVkImageView() const { return ImageInfo.ImageView; }
    VkFormat GetVkFormat() const { return ImageInfo.Format; }
    const VkImageSubresourceRange& GetVkImageSubresourceRange() const { return ImageInfo.SubresourceRange; }

    VkBuffer GetVkBuffer() const { return BufferInfo.Buffer; }
    VkDeviceSize GetOffset() const { return BufferInfo.Offset; }
    VkDeviceSize GetRange() const { return BufferInfo.Range; }

protected:
    EType Type;
    union
    {
        struct
        {
            VkBuffer     Buffer;
            VkDeviceSize Offset;
            VkDeviceSize Range;
        } BufferInfo;

        struct
        {
            VkImage                 Image;
            VkImageView             ImageView;
            VkImageViewType         ImageViewType;
            VkFormat                Format;
            VkImageViewCreateFlags  Flags;
            VkImageSubresourceRange SubresourceRange;
        } ImageInfo;
    };
};

class FVulkanShaderResourceView : public FRHIShaderResourceView, public FVulkanResourceView
{
public:
    FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool InitializeTextureSRV(const FRHITextureSRVDesc& InDesc);
    bool InitializeBufferSRV(const FRHIBufferSRVDesc& InDesc);
};

class FVulkanUnorderedAccessView : public FRHIUnorderedAccessView, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool InitializeTextureUAV(const FRHITextureUAVDesc& InDesc);
    bool InitializeBufferUAV(const FRHIBufferUAVDesc& InDesc);
};
