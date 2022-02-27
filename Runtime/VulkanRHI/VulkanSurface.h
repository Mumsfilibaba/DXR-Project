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

    static CVulkanSurfaceRef CreateSurface(CVulkanDevice* InDevice, CVulkanQueue* Queue, void* InWindowHandle);

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

    CVulkanSurface(CVulkanDevice* InDevice, CVulkanQueue* Queue, void* InWindowHandle);
    ~CVulkanSurface();

    bool Initialize();

    TSharedRef<CVulkanQueue> Queue;
	void*                    WindowHandle;
    VkSurfaceKHR             Surface;
};
