#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceChild.h"
#include "VulkanDeletionQueue.h"
#include "VulkanFence.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"

typedef TSharedRef<class FVulkanQueue> FVulkanQueueRef;

class FVulkanCommandPool;
class FVulkanCommandBuffer;

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

    VkQueue GetVkQueue() const
    {
        return Queue;
    }

    EVulkanCommandQueueType GetType() const
    {
        return QueueType;
    }

    uint32 GetQueueFamilyIndex() const
    {
        return QueueFamilyIndex;
    }

private:
    VkQueue                      Queue;
    uint32                       QueueFamilyIndex;
    EVulkanCommandQueueType      QueueType;
    TArray<VkSemaphore>          WaitSemaphores;
    TArray<VkPipelineStageFlags> WaitStages;
    TArray<VkSemaphore>          SignalSemaphores;
    TArray<FVulkanCommandPool*>  CommandPools;
    FCriticalSection             CommandPoolsCS;
};
