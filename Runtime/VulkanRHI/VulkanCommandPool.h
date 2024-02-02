#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceChild.h"
#include "VulkanLoader.h"

class FVulkanCommandPool : public FVulkanDeviceChild
{
public:
    FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    FVulkanCommandPool(FVulkanCommandPool&& Other);
    ~FVulkanCommandPool();

    bool Initialize();

    bool Reset(VkCommandPoolResetFlags Flags = 0)
    {
        VkResult Result = vkResetCommandPool(GetDevice()->GetVkDevice(), CommandPool, Flags);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("vkResetCommandPool Failed");
            return false;
        }

        return true;
    }

    VkCommandPool GetVkCommandPool() const
    {
        return CommandPool;
    }
    
private:
    VkCommandPool           CommandPool;
    EVulkanCommandQueueType Type;
};
