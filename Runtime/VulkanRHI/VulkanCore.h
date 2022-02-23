#pragma once
#include "Core/Core.h"
#include "Core/Debug/Debug.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/String.h"

#if PLATFORM_MACOS
    #define VK_USE_PLATFORM_MACOS_MVK
    // #define VK_USE_PLATFORM_METAL_EXT
#elif PLATFORM_WINDOWS
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES (1)
#include <vulkan/vulkan.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Vulkan Typedefs

typedef PFN_vkVoidFunction VulkanVoidFunction;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Vulkan Error

#if !PRODUCTION_BUILD
#define VULKAN_ERROR_ALWAYS(ErrorMessage)                         \
    do                                                            \
    {                                                             \
        LOG_ERROR(String("[VulkanRHI] ") + String(ErrorMessage)); \
        CDebug::DebugBreak();                                     \
    } while (0)

#define VULKAN_ERROR(Condition, ErrorMessage)  \
    do                                         \
    {                                          \
        if (!(Condition))                      \
        {                                      \
            VULKAN_ERROR_ALWAYS(ErrorMessage); \
        }                                      \
    } while (0)

#define VULKAN_WARNING(Message)                                \
    do                                                         \
    {                                                          \
        LOG_WARNING(String("[VulkanRHI] ") + String(Message)); \
    } while (0)

#define VULKAN_INFO(Message)                                \
    do                                                      \
    {                                                       \
        LOG_INFO(String("[VulkanRHI] ") + String(Message)); \
    } while (0)

#else
#define VULKAN_ERROR_ALWAYS(ErrorString)    do {} while(0)
#define VULKAN_ERROR(Condtion, ErrorString) do {} while(0)
#define VULKAN_WARNING(Message)             do {} while(0)
#endif

#ifndef VULKAN_SUCCEEDED
	#define VULKAN_SUCCEEDED(Result) (Result == VK_SUCCESS)
#endif

#ifndef VULKAN_FAILED
	#define VULKAN_FAILED(Result) (Result != VK_SUCCESS)
#endif

#ifndef VULKAN_CHECK_RESULT
    #define VULKAN_CHECK_RESULT(Result, ErrorMessage) \
        if (VULKAN_FAILED(Result))                        \
        {                                             \
            VULKAN_ERROR_ALWAYS(ErrorMessage);        \
            return false;                             \
        }
#endif

#ifndef VULKAN_CHECK_HANDLE
	#define VULKAN_CHECK_HANDLE(Handle) (Handle != VK_NULL_HANDLE)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Vulkan Helper functions

inline String GetVersionAsString(uint32 VersionNumber)
{
	return
		ToString(VK_API_VERSION_MAJOR(VersionNumber)) + '.' +
		ToString(VK_API_VERSION_MINOR(VersionNumber)) + '.' +
		ToString(VK_API_VERSION_PATCH(VersionNumber)) + '.' +
		ToString(VK_API_VERSION_VARIANT(VersionNumber));
}
