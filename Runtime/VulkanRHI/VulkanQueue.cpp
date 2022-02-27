#include "VulkanQueue.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFence.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanQueue

CVulkanQueueRef CVulkanQueue::CreateQueue(CVulkanDevice* InDevice, EVulkanCommandQueueType InType)
{
    CVulkanQueueRef NewQueue = dbg_new CVulkanQueue(InDevice, InType);
    if (NewQueue && NewQueue->Initialize())
    {
        return NewQueue;
    }

    return nullptr;
}

CVulkanQueue::CVulkanQueue(CVulkanDevice* InDevice, EVulkanCommandQueueType InType)
    : CVulkanDeviceObject(InDevice)
    , Type(InType)
    , CommandQueue(VK_NULL_HANDLE)
{
}

CVulkanQueue::~CVulkanQueue()
{
}

bool CVulkanQueue::Initialize()
{
    TOptional<SVulkanQueueFamilyIndices> QueueIndices = GetDevice()->GetQueueIndicies();
    VULKAN_ERROR(QueueIndices.HasValue(), "Queue Families is not initialized correctly");

    QueueFamilyIndex = GetDevice()->GetCommandQueueIndexFromType(Type);
    vkGetDeviceQueue(GetDevice()->GetVkDevice(), QueueFamilyIndex, 0, &CommandQueue);

    return true;
}

bool CVulkanQueue::ExecuteCommandBuffer(CVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, CVulkanFence* Fence)
{
    VkSubmitInfo SubmitInfo;
    CMemory::Memzero(&SubmitInfo);

    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = nullptr;

    VULKAN_ERROR(WaitSemaphores.Size() == WaitStages.Size(), "The size of WaitSemaphores and WaitStages must be the same");

    if (!WaitSemaphores.IsEmpty())
    {
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.Size();
        SubmitInfo.pWaitSemaphores    = WaitSemaphores.Data();
        SubmitInfo.pWaitDstStageMask  = WaitStages.Data();
    }
    else
    {
        SubmitInfo.waitSemaphoreCount = 0;
        SubmitInfo.pWaitSemaphores    = nullptr;
        SubmitInfo.pWaitDstStageMask = nullptr;
    }

    if (!SignalSemaphores.IsEmpty())
    {
        SubmitInfo.signalSemaphoreCount = SignalSemaphores.Size();
        SubmitInfo.pSignalSemaphores = SignalSemaphores.Data();
    }
    else
    {
        SubmitInfo.signalSemaphoreCount = 0;
        SubmitInfo.pSignalSemaphores    = nullptr;
    }

    TInlineArray<VkCommandBuffer, 8> CommandBuffersArray;
    CommandBuffersArray.Resize(NumCommandBuffers);

    if (!CommandBuffersArray.IsEmpty())
    {
        VULKAN_ERROR(CommandBuffers != nullptr, "CommandBuffers cannot be nullptr");

        for (uint32 Index = 0; Index < NumCommandBuffers; ++Index)
        {
            VULKAN_ERROR(CommandBuffers[Index] != nullptr, String("CommandBuffer[") + ToString(Index) + "] cannot be nullptr");
            CommandBuffersArray[Index] = CommandBuffers[Index]->GetVkCommandBuffer();
        }

        SubmitInfo.commandBufferCount = CommandBuffersArray.Size();
        SubmitInfo.pCommandBuffers    = CommandBuffersArray.Data();
    }
    else
    {
        SubmitInfo.commandBufferCount = 0;
        SubmitInfo.pCommandBuffers = nullptr;
    }

    VkFence SignalFence = Fence ? Fence->GetVkFence() : VK_NULL_HANDLE;

    VkResult Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, SignalFence);
    VULKAN_CHECK_RESULT(Result, "vkQueueSubmit failed");

    WaitSemaphores.Clear();
    WaitStages.Clear();

    SignalSemaphores.Clear();

    return true;
}

void CVulkanQueue::AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage)
{
    WaitSemaphores.Push(Semaphore);
    WaitStages.Push(WaitStage);

    VULKAN_ERROR(WaitSemaphores.Size() == WaitStages.Size(), "WaitSemaphores and WaitStages must be the same size");
}

void CVulkanQueue::AddSignalSemaphore(VkSemaphore Semaphore)
{
    SignalSemaphores.Push(Semaphore);
}
