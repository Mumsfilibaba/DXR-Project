#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanFence

class CVulkanFence : public CVulkanDeviceObject
{
public:

    inline CVulkanFence(CVulkanDevice* InDevice)
        : CVulkanDeviceObject(InDevice)
        , Fence(VK_NULL_HANDLE)
    { }

    inline CVulkanFence(CVulkanFence&& Other)
        : CVulkanDeviceObject(Other.GetDevice())
        , Fence(Other.Fence)
    {
        Other.Fence = VK_NULL_HANDLE;
    }

    inline ~CVulkanFence()
    {
        if (VULKAN_CHECK_HANDLE(Fence))
        {
            vkDestroyFence(GetDevice()->GetVkDevice(), Fence, nullptr);
            Fence = VK_NULL_HANDLE;
        }
    }

    inline bool Initialize()
    {
        VkFenceCreateInfo FenceCreateInfo;
        CMemory::Memzero(&FenceCreateInfo);

        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.pNext = nullptr;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult Result = vkCreateFence(GetDevice()->GetVkDevice(), &FenceCreateInfo, nullptr, &Fence);
        VULKAN_CHECK_RESULT(Result, "Failed to create Fence");

        return true;
    }

    FORCEINLINE bool Wait(uint64 TimeOut) const
    {
        VULKAN_CHECK_RESULT(vkWaitForFences(GetDevice()->GetVkDevice(), 1, &Fence, VK_TRUE, TimeOut), "vkWaitForFences Failed");
        return true;
    }

    FORCEINLINE bool Reset()
    {
        VULKAN_CHECK_RESULT(vkResetFences(GetDevice()->GetVkDevice(), 1, &Fence), "vkResetFences Failed");
        return true;
    }

    FORCEINLINE VkFence GetVkFence() const
    {
        return Fence;
    }
    
private:
    VkFence Fence;
};
