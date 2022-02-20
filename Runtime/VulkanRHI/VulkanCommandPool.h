#pragma once
#include "VulkanDevice.h"
#include "VulkanDeviceObject.h"

#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandPool

class CVulkanCommandPool : public CVulkanDeviceObject
{
public:

    CVulkanCommandPool(CVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~CVulkanCommandPool();

    bool Initialize();

	FORCEINLINE VkCommandPool GetVkCommandPool() const
	{
		return CommandPool;
	}
	
private:

    EVulkanCommandQueueType Type;
    VkCommandPool           CommandPool;
};
