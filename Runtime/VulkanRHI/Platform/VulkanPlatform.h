#pragma once

#if PLATFORM_WINDOWS
    #include "VulkanRHI/Windows/VulkanPlatformWindows.h"
    typedef VulkanPlatformWindows VulkanPlatform;
#elif PLATFORM_MACOS
    #include "VulkanRHI/Mac/VulkanPlatformMac.h"
    typedef VulkanPlatformMac VulkanPlatform;
#else
    #include "VulkanRHI/Base/VulkanPlatformBase.h"
    typedef VulkanPlatformBase VulkanPlatform;
#endif
