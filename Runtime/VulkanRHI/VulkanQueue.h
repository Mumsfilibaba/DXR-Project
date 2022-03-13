#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class CVulkanQueue> CVulkanQueueRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanQueue

class CVulkanQueue : public CVulkanDeviceObject, public CRefCounted
{
public:

    static CVulkanQueueRef CreateQueue(CVulkanDevice* InDevice, EVulkanCommandQueueType InType);

    bool ExecuteCommandBuffer(class CVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, class CVulkanFence* Fence);

    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);
    void AddSignalSemaphore(VkSemaphore Semaphore);
    
    bool IsWaitingForSemaphore(VkSemaphore Semaphore) const { return WaitSemaphores.Contains(Semaphore); }
    bool IsSignalingSemaphore(VkSemaphore Semaphore) const { return SignalSemaphores.Contains(Semaphore); }

    void WaitForCompletion();

    // Create empty submit that waits for the semaphores and waits for completion
    void Flush();

    FORCEINLINE VkQueue GetVkQueue() const
    {
        return CommandQueue;
    }

    FORCEINLINE EVulkanCommandQueueType GetType() const
    {
        return Type;
    }

    FORCEINLINE uint32 GetQueueFamilyIndex() const
    {
        return QueueFamilyIndex;
    }

private:

    CVulkanQueue(CVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~CVulkanQueue();

    bool Initialize();

    EVulkanCommandQueueType Type;
    
    VkQueue CommandQueue;
    uint32  QueueFamilyIndex;

    TArray<VkSemaphore>          WaitSemaphores;
    TArray<VkPipelineStageFlags> WaitStages;
    TArray<VkSemaphore>          SignalSemaphores;
};
