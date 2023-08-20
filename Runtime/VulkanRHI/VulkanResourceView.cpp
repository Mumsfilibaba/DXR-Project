#include "VulkanResourceView.h"
#include "VulkanDevice.h"

FVulkanImageView::FVulkanImageView(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , Image(VK_NULL_HANDLE)
    , ImageView(VK_NULL_HANDLE)
{
}

FVulkanImageView::~FVulkanImageView()
{
    DestroyView();
}

bool FVulkanImageView::CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat Format, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& InSubresourceRange)
{
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.pNext            = nullptr;
    ImageViewCreateInfo.flags            = Flags;
    ImageViewCreateInfo.format           = Format;
    ImageViewCreateInfo.image            = InImage;
    ImageViewCreateInfo.viewType         = ViewType;
    ImageViewCreateInfo.components.r     = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g     = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b     = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a     = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange = SubresourceRange = InSubresourceRange;

    VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageView);
    VULKAN_CHECK_RESULT(Result, "vkCreateImageView failed");

    Image = InImage;
    return true;
}

void FVulkanImageView::DestroyView()
{
    if (VULKAN_CHECK_HANDLE(ImageView))
    {
        vkDestroyImageView(GetDevice()->GetVkDevice(), ImageView, nullptr);
        
        Image     = VK_NULL_HANDLE;
        ImageView = VK_NULL_HANDLE;
    }
}

FVulkanShaderResourceView::FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIShaderResourceView(InResource)
    , FVulkanDeviceObject(InDevice)
{
}

FVulkanUnorderedAccessView::FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource)
    : FRHIUnorderedAccessView(InResource)
    , FVulkanDeviceObject(InDevice)
{
}