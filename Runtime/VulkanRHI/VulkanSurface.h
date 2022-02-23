#pragma once
#include "VulkanDeviceObject.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSurface

class CVulkanSurface : public CVulkanDeviceObject, public CRefCounted
{
public:

    static TSharedRef<CVulkanSurface> CreateSurface(CVulkanDevice* InDevice, CPlatformWindow* InWindow);

	FORCEINLINE VkSurfaceKHR GetVkSurface() const
	{
		return Surface;
	}
	
private:

    CVulkanSurface(CVulkanDevice* InDevice, CPlatformWindow* InWindow);
    ~CVulkanSurface();

    bool Initialize();

	TSharedRef<CPlatformWindow> Window;
    VkSurfaceKHR                Surface;
    TArray<VkSurfaceFormatKHR>  SupportedFormats;
    TArray<VkPresentModeKHR>    PresentModes;
};
