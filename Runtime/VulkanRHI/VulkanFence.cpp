#include "VulkanFence.h"

FVulkanFence::FVulkanFence(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Fence(VK_NULL_HANDLE)
{
}

FVulkanFence::FVulkanFence(FVulkanFence&& Other)
    : FVulkanDeviceChild(Other.GetDevice())
    , Fence(Other.Fence)
{
    Other.Fence  = VK_NULL_HANDLE;
}

FVulkanFence::~FVulkanFence()
{
    if (VULKAN_CHECK_HANDLE(Fence))
    {
        vkDestroyFence(GetDevice()->GetVkDevice(), Fence, nullptr);
        Fence = VK_NULL_HANDLE;
    }
}

bool FVulkanFence::Initialize(bool bSignaled)
{
    VkFenceCreateInfo FenceCreateInfo;
    FMemory::Memzero(&FenceCreateInfo);

    FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.pNext = nullptr;
    FenceCreateInfo.flags = bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult Result = vkCreateFence(GetDevice()->GetVkDevice(), &FenceCreateInfo, nullptr, &Fence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Fence");
        return false;
    }
    else
    {
        return true;
    }
}
