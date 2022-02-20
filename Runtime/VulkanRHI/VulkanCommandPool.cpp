#include "VulkanCommandPool.h"
#include "VulkanFunctions.h"
#include "VulkanDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandPool
	
CVulkanCommandPool::CVulkanCommandPool(CVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : CVulkanDeviceObject(InDevice)
	, Type(InType)
    , CommandPool(VK_NULL_HANDLE)
{
}

CVulkanCommandPool::~CVulkanCommandPool()
{
    if (VULKAN_CHECK_HANDLE(CommandPool))
    {
        vkDestroyCommandPool(GetDevice()->GetVkDevice(), CommandPool, nullptr);
    }
}

bool CVulkanCommandPool::Initialize()
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
