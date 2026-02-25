#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanDeletionQueue.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanResourceState.h"

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
    void AddWaitTimelineSemaphore(VkSemaphore Semaphore, uint64 Value, VkPipelineStageFlags WaitStage);
    void AddSignalSemaphore(VkSemaphore Semaphore);
    void AddSignalTimelineSemaphore(VkSemaphore Semaphore, uint64 Value);
    
    bool IsWaitingForSemaphore(VkSemaphore Semaphore) const { return WaitSemaphores.Contains(Semaphore); }
    bool IsSignalingSemaphore(VkSemaphore Semaphore)  const { return SignalSemaphores.Contains(Semaphore); }
    
    void WaitForCompletion();

    // Create empty submit that waits for the semaphores and waits for completion
    bool FlushWaitSemaphoresAndWait();

    void SetDebugName(const FString& Name)
    {
        VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *Name, Queue, VK_OBJECT_TYPE_QUEUE);
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
    TArray<uint64>               WaitSemaphoreValues;
    TArray<VkSemaphore>          SignalSemaphores;
    TArray<uint64>               SignalSemaphoreValues;
    TQueue<FVulkanCommandPool*>  AvailableCommandPools;
    TArray<FVulkanCommandPool*>  CommandPools;
    FCriticalSection             CommandPoolsCS;
};

struct FVulkanCommands
{
    FVulkanCommands(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommands();

    void AcquireFence();
    void PreExecute();
    void Execute();
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

    void AddDescriptorPool(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorPool* Pool)
    {
        PendingDescriptorPools.Add(TPair<FVulkanDescriptorPoolInfo, FVulkanDescriptorPool*>(PoolInfo, Pool));
    }

    bool IsExecutionFinished() const
    {
        return Fence ? Fence->IsSignaled() : false;
    }

    bool IsEmpty() const
    {
        return CommandBuffers.IsEmpty();
    }

    FVulkanQueue&                            Queue;
    FVulkanDevice* const                     Device;
    FVulkanFence*                            Fence;
    TArray<FVulkanCommandPool*>              CommandPools;
    TArray<FVulkanCommandBuffer*>            CommandBuffers;
    TArray<FVulkanQueryPool*>                QueryPools;
    TArray<FVulkanDeferredObject>            DeletionQueue;
    TArray<TPair<FVulkanDescriptorPoolInfo, FVulkanDescriptorPool*>> PendingDescriptorPools;
    TArray<FVulkanPendingImageBarrier>       PendingImageBarriers;
    TArray<FVulkanPendingBufferBarrier>      PendingBufferBarriers;
    TMap<FVulkanTexture*, FVulkanImageState> PendingImageStates;
    TMap<FVulkanBuffer*, FVulkanBufferState> PendingBufferStates;
};
