#include "VulkanCommandBuffer.h"
#include "VulkanLoader.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"

FVulkanCommandBuffer::FVulkanCommandBuffer(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : FVulkanDeviceObject(InDevice)
    , CommandPool(InDevice, InType)
    , Fence(InDevice)
    , Level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    , CommandBuffer(VK_NULL_HANDLE)
    , bIsRecording(false)
{
}

FVulkanCommandBuffer::FVulkanCommandBuffer(FVulkanCommandBuffer&& Other)
    : FVulkanDeviceObject(GetDevice())
    , CommandPool(Move(Other.CommandPool))
    , Fence(Move(Other.Fence))
    , Level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    , CommandBuffer(Other.CommandBuffer)
    , bIsRecording(false)
{
    Other.CommandBuffer = VK_NULL_HANDLE;
}

FVulkanCommandBuffer::~FVulkanCommandBuffer()
{
    VULKAN_ERROR_COND(Fence.Wait(UINT64_MAX), "Failed to wait for fence");
}

bool FVulkanCommandBuffer::Initialize(VkCommandBufferLevel InLevel)
{
    if (!CommandPool.Initialize())
    {
        return false;
    }

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
    FMemory::Memzero(&CommandBufferAllocateInfo);

    CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.pNext              = nullptr;
    CommandBufferAllocateInfo.commandPool        = CommandPool.GetVkCommandPool();
    CommandBufferAllocateInfo.level              = Level = InLevel;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    VkResult Result = vkAllocateCommandBuffers(GetDevice()->GetVkDevice(), &CommandBufferAllocateInfo, &CommandBuffer);
    VULKAN_CHECK_RESULT(Result, "Failed to allocate CommandBuffer");

    if (!Fence.Initialize())
    {
        return false;
    }

    return true;
}
