#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandPool

class CVulkanCommandPool : public CVulkanDeviceObject
{
public:

    inline CVulkanCommandPool(CVulkanDevice* InDevice, EVulkanCommandQueueType InType)
        : CVulkanDeviceObject(InDevice)
        , Type(InType)
        , CommandPool(VK_NULL_HANDLE)
    {
    }

    inline ~CVulkanCommandPool()
    {
        if (VULKAN_CHECK_HANDLE(CommandPool))
        {
            vkDestroyCommandPool(GetDevice()->GetVkDevice(), CommandPool, nullptr);
            CommandPool = VK_NULL_HANDLE;
        }
    }

    inline bool Initialize()
    {
        VkCommandPoolCreateInfo CommandPoolCreateInfo;
        CMemory::Memzero(&CommandPoolCreateInfo);

        CommandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CommandPoolCreateInfo.pNext            = nullptr;
        CommandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CommandPoolCreateInfo.queueFamilyIndex = GetDevice()->GetCommandQueueIndexFromType(Type);

        VkResult Result = vkCreateCommandPool(GetDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool);
        VULKAN_CHECK_RESULT(Result, "Failed to create CommandPool");

        return true;
    }

    FORCEINLINE bool Reset(VkCommandPoolResetFlags Flags = 0)
    {
		VULKAN_CHECK_RESULT(vkResetCommandPool(GetDevice()->GetVkDevice(), CommandPool, Flags), "vkResetCommandPool Failed");
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
