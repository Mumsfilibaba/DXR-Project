#include "VulkanQueue.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFence.h"

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : FVulkanDeviceObject(InDevice)
    , Type(InType)
    , CommandQueue(VK_NULL_HANDLE)
{
}

FVulkanQueue::~FVulkanQueue()
{
}

bool FVulkanQueue::Initialize()
{
    TOptional<FVulkanQueueFamilyIndices> QueueIndices = GetDevice()->GetQueueIndicies();
    VULKAN_ERROR_COND(QueueIndices.HasValue(), "Queue Families is not initialized correctly");

    QueueFamilyIndex = GetDevice()->GetCommandQueueIndexFromType(Type);
    vkGetDeviceQueue(GetDevice()->GetVkDevice(), QueueFamilyIndex, 0, &CommandQueue);
    return true;
}

bool FVulkanQueue::ExecuteCommandBuffer(FVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, FVulkanFence* Fence)
{
    VkSubmitInfo SubmitInfo;
    FMemory::Memzero(&SubmitInfo);

    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = nullptr;

    if (!WaitSemaphores.IsEmpty())
    {
        VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "The size of WaitSemaphores and WaitStages must be the same");
        
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.Size();
        SubmitInfo.pWaitSemaphores    = WaitSemaphores.Data();
        SubmitInfo.pWaitDstStageMask  = WaitStages.Data();
    }
    else
    {
        SubmitInfo.waitSemaphoreCount = 0;
        SubmitInfo.pWaitSemaphores    = nullptr;
        SubmitInfo.pWaitDstStageMask  = nullptr;
    }

    if (!SignalSemaphores.IsEmpty())
    {
        SubmitInfo.signalSemaphoreCount = SignalSemaphores.Size();
        SubmitInfo.pSignalSemaphores    = SignalSemaphores.Data();
    }
    else
    {
        SubmitInfo.signalSemaphoreCount = 0;
        SubmitInfo.pSignalSemaphores    = nullptr;
    }

    TArray<VkCommandBuffer, TInlineArrayAllocator<VkCommandBuffer, 8>> CommandBuffersArray;
    CommandBuffersArray.Resize(NumCommandBuffers);

    if (!CommandBuffersArray.IsEmpty())
    {
        VULKAN_ERROR_COND(CommandBuffers != nullptr, "CommandBuffers cannot be nullptr");

        for (uint32 Index = 0; Index < NumCommandBuffers; ++Index)
        {
            VULKAN_ERROR_COND(CommandBuffers[Index] != nullptr, "CommandBuffer[%d] cannot be nullptr", Index);
            CommandBuffersArray[Index] = CommandBuffers[Index]->GetVkCommandBuffer();
        }

        SubmitInfo.commandBufferCount = CommandBuffersArray.Size();
        SubmitInfo.pCommandBuffers    = CommandBuffersArray.Data();
    }
    else
    {
        SubmitInfo.commandBufferCount = 0;
        SubmitInfo.pCommandBuffers    = nullptr;
    }

    VkFence SignalFence = Fence ? Fence->GetVkFence() : VK_NULL_HANDLE;

    VkResult Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, SignalFence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkQueueSubmit failed");
        return false;
    }

    WaitSemaphores.Clear();
    WaitStages.Clear();

    SignalSemaphores.Clear();
    return true;
}

void FVulkanQueue::AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage)
{
    WaitSemaphores.Add(Semaphore);
    WaitStages.Add(WaitStage);
    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "WaitSemaphores and WaitStages must be the same size");
}

void FVulkanQueue::AddSignalSemaphore(VkSemaphore Semaphore)
{
    SignalSemaphores.Add(Semaphore);
}

void FVulkanQueue::WaitForCompletion()
{
    vkQueueWaitIdle(CommandQueue);
}

bool FVulkanQueue::FlushWaitSemaphoresAndWait()
{
    VkSubmitInfo SubmitInfo;
    FMemory::Memzero(&SubmitInfo);

    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext                = nullptr;
    SubmitInfo.commandBufferCount   = 0;
    SubmitInfo.pCommandBuffers      = nullptr;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores    = nullptr;

    if (!WaitSemaphores.IsEmpty())
    {
        VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "The size of WaitSemaphores and WaitStages must be the same");
        
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.Size();
        SubmitInfo.pWaitSemaphores    = WaitSemaphores.Data();
        SubmitInfo.pWaitDstStageMask  = WaitStages.Data();
    }
    else
    {
        SubmitInfo.waitSemaphoreCount = 0;
        SubmitInfo.pWaitSemaphores    = nullptr;
        SubmitInfo.pWaitDstStageMask  = nullptr;
    }

    VkResult Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkQueueSubmit failed");
        return false;
    }

    WaitSemaphores.Clear();
    WaitStages.Clear();

    SignalSemaphores.Clear();

    WaitForCompletion();
    return true;
}
