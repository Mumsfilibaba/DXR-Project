#include "VulkanCommandBuffer.h"
#include "VulkanFunctions.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandBuffer

CVulkanCommandBuffer::CVulkanCommandBuffer(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
    , CommandBuffer(VK_NULL_HANDLE)
{
}

CVulkanCommandBuffer::~CVulkanCommandBuffer()
{
}

bool CVulkanCommandBuffer::Initialize(CVulkanCommandPool* InCommandPool, VkCommandBufferLevel InLevel)
{
    VULKAN_ERROR(InCommandPool != nullptr, "CommandPool cannot be nullptr");

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
    CMemory::Memzero(&CommandBufferAllocateInfo);

	CommandBufferAllocateInfo.sType		         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CommandBufferAllocateInfo.pNext		         = nullptr;
	CommandBufferAllocateInfo.commandPool        = InCommandPool->GetVkCommandPool();
	CommandBufferAllocateInfo.level		         = Level = InLevel;
	CommandBufferAllocateInfo.commandBufferCount = 1;

    VkResult Result = vkAllocateCommandBuffers(GetDevice()->GetVkDevice(), &CommandBufferAllocateInfo, &CommandBuffer);
    VULKAN_CHECK_RESULT(Result, "Failed to allocate CommandBuffer");

    CommandPool = InCommandPool;
    return true;
}
