#pragma once
#include "VulkanQueue.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanSurface> FVulkanSurfaceRef;

class FVulkanSurface : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSurface(FVulkanDevice* InDevice, FVulkanQueue& InQueue, void* InWindowHandle);
    ~FVulkanSurface();

    bool Initialize();
    bool GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const;
    bool GetSupportedPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const;
    bool GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const;

    const void* GetWindowHandle() const
    {
        return WindowHandle;
    }

    VkSurfaceKHR GetVkSurface() const
    {
        return Surface;
    }
    
private:
    VkSurfaceKHR  Surface;
    void*         WindowHandle;
    FVulkanQueue& Queue;
};
