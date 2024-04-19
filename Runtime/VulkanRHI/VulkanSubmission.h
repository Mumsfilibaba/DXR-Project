#pragma once
#include "VulkanFence.h"
#include "VulkanQuery.h"
#include "Core/Containers/Array.h"

class FVulkanCommandPool;
class FVulkanCommandBuffer;

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

    FVulkanQueue&  Queue;
    FVulkanFence*  Fence;
    FVulkanDevice* Device;

    TArray<FVulkanCommandPool*>   CommandPools;
    TArray<FVulkanCommandBuffer*> CommandBuffers;
    TArray<FVulkanQueryPool*>     QueryPools;
    TArray<FVulkanDeferredObject> DeletionQueue;
};
