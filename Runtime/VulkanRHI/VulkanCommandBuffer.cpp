#include "VulkanCommandBuffer.h"
#include "VulkanLoader.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandBuffer

CVulkanCommandBuffer::CVulkanCommandBuffer(CVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : CVulkanDeviceObject(InDevice)
    , Fence(InDevice)
    , CommandPool(InDevice, InType)
    , CommandBuffer(VK_NULL_HANDLE)
{
}

CVulkanCommandBuffer::CVulkanCommandBuffer(CVulkanCommandBuffer&& Other)
    : CVulkanDeviceObject(GetDevice())
    , Fence(Move(Other.Fence))
    , CommandPool(Move(Other.CommandPool))
    , CommandBuffer(Other.CommandBuffer)
{
    Other.CommandBuffer = VK_NULL_HANDLE;
}

CVulkanCommandBuffer::~CVulkanCommandBuffer()
{
    VULKAN_ERROR(Fence.Wait(UINT64_MAX), "Failed to wait for fence");
}

bool CVulkanCommandBuffer::Initialize(VkCommandBufferLevel InLevel)
{
    if (!CommandPool.Initialize())
    {
        return false;
    }

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
    CMemory::Memzero(&CommandBufferAllocateInfo);

    CommandBufferAllocateInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.pNext                 = nullptr;
    CommandBufferAllocateInfo.commandPool        = CommandPool.GetVkCommandPool();
    CommandBufferAllocateInfo.level                 = Level = InLevel;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    VkResult Result = vkAllocateCommandBuffers(GetDevice()->GetVkDevice(), &CommandBufferAllocateInfo, &CommandBuffer);
    VULKAN_CHECK_RESULT(Result, "Failed to allocate CommandBuffer");

    if (!Fence.Initialize())
    {
        return false;
    }

    return true;
}
