#pragma once

#if PLATFORM_WINDOWS
#include "VulkanRHI/Windows/WindowsVulkanMisc.h"
typedef CWindowsVulkanMisc PlatformVulkanMisc;

#elif PLATFORM_MACOS
#include "VulkanRHI/Mac/MacVulkanMisc.h"
typedef CMacVulkanMisc PlatformVulkanMisc;

#else
#include "VulkanRHI/Interface/PlatformVulkanMisc.h"
typedef CPlatformVulkanMisc PlatformVulkanMisc;

#endif
