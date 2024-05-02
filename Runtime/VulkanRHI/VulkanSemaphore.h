#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanLoader.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanSemaphore> FVulkanSemaphoreRef;

class FVulkanSemaphore : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSemaphore(FVulkanDevice* InDevice);
    ~FVulkanSemaphore();

    bool Initialize();
    bool SetDebugName(const FString& Name);

    FORCEINLINE VkSemaphore GetVkSemaphore() const
    {
        return Semaphore;
    }
    
private:
    VkSemaphore Semaphore;
};
