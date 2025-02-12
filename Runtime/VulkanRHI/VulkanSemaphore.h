#pragma once
#include "Core/Containers/SharedRef.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanRefCounted.h"

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
