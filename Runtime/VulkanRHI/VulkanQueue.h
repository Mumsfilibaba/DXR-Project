#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceChild.h"
#include "VulkanDeletionQueue.h"
#include "VulkanFence.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"

typedef TSharedRef<class FVulkanQueue> FVulkanQueueRef;

class FVulkanCommandPool;
class FVulkanCommandBuffer;
class FVulkanQueryPool;

class FVulkanQueue : public FVulkanDeviceChild
{
public:
    FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InQueueType);
    ~FVulkanQueue();

    bool Initialize();
    FVulkanCommandPool* ObtainCommandPool();
    void RecycleCommandPool(FVulkanCommandPool* InCommandPool);
    bool ExecuteCommandBuffer(class FVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, class FVulkanFence* Fence);
    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);
    void AddSignalSemaphore(VkSemaphore Semaphore);
    bool IsWaitingForSemaphore(VkSemaphore Semaphore) const { return WaitSemaphores.Contains(Semaphore); }
    bool IsSignalingSemaphore(VkSemaphore Semaphore)  const { return SignalSemaphores.Contains(Semaphore); }
    void WaitForCompletion();

    // Create empty submit that waits for the semaphores and waits for completion
    bool FlushWaitSemaphoresAndWait();

    void SetDebugName(const FString& Name)
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), Name.GetCString(), Queue, VK_OBJECT_TYPE_QUEUE);
    }

    VkQueue GetVkQueue() const { return Queue; }
    EVulkanCommandQueueType GetType() const { return QueueType; }
    uint32 GetQueueFamilyIndex() const { return QueueFamilyIndex; }

private:
    VkQueue                      Queue;
    uint32                       QueueFamilyIndex;
    EVulkanCommandQueueType      QueueType;
    TArray<VkSemaphore>          WaitSemaphores;
    TArray<VkPipelineStageFlags> WaitStages;
    TArray<VkSemaphore>          SignalSemaphores;
    TQueue<FVulkanCommandPool*>  AvailableCommandPools;
    TArray<FVulkanCommandPool*>  CommandPools;
    FCriticalSection             CommandPoolsCS;
};

struct FVulkanCommandPayload
{
    FVulkanCommandPayload(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommandPayload();

    void Submit();
    void Finish();

    void AddCommandPool(FVulkanCommandPool* InCommandPool)
    {
        CommandPools.Add(InCommandPool);
    }

    void AddCommandBuffer(FVulkanCommandBuffer* InCommandBuffer)
    {
        CommandBuffers.Add(InCommandBuffer);
    }

    void AddQueryPool(FVulkanQueryPool* InQueryPool)
    {
        QueryPools.Add(InQueryPool);
    }

    bool IsExecutionFinished() const
    {
        if (Fence)
        {
            return Fence->IsSignaled();
        }
        
        return false;
    }

    bool IsEmpty() const
    {
        return CommandBuffers.IsEmpty();
    }

    FVulkanQueue&                 Queue;
    FVulkanFence*                 Fence;
    FVulkanDevice*                Device;
    TArray<FVulkanCommandPool*>   CommandPools;
    TArray<FVulkanCommandBuffer*> CommandBuffers;
    TArray<FVulkanQueryPool*>     QueryPools;
    TArray<FVulkanDeferredObject> DeletionQueue;
};