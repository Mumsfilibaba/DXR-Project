#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

class FVulkanFence : public FVulkanDeviceObject
{
public:
    FVulkanFence(FVulkanDevice* InDevice)
        : FVulkanDeviceObject(InDevice)
        , Fence(VK_NULL_HANDLE)
    {
    }

    FVulkanFence(FVulkanFence&& Other)
        : FVulkanDeviceObject(Other.GetDevice())
        , Fence(Other.Fence)
    {
        Other.Fence = VK_NULL_HANDLE;
    }

    ~FVulkanFence()
    {
        if (VULKAN_CHECK_HANDLE(Fence))
        {
            vkDestroyFence(GetDevice()->GetVkDevice(), Fence, nullptr);
            Fence = VK_NULL_HANDLE;
        }
    }

    bool Initialize()
    {
        VkFenceCreateInfo FenceCreateInfo;
        FMemory::Memzero(&FenceCreateInfo);

        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.pNext = nullptr;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult Result = vkCreateFence(GetDevice()->GetVkDevice(), &FenceCreateInfo, nullptr, &Fence);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to create Fence");
            return false;
        }

        return true;
    }

    bool IsSignaled() const 
    {
        VkResult Result = vkGetFenceStatus(GetDevice()->GetVkDevice(), Fence);
        if (Result == VK_ERROR_DEVICE_LOST)
        {
            VULKAN_ERROR("Device Lost");
            return false;
        }

        return Result == VK_SUCCESS;
    }

    bool Wait(uint64 TimeOut) const
    {
        VkResult Result = vkWaitForFences(GetDevice()->GetVkDevice(), 1, &Fence, VK_TRUE, TimeOut);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("vkWaitForFences Failed");
            return false;
        }

        return true;
    }

    bool Reset()
    {
        VkResult Result = vkResetFences(GetDevice()->GetVkDevice(), 1, &Fence);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("vkResetFences Failed");
            return false;
        }

        return true;
    }

    FORCEINLINE VkFence GetVkFence() const
    {
        return Fence;
    }
    
private:
    VkFence Fence;
};
