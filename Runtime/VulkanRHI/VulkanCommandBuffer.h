#pragma once
#include "VulkanDeviceObject.h"

class CVulkanCommandPool;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandBuffer

class CVulkanCommandBuffer : public CVulkanDeviceObject
{
public:

    CVulkanCommandBuffer(CVulkanDevice* InDevice);
    ~CVulkanCommandBuffer();

    bool Initialize(CVulkanCommandPool* InCommandPool, VkCommandBufferLevel InLevel);

    inline void Begin()
    {
    }

	FORCEINLINE VkCommandBuffer GetVkCommandBuffer() const
	{
		return CommandBuffer;
	}
	
private:
    CVulkanCommandPool* CommandPool;
    VkCommandBuffer     CommandBuffer;

    VkCommandBufferLevel Level;
};
