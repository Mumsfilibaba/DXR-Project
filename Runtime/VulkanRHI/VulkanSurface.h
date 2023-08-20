#pragma once
#include "VulkanQueue.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanSurface> FVulkanSurfaceRef;

class FVulkanSurface : public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanSurface(FVulkanDevice* InDevice, FVulkanQueue* InQueue, void* InWindowHandle);
    ~FVulkanSurface();

    bool Initialize();

    bool GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const;
    
    bool GetPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const;
    
    bool GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const;

    FORCEINLINE const void* GetWindowHandle() const
    {
        return WindowHandle;
    }

    FORCEINLINE VkSurfaceKHR GetVkSurface() const
    {
        return Surface;
    }
    
private:
    FVulkanQueueRef Queue;

    VkSurfaceKHR Surface;
    void*        WindowHandle;
};
