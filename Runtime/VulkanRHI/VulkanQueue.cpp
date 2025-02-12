#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanFence.h"

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InQueueType)
    : FVulkanDeviceChild(InDevice)
    , Queue(VK_NULL_HANDLE)
    , QueueFamilyIndex()
    , QueueType(InQueueType)
    , WaitSemaphores()
    , WaitStages()
    , SignalSemaphores()
    , AvailableCommandPools()
    , CommandPools()
    , CommandPoolsCS()
{
}

FVulkanQueue::~FVulkanQueue()
{
    SCOPED_LOCK(CommandPoolsCS);

    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        delete CommandPool;
    }

    CommandPools.Clear();
    AvailableCommandPools.Clear();
    
    Queue = VK_NULL_HANDLE;
}

bool FVulkanQueue::Initialize()
{
    TOptional<FVulkanQueueFamilyIndices> QueueIndices = GetDevice()->GetQueueIndicies();
    VULKAN_ERROR_COND(QueueIndices.HasValue(), "Queue Families is not initialized correctly");

    QueueFamilyIndex = GetDevice()->GetQueueIndexFromType(QueueType);
    vkGetDeviceQueue(GetDevice()->GetVkDevice(), QueueFamilyIndex, 0, &Queue);
    return true;
}

FVulkanCommandPool* FVulkanQueue::ObtainCommandPool()
{
    SCOPED_LOCK(CommandPoolsCS);
    
    if (!AvailableCommandPools.IsEmpty())
    {
        FVulkanCommandPool* CommandPool;
        if (AvailableCommandPools.Dequeue(CommandPool))
        {
            CommandPool->Reset();
            return CommandPool;
        }
    }
    
    FVulkanCommandPool* CommandPool = new FVulkanCommandPool(GetDevice(), QueueType);
    if (!CommandPool->Initialize())
    {
        DEBUG_BREAK();
        delete CommandPool;
        return nullptr;
    }

    CommandPools.Add(CommandPool);
    return CommandPool;
}

void FVulkanQueue::RecycleCommandPool(FVulkanCommandPool* InCommandPool)
{
    if (InCommandPool)
    {
        SCOPED_LOCK(CommandPoolsCS);
        AvailableCommandPools.Enqueue(InCommandPool);
    }
    else
    {
        LOG_WARNING("Trying to Recycle an invalid CommandPool");
    }
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
    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, SignalFence);
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
    vkQueueWaitIdle(Queue);
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

    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
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

FVulkanCommandPayload::FVulkanCommandPayload(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : Queue(InQueue)
    , Fence(nullptr)
    , Device(InDevice)
    , CommandPools()
    , CommandBuffers()
    , QueryPools()
    , DeletionQueue()
{
}

FVulkanCommandPayload::~FVulkanCommandPayload()
{
    CHECK(Fence == nullptr);
}

void FVulkanCommandPayload::Submit()
{
    CHECK(CommandBuffers.IsEmpty() == false);
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    Fence = FenceManager.ObtainFence();
    CHECK(Fence != nullptr);

    Queue.ExecuteCommandBuffer(CommandBuffers.Data(), CommandBuffers.Size(), Fence);
}

void FVulkanCommandPayload::Finish()
{
    // Resolve queries
    for (FVulkanQueryPool* QueryPool : QueryPools)
    {
        FVulkanQueryPoolManager* QueryPoolManager = QueryPool->GetQueryPoolManager();
        QueryPool->ResolveQueries();
        QueryPoolManager->RecycleQueryPool(QueryPool);
    }

    QueryPools.Clear();

    // Recycle all the CommandBuffers before CommandPools to avoid needing to lock the CommandPools
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        FVulkanCommandPool* CommandPool = CommandBuffer->GetOwnerPool();
        CommandPool->RecycleBuffer(CommandBuffer);
    }

    CommandBuffers.Clear();

    // Recycle all the CommandPool
    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        Queue.RecycleCommandPool(CommandPool);
    }
    
    CommandPools.Clear();

    // Recycle the fence
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    FenceManager.RecycleFence(Fence);
    Fence = nullptr;

    // Delete all the resources that has been queued up for destruction
    FVulkanDeferredObject::ProcessItems(DeletionQueue);
    DeletionQueue.Clear();

    // Destroy this instance after execution is finished
    delete this;
}
