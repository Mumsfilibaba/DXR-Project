#pragma once
#include "VulkanQueue.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class CVulkanSurface> CVulkanSurfaceRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSurface

class CVulkanSurface : public CVulkanDeviceObject, public CRefCounted
{
public:

    static CVulkanSurfaceRef CreateSurface(CVulkanDevice* InDevice, CVulkanQueue* InQueue, PlatformWindowHandle InWindowHandle);

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

    CVulkanSurface(CVulkanDevice* InDevice, CVulkanQueue* InQueue, PlatformWindowHandle InWindowHandle);
    ~CVulkanSurface();

    bool Initialize();

    CVulkanQueueRef      Queue;
    VkSurfaceKHR         Surface;
    PlatformWindowHandle WindowHandle;
};
