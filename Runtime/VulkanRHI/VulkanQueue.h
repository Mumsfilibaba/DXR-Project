#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/Map.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanDeletionQueue.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanResourceState.h"
#include "VulkanRHI/VulkanCommandBuffer.h"

enum class EVulkanCommandsFlags : uint32
{
    None           = 0,
    ResolveQueries = 1 << 0,
};

ENUM_CLASS_OPERATORS(EVulkanCommandsFlags);

class FVulkanCommandPool;
struct FVulkanCommands;

typedef TSharedRef<class FVulkanQueue> FVulkanQueueRef;

class FVulkanQueue : public FVulkanDeviceChild
{
public:
    FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InQueueType);
    ~FVulkanQueue();

    bool Initialize();
    
    FVulkanCommandPool* ObtainCommandPool();
    void RecycleCommandPool(FVulkanCommandPool* InCommandPool);
    
    bool ExecuteCommandBuffer(class FVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, class FVulkanFence* Fence);
    
    void SubmitCommands(FVulkanCommands* Commands);
    void ProcessCommandQueue();

    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);
    void AddWaitTimelineSemaphore(VkSemaphore Semaphore, uint64 Value, VkPipelineStageFlags WaitStage);
    void AddSignalSemaphore(VkSemaphore Semaphore);
    void AddSignalTimelineSemaphore(VkSemaphore Semaphore, uint64 Value);
    
    bool IsSignalingSemaphore(VkSemaphore Semaphore)  const { return SignalSemaphores.Contains(Semaphore); }
    bool IsWaitingForSemaphore(VkSemaphore Semaphore) const { return WaitSemaphores.Contains(Semaphore); }
    
    void WaitForCompletion();

    // Create empty submit that waits for the semaphores and waits for completion
    bool FlushWaitSemaphoresAndWait();

    // Drop any pending WAIT/SIGNAL entries without submitting them. Caller must ensure the GPU is idle
    // (e.g. via WaitForCompletion) so that discarding the referenced binary/timeline semaphores is safe.
    void ClearPendingSemaphores();

    // Remove all pending WAIT and SIGNAL entries that reference the given semaphore (binary or timeline).
    // Keeps WaitSemaphores/WaitStages/WaitSemaphoreValues in lockstep and likewise for Signal*.
    void RemovePendingSemaphore(VkSemaphore Semaphore);

    void SetDebugName(const String& Name)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *Name, Queue, VK_OBJECT_TYPE_QUEUE);
    #if VULKAN_STORE_DEBUG_NAMES
        DebugName = Name;
    #endif
    }

    void GetDebugName(String& OutDebugName) const
    {
    #if VULKAN_STORE_DEBUG_NAMES
        OutDebugName = DebugName;
    #else
        OutDebugName.Clear();
    #endif
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
    typedef TQueue<FVulkanCommands*, EQueueType::MPSC> FCommandsQueue;

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
    FCommandsQueue               PendingSubmissions;
    FCriticalSection             SubmissionCS;
    FCriticalSection             ConsumerCS;
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    TArray<FVulkanQueryRange>    PendingQueryRanges;
    TArray<FVulkanQuery>         PendingTimestampQueries;
    TArray<FVulkanQuery>         PendingOcclusionQueries;
    TArray<FVulkanQuery>         PendingPipelineStatsQueries;
    TArray<FVulkanQueryRHI*>     PendingQueryRHIs;
#endif
#if VULKAN_STORE_DEBUG_NAMES
    String                       DebugName;
#endif
};

struct FVulkanCommands
{
    FVulkanCommands(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommands();

    void AcquireFence();
    void PreExecute();
    void Execute();
    void PostExecute();

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
    void ValidateImageLayouts();
    void ValidatePendingBarrierOrdering();
#endif

    void AddCommandPool(FVulkanCommandPool* InCommandPool)
    {
        CommandPools.Add(InCommandPool);
    }

    void AddCommandBuffer(FVulkanCommandBuffer* InCommandBuffer)
    {
        CommandBuffers.Add(InCommandBuffer);
    }

    bool IsExecutionFinished() const
    {
        return Fence ? Fence->IsSignaled() : false;
    }

    bool IsEmpty() const
    {
        return CommandBuffers.IsEmpty();
    }

    FVulkanQueue&                                     Queue;
    FVulkanDevice* const                              Device;
    EVulkanCommandsFlags                              Flags;
    FVulkanFence*                                     Fence;
    TArray<FVulkanCommandPool*>                       CommandPools;
    TArray<FVulkanCommandBuffer*>                     CommandBuffers;
    TArray<FVulkanQueryRange>                         QueryRanges;
    TArray<FVulkanQuery>                              TimestampQueries;
    TArray<FVulkanQuery>                              OcclusionQueries;
    TArray<FVulkanQuery>                              PipelineStatsQueries;
    TArray<struct FVulkanQueryRHI*>                   PendingQueries;
    TArray<FVulkanDeferredObject>                     DeferredObjects;
    TArray<FVulkanPendingImageBarrier>                PendingImageBarriers;
    TArray<FVulkanPendingBufferBarrier>               PendingBufferBarriers;
    TMap<FVulkanTextureRHI*, FVulkanImageLayoutState> PendingImageStates;
    TMap<FVulkanBufferRHI*, FVulkanBufferState>       PendingBufferStates;
};
