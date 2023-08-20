#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"
#include "Core/Containers/SharedRef.h"

class FVulkanQueue : public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~FVulkanQueue();

    bool Initialize();

    bool ExecuteCommandBuffer(class FVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, class FVulkanFence* Fence);

    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);
    
    void AddSignalSemaphore(VkSemaphore Semaphore);
    
    bool IsWaitingForSemaphore(VkSemaphore Semaphore) const { return WaitSemaphores.Contains(Semaphore); }

    bool IsSignalingSemaphore(VkSemaphore Semaphore) const { return SignalSemaphores.Contains(Semaphore); }

    void WaitForCompletion();

    // Create empty submit that waits for the semaphores and waits for completion
    bool FlushWaitSemaphoresAndWait();

    FORCEINLINE void SetName(const FString& Name)
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), Name.GetCString(), CommandQueue, VK_OBJECT_TYPE_QUEUE);
    }

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
    EVulkanCommandQueueType Type;
    
    VkQueue CommandQueue;
    uint32  QueueFamilyIndex;

    TArray<VkSemaphore>          WaitSemaphores;
    TArray<VkPipelineStageFlags> WaitStages;
    TArray<VkSemaphore>          SignalSemaphores;
};
