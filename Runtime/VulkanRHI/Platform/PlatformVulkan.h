#pragma once

#if PLATFORM_WINDOWS
    #include "VulkanRHI/Windows/WindowsVulkanPlatform.h"
    typedef FWindowsVulkanPlatform FPlatformVulkan;
#elif PLATFORM_MACOS
    #include "VulkanRHI/Mac/MacVulkanPlatform.h"
    typedef FMacVulkanPlatform FPlatformVulkan;
#else
    #include "VulkanRHI/Interface/PlatformVulkan.h"
    typedef FGenericVulkanPlatform FPlatformVulkan;
#endif
