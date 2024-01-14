#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

class FVulkanCommandPool : public FVulkanDeviceObject
{
public:
    FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
        : FVulkanDeviceObject(InDevice)
        , Type(InType)
        , CommandPool(VK_NULL_HANDLE)
    {
    }

    FVulkanCommandPool(FVulkanCommandPool&& Other)
        : FVulkanDeviceObject(Other.GetDevice())
        , Type(Other.Type)
        , CommandPool(Other.CommandPool)
    {
        Other.CommandPool = VK_NULL_HANDLE;
        Other.Type        = EVulkanCommandQueueType::Unknown;
    }

    ~FVulkanCommandPool()
    {
        if (VULKAN_CHECK_HANDLE(CommandPool))
        {
            vkDestroyCommandPool(GetDevice()->GetVkDevice(), CommandPool, nullptr);
            CommandPool = VK_NULL_HANDLE;
        }
    }

    bool Initialize()
    {
        VkCommandPoolCreateInfo CommandPoolCreateInfo;
        FMemory::Memzero(&CommandPoolCreateInfo);

        CommandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CommandPoolCreateInfo.pNext            = nullptr;
        CommandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CommandPoolCreateInfo.queueFamilyIndex = GetDevice()->GetCommandQueueIndexFromType(Type);

        VkResult Result = vkCreateCommandPool(GetDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to create CommandPool");
            return false;
        }

        return true;
    }

    FORCEINLINE bool Reset(VkCommandPoolResetFlags Flags = 0)
    {
        VkResult Result = vkResetCommandPool(GetDevice()->GetVkDevice(), CommandPool, Flags);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("vkResetCommandPool Failed");
            return false;
        }

        return true;
    }

    FORCEINLINE VkCommandPool GetVkCommandPool() const
    {
        return CommandPool;
    }
    
private:
    EVulkanCommandQueueType Type;
    VkCommandPool           CommandPool;
};
