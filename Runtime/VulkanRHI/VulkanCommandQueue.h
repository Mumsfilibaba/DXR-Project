#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandQueue

class CVulkanCommandQueue : public CVulkanDeviceObject
{
public:

    CVulkanCommandQueue(CVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~CVulkanCommandQueue();

    bool Initialize();

    bool ExecuteCommandBuffer(class CVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers);

    /**
     * Adds a Semaphore to the queue which will be waited on during next call to execute commandbuffer
     * 
     * @param Semaphore: Semaphore to wait for
     * @param WaitStage: Pipeline-stage at which the CommandQueue should wait for the semaphore
     */
    void AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage);

    /**
     * Adds a Semaphore to the queue which will be signaled when the next call to execute commandbuffer finishes
     * 
     * @param Semaphore: Semaphore to signal
     */
    void AddSignalSemaphore(VkSemaphore Semaphore);
	
	FORCEINLINE VkQueue GetVkQueue() const
	{
		return CommandQueue;
	}

private:
	EVulkanCommandQueueType Type;
    VkQueue                 CommandQueue;

    TArray<VkSemaphore>          WaitSemaphores;
    TArray<VkPipelineStageFlags> WaitStages;
    TArray<VkSemaphore>          SignalSemaphores;
};
