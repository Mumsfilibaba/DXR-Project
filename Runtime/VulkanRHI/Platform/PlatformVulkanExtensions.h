#pragma once

#if PLATFORM_WINDOWS
#include "VulkanRHI/Windows/WindowsVulkanExtensions.h"
typedef CWindowsVulkanExtensions PlatformVulkanExtensions;

#elif PLATFORM_MACOS
#include "VulkanRHI/Mac/MacVulkanExtensions.h"
typedef CMacVulkanExtensions PlatformVulkanExtensions;

#else
#include "VulkanRHI/Interface/PlatformVulkanExtensions.h"
typedef CPlatformVulkanExtensions PlatformVulkanExtensions;

#endif
