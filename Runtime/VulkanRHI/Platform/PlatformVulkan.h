#pragma once

#if PLATFORM_WINDOWS
#include "VulkanRHI/Windows/WindowsVulkan.h"
typedef CWindowsVulkan PlatformVulkan;

#elif PLATFORM_MACOS
#include "VulkanRHI/Mac/MacVulkan.h"
typedef CMacVulkan PlatformVulkan;

#else
#include "VulkanRHI/Interface/PlatformVulkan.h"
typedef CPlatformVulkan PlatformVulkan;

#endif
