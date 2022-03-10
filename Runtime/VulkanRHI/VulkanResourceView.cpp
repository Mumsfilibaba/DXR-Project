#include "VulkanResourceView.h"
#include "VulkanDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanImageView

CVulkanImageView::CVulkanImageView(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
    , Image(VK_NULL_HANDLE)
    , ImageView(VK_NULL_HANDLE)
{
}

CVulkanImageView::~CVulkanImageView()
{
    DestroyView();
}

bool CVulkanImageView::CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat Format, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& SubresoureRange)
{
    VkImageViewCreateInfo ImageViewCreateInfo;
    CMemory::Memzero(&ImageViewCreateInfo);

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
    ImageViewCreateInfo.subresourceRange = SubresoureRange;

    VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageView);
    VULKAN_CHECK_RESULT(Result, "vkCreateImageView failed");

    Image = InImage;
    return true;
}

void CVulkanImageView::DestroyView()
{
    if (VULKAN_CHECK_HANDLE(ImageView))
    {
        vkDestroyImageView(GetDevice()->GetVkDevice(), ImageView, nullptr);
        
        Image     = VK_NULL_HANDLE;
        ImageView = VK_NULL_HANDLE;
    }
}
