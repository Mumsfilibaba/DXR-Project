#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanCommandBuffer::FVulkanCommandBuffer(FVulkanDevice* InDevice, FVulkanCommandPool* InOwnerPool)
    : FVulkanDeviceChild(InDevice)
    , OwnerPool(InOwnerPool)
    , CommandBuffer()
    , Level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    , NumCommands(0)
    , bIsRecording(false)
{
    CHECK(OwnerPool != nullptr);
}

FVulkanCommandBuffer::~FVulkanCommandBuffer()
{
    CommandBuffer.FreeCommandBuffer(GetDevice()->GetVkDevice(), OwnerPool->GetVkCommandPool());
    CommandBuffer = FCommandBuffer();
}

bool FVulkanCommandBuffer::Initialize(VkCommandBufferLevel InLevel)
{
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.pNext              = nullptr;
    CommandBufferAllocateInfo.commandPool        = OwnerPool->GetVkCommandPool();
    CommandBufferAllocateInfo.level              = Level = InLevel;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    VkResult Result = CommandBuffer.AllocateCommandBuffer(GetDevice()->GetVkDevice(), &CommandBufferAllocateInfo);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate CommandBuffer");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanCommandBuffer::Reset()
{
    VkResult Result = CommandBuffer.ResetCommandBuffer(0);
	if (VULKAN_FAILED(Result))
	{
		VULKAN_ERROR_CRITICAL("Failed to reset CommandBuffer");
		return false;
	}
	else
	{
		return true;
	}
}

bool FVulkanCommandBuffer::Begin(VkCommandBufferUsageFlags Flags)
{
    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = Flags;

    VkResult Result = CommandBuffer.BeginCommandBuffer(&BeginInfo);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkBeginCommandBuffer Failed");
        return false;
    }

    bIsRecording = true;
    return true;
}

bool FVulkanCommandBuffer::End()
{
    VkResult Result = CommandBuffer.EndCommandBuffer();
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkEndCommandBuffer Failed");
        return false;
    }

    NumCommands = 0;
    bIsRecording = false;
    return true;
}

FVulkanCommandPool::FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : FVulkanDeviceChild(InDevice)
    , CommandPool(VK_NULL_HANDLE)
    , Type(InType)
    , Flags(0)
    , CommandBuffers()
    , AvailableCommandBuffers()
{
}

FVulkanCommandPool::~FVulkanCommandPool()
{
    DestroyBuffers();

    if (VULKAN_CHECK_HANDLE(CommandPool))
    {
        vkDestroyCommandPool(GetDevice()->GetVkDevice(), CommandPool, nullptr);
        CommandPool = VK_NULL_HANDLE;
    }
}

bool FVulkanCommandPool::Initialize(VkCommandPoolCreateFlags InFlags)
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo = {};
    CommandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolCreateInfo.pNext            = nullptr;
    CommandPoolCreateInfo.flags            = InFlags;
    CommandPoolCreateInfo.queueFamilyIndex = GetDevice()->GetQueueIndexFromType(Type);

    VkResult Result = vkCreateCommandPool(GetDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create CommandPool");
        return false;
    }
    else
    {
        Flags = InFlags;
        return true;
    }
}

bool FVulkanCommandPool::Reset(VkCommandPoolResetFlags InFlags)
{
	VkResult Result = vkResetCommandPool(GetDevice()->GetVkDevice(), CommandPool, InFlags);
	if (VULKAN_FAILED(Result))
	{
		VULKAN_ERROR_CRITICAL("vkResetCommandPool Failed");
		return false;
	}

    if (!AllowCommandBufferReset())
    {
		for (FVulkanCommandBuffer* CommandBuffer : RecycledCommandBuffers)
		{
            AvailableCommandBuffers.Enqueue(CommandBuffer);
		}

        RecycledCommandBuffers.Clear();
    }

	return true;
}

void FVulkanCommandPool::DestroyBuffers()
{
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        delete CommandBuffer;
    }

    AvailableCommandBuffers.Clear();
    CommandBuffers.Clear();
    RecycledCommandBuffers.Clear();
}

FVulkanCommandBuffer* FVulkanCommandPool::GetOrCreateBuffer()
{
    FVulkanCommandBuffer* CommandBuffer = nullptr;
    if (AvailableCommandBuffers.IsEmpty())
    {
        FVulkanCommandBuffer* NewCommandBuffer = new FVulkanCommandBuffer(GetDevice(), this);
        if (!NewCommandBuffer->Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
        {
            DEBUG_BREAK();
            delete NewCommandBuffer;
            return nullptr;
        }

        CommandBuffer = NewCommandBuffer;
        CommandBuffers.Add(NewCommandBuffer);
    }
    else
    {
		AvailableCommandBuffers.Dequeue(CommandBuffer);
        CHECK(CommandBuffer != nullptr);
        
        if (AllowCommandBufferReset())
        {
            CommandBuffer->Reset();
        }
    }

    return CommandBuffer;
}

void FVulkanCommandPool::RecycleBuffer(FVulkanCommandBuffer* InCommandBuffer)
{
    if (InCommandBuffer)
    {
        if (AllowCommandBufferReset())
        {
            AvailableCommandBuffers.Enqueue(InCommandBuffer);
        }
        else
        {
            RecycledCommandBuffers.Add(InCommandBuffer);
        }
    }
}
