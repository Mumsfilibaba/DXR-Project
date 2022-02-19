#pragma once

#if PLATFORM_WINDOWS
#include "VulkanRHI/Interface/PlatformVulkanExtensions.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsVulkanExtensions

class CWindowsVulkanExtensions : public CPlatformVulkanExtensions
{
public:

    static FORCEINLINE TArray<const char*> GetRequiredInstanceExtensions()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredInstanceLayers()
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceExtensions() 
    { 
        return TArray<const char*>(); 
    }

    static FORCEINLINE TArray<const char*> GetRequiredDeviceLayers()
    { 
        return TArray<const char*>(); 
    }
};

#endif