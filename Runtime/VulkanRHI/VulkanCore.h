#pragma once
#include "Core/Core.h"
#include "Core/Debug/Debug.h"
#include "Core/Logging/Log.h"

#define VK_NO_PROTOTYPES (1)
#include <vulkan/vulkan.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Vulkan Typedefs

typedef PFN_vkVoidFunction VulkanVoidFunction;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Vulkan Error

#if !PRODUCTION_BUILD
#define VULKAN_ERROR(Condition, ErrorMessage) \
    do                                        \
    {                                         \
        if (!(Condition))                     \
        {                                     \
            LOG_ERROR(ErrorMessage);          \
            CDebug::DebugBreak();             \
        }                                     \
    } while (0)

#define VULKAN_ERROR_ALWAYS(ErrorMessage) \
    do                                    \
    {                                     \
        LOG_ERROR(ErrorMessage);          \
        CDebug::DebugBreak();             \
    } while (0)

#else
#define VULKAN_ERROR(Condtion, ErrorString) do {} while(0)
#define VULKAN_ERROR_ALWAYS(ErrorString)    do {} while(0)
#endif

#ifndef VK_SUCCEEDED
	#define VK_SUCCEEDED(Result) (Result == VK_SUCCESS)
#endif

#ifndef VK_FAILED
	#define VK_FAILED(Result) (Result != VK_SUCCESS)
#endif

#ifndef VULKAN_CHECK
    #define VULKAN_CHECK(Result, ErrorMessage) \
        if (VK_FAILED(Result))                 \
        {                                      \
            VULKAN_ERROR_ALWAYS(ErrorMessage); \
            return false;                      \
        }
#endif

#ifndef VULKAN_CHECK_HANDLE
	#define VULKAN_CHECK_HANDLE(Handle) (Handle != VK_NULL_HANDLE)
#endif
