#include "VulkanCommandPool.h"

FVulkanCommandPool::FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : FVulkanDeviceChild(InDevice)
    , Type(InType)
    , CommandPool(VK_NULL_HANDLE)
{
}

FVulkanCommandPool::FVulkanCommandPool(FVulkanCommandPool&& Other)
    : FVulkanDeviceChild(Other.GetDevice())
    , Type(Other.Type)
    , CommandPool(Other.CommandPool)
{
    Other.CommandPool = VK_NULL_HANDLE;
    Other.Type        = EVulkanCommandQueueType::Unknown;
}

FVulkanCommandPool::~FVulkanCommandPool()
{
    if (VULKAN_CHECK_HANDLE(CommandPool))
    {
        vkDestroyCommandPool(GetDevice()->GetVkDevice(), CommandPool, nullptr);
        CommandPool = VK_NULL_HANDLE;
    }
}

bool FVulkanCommandPool::Initialize()
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