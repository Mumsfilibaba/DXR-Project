#pragma once
#include "VulkanFence.h"
#include "VulkanQuery.h"
#include "Core/Containers/Array.h"

class FVulkanCommandPool;
class FVulkanCommandBuffer;

class FVulkanCommandPacket : public FVulkanDeviceChild
{
public:
    FVulkanCommandPacket(FVulkanDevice* InDevice, FVulkanQueue& InQueue);
    ~FVulkanCommandPacket();

    void Submit();
    void HandleSubmitFinished();
    
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

    TArray<FVulkanQueryPool*> QueryPools;
    FDeletionQueueArray       Resources;

private:
    FVulkanQueue& Queue;
    FVulkanFence* Fence;

    TArray<FVulkanCommandPool*>   CommandPools;
    TArray<FVulkanCommandBuffer*> CommandBuffers;
};
