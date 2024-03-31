#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanLoader.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"

class FVulkanFence : public FVulkanDeviceChild
{
public:
    FVulkanFence(const FVulkanFence&) = delete;
    FVulkanFence& operator=(const FVulkanFence&) = delete;

    FVulkanFence(FVulkanDevice* InDevice);
    ~FVulkanFence();

    bool Initialize(bool bSignaled);

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

    bool Wait(uint64 TimeOut = UINT64_MAX) const
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
    
    VkFence GetVkFence() const
    {
        return Fence;
    }
    
private:
    VkFence Fence;
};
