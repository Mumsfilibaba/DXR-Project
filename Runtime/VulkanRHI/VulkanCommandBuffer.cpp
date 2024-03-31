#include "VulkanCommandBuffer.h"
#include "VulkanLoader.h"
#include "VulkanDevice.h"

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
    CommandBuffer = FCommandBuffer();
}

bool FVulkanCommandBuffer::Initialize(VkCommandBufferLevel InLevel)
{
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
    FMemory::Memzero(&CommandBufferAllocateInfo);

    CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.pNext              = nullptr;
    CommandBufferAllocateInfo.commandPool        = OwnerPool->GetVkCommandPool();
    CommandBufferAllocateInfo.level              = Level = InLevel;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    VkResult Result = CommandBuffer.AllocateCommandBuffer(GetDevice()->GetVkDevice(), &CommandBufferAllocateInfo);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to allocate CommandBuffer");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanCommandBuffer::Begin(VkCommandBufferUsageFlags Flags)
{
    VkCommandBufferBeginInfo BeginInfo;
    FMemory::Memzero(&BeginInfo);

    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = Flags;

    VkResult Result = CommandBuffer.BeginCommandBuffer(&BeginInfo);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkBeginCommandBuffer Failed");
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
        VULKAN_ERROR("vkEndCommandBuffer Failed");
        return false;
    }

    NumCommands  = 0;
    bIsRecording = false;
    return true;
}

FVulkanCommandPool::FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : FVulkanDeviceChild(InDevice)
    , CommandPool(VK_NULL_HANDLE)
    , Type(InType)
{
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
    CommandPoolCreateInfo.queueFamilyIndex = GetDevice()->GetQueueIndexFromType(Type);

    VkResult Result = vkCreateCommandPool(GetDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create CommandPool");
        return false;
    }
    else
    {
        return true;
    }
}

FVulkanCommandBuffer* FVulkanCommandPool::CreateBuffer()
{
    FVulkanCommandBuffer* CommandBuffer = nullptr;
    if (CommandBuffers.IsEmpty())
    {
        FVulkanCommandBuffer* NewCommandBuffer = new FVulkanCommandBuffer(GetDevice(), this);
        if (NewCommandBuffer->Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
        {
            CommandBuffer = NewCommandBuffer;
        }
    }
    else
    {
        CommandBuffer = CommandBuffers.LastElement();
        CommandBuffers.Pop();
    }
    
    CHECK(CommandBuffer != nullptr);
    return CommandBuffer;
}

void FVulkanCommandPool::RecycleBuffer(FVulkanCommandBuffer* InCommandBuffer)
{
    if (InCommandBuffer)
    {
        CommandBuffers.Add(InCommandBuffer);
    }
}
