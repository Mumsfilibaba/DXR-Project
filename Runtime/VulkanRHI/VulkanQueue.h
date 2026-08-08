#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/Map.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanDeletionQueue.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanRecyclePool.h"
#include "VulkanRHI/VulkanResourceState.h"
#include "VulkanRHI/VulkanCommandBuffer.h"

enum class EVulkanCommandsFlags : uint32
{
    None           = 0,
    ResolveQueries = 1 << 0,
};

ENUM_CLASS_OPERATORS(EVulkanCommandsFlags);

class FVulkanCommandPool;
class FVulkanCommandContext;
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
    void RetireCommandPoolDeferred(FVulkanCommandPool* InCommandPool);

    FVulkanCommandContext* ObtainCommandContext();
    void ReleaseCommandContext(FVulkanCommandContext* InContext);
    void PruneCommandContexts(uint64 CurrentFrame);

    bool ExecuteCommands(FVulkanCommands& InCommands);
    
    void SubmitCommands(FVulkanCommands* Commands);
    void ProcessCommandQueue();

    void WaitForCompletion();

    VkResult Present(const VkPresentInfoKHR& PresentInfo);

    bool SubmitSemaphoresOnly(VkSemaphore WaitSemaphore, VkPipelineStageFlags WaitStage, VkSemaphore SignalSemaphore);
    void RetireDeferredObjects(TArray<FVulkanDeferredObject>&& InObjects);

    template<typename FunctorType>
    void ForEachLiveCommandContext(FunctorType&& Functor)
    {
        CommandContextPool.ForEachTracked(Forward<FunctorType>(Functor));
    }

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

    VkQueue                                   Queue;
    uint32                                    QueueFamilyIndex;
    EVulkanCommandQueueType                   QueueType;
    TVulkanRecyclePool<FVulkanCommandPool>    CommandPoolPool;
    TVulkanRecyclePool<FVulkanCommandContext> CommandContextPool;
    TArray<FVulkanCommandPool*>               DeferredCommandPools;
    FCriticalSection                          DeferredCommandPoolsCS;
    TArray<FVulkanDeferredObject>             DeferredObjects;
    FCriticalSection                          DeferredObjectsCS;
    TAtomicInt<uint64>                        CurrentFrame;
    FCommandsQueue                            PendingSubmissions;
    FCriticalSection                          QueueCS;
    FCriticalSection                          ConsumerCS;
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    TArray<FVulkanQueryRange>                 PendingQueryRanges;
    TArray<FVulkanQuery>                      PendingTimestampQueries;
    TArray<FVulkanQuery>                      PendingOcclusionQueries;
    TArray<FVulkanQuery>                      PendingPipelineStatsQueries;
    TArray<FVulkanQueryRHI*>                  PendingQueryRHIs;
#endif
#if VULKAN_STORE_DEBUG_NAMES
    String                                    DebugName;
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

    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);
    void AddWaitTimelineSemaphore(VkSemaphore Semaphore, uint64 Value, VkPipelineStageFlags WaitStage);
    void AddSignalSemaphore(VkSemaphore Semaphore);
    void AddSignalTimelineSemaphore(VkSemaphore Semaphore, uint64 Value);

    void AddCommandPool(FVulkanCommandPool* InCommandPool)
    {
        CommandPools.Add(InCommandPool);
    }

    void AddCommandBuffer(FVulkanCommandBuffer* InCommandBuffer)
    {
        CommandBuffers.Add(InCommandBuffer);
    }

    bool HasPendingSemaphores() const
    {
        return !WaitSemaphores.IsEmpty() || !SignalSemaphores.IsEmpty();
    }

    bool IsExecutionFinished() const
    {
        return Fence ? Fence->IsSignaled() : false;
    }

    bool IsEmpty() const
    {
        return CommandBuffers.IsEmpty() && !HasPendingSemaphores();
    }

    FVulkanQueue&                                     Queue;
    FVulkanDevice* const                              Device;
    EVulkanCommandsFlags                              Flags;
    FVulkanFence*                                     Fence;
    TArray<FVulkanCommandPool*>                       CommandPools;
    TArray<FVulkanCommandBuffer*>                     CommandBuffers;
    TArray<VkSemaphore>                               WaitSemaphores;
    TArray<VkPipelineStageFlags>                      WaitStages;
    TArray<uint64>                                    WaitSemaphoreValues;
    TArray<VkSemaphore>                               SignalSemaphores;
    TArray<uint64>                                    SignalSemaphoreValues;
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
