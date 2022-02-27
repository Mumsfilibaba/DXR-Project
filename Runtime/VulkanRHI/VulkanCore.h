#pragma once
#include "Core/Core.h"
#include "Core/Debug/Debug.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/String.h"

#include "RHI/TextureFormat.h"

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

inline VkFormat ConvertFormat(EFormat Format)
{
    switch (Format)
    {
    case EFormat::R32G32B32A32_Float:    return VK_FORMAT_R32G32B32A32_SFLOAT;
    case EFormat::R32G32B32A32_Uint:     return VK_FORMAT_R32G32B32A32_UINT;
    case EFormat::R32G32B32A32_Sint:     return VK_FORMAT_R32G32B32A32_SINT;
    
    case EFormat::R32G32B32_Float:       return VK_FORMAT_R32G32B32_SFLOAT;
    case EFormat::R32G32B32_Uint:        return VK_FORMAT_R32G32B32_UINT;
    case EFormat::R32G32B32_Sint:        return VK_FORMAT_R32G32B32_SINT;
    
    case EFormat::R16G16B16A16_Float:    return VK_FORMAT_R16G16B16A16_SFLOAT;
    case EFormat::R16G16B16A16_Unorm:    return VK_FORMAT_R16G16B16A16_UNORM;
    case EFormat::R16G16B16A16_Uint:     return VK_FORMAT_R16G16B16A16_UINT;
    case EFormat::R16G16B16A16_Snorm:    return VK_FORMAT_R16G16B16A16_SNORM;
    case EFormat::R16G16B16A16_Sint:     return VK_FORMAT_R16G16B16A16_SINT;
    
    case EFormat::R32G32_Float:          return VK_FORMAT_R32G32_SFLOAT;
    case EFormat::R32G32_Uint:           return VK_FORMAT_R32G32_UINT;
    case EFormat::R32G32_Sint:           return VK_FORMAT_R32G32_SINT;
    
    case EFormat::R10G10B10A2_Unorm:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case EFormat::R10G10B10A2_Uint:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    
    case EFormat::R11G11B10_Float:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    
    case EFormat::R8G8B8A8_Unorm:        return VK_FORMAT_R8G8B8A8_UNORM;
    case EFormat::R8G8B8A8_Unorm_SRGB:   return VK_FORMAT_R8G8B8A8_SRGB;
    case EFormat::R8G8B8A8_Uint:         return VK_FORMAT_R8G8B8A8_UINT;
    case EFormat::R8G8B8A8_Snorm:        return VK_FORMAT_R8G8B8A8_SNORM;
    case EFormat::R8G8B8A8_Sint:         return VK_FORMAT_R8G8B8A8_SINT;

    case EFormat::B8G8R8A8_Unorm:        return VK_FORMAT_B8G8R8A8_UNORM;
    case EFormat::B8G8R8A8_Unorm_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
    case EFormat::B8G8R8A8_Uint:         return VK_FORMAT_B8G8R8A8_UINT;
    case EFormat::B8G8R8A8_Snorm:        return VK_FORMAT_B8G8R8A8_SNORM;
    case EFormat::B8G8R8A8_Sint:         return VK_FORMAT_B8G8R8A8_SINT;
    
    case EFormat::R16G16_Float:          return VK_FORMAT_R16G16_SFLOAT;
    case EFormat::R16G16_Unorm:          return VK_FORMAT_R16G16_UNORM;
    case EFormat::R16G16_Uint:           return VK_FORMAT_R16G16_UINT;
    case EFormat::R16G16_Snorm:          return VK_FORMAT_R16G16_SNORM;
    case EFormat::R16G16_Sint:           return VK_FORMAT_R16G16_SINT;
    
    case EFormat::D32_Float:             return VK_FORMAT_D32_SFLOAT;
    case EFormat::R32_Float:             return VK_FORMAT_R32_SFLOAT;
    case EFormat::R32_Uint:              return VK_FORMAT_R32_UINT;
    case EFormat::R32_Sint:              return VK_FORMAT_R32_SINT;
    
    case EFormat::D24_Unorm_S8_Uint:     return VK_FORMAT_D24_UNORM_S8_UINT;
    
    case EFormat::R8G8_Unorm:            return VK_FORMAT_R8G8_UNORM;
    case EFormat::R8G8_Uint:             return VK_FORMAT_R8G8_UINT;
    case EFormat::R8G8_Snorm:            return VK_FORMAT_R8G8_SNORM;
    case EFormat::R8G8_Sint:             return VK_FORMAT_R8G8_SINT;
    
    case EFormat::R16_Float:             return VK_FORMAT_R16_SFLOAT;
    case EFormat::D16_Unorm:             return VK_FORMAT_D16_UNORM;
    case EFormat::R16_Unorm:             return VK_FORMAT_R16_UNORM;
    case EFormat::R16_Uint:              return VK_FORMAT_R16_UINT;
    case EFormat::R16_Snorm:             return VK_FORMAT_R16_SNORM;
    case EFormat::R16_Sint:              return VK_FORMAT_R16_SINT;
    
    case EFormat::R8_Unorm:              return VK_FORMAT_R8_UNORM;
    case EFormat::R8_Uint:               return VK_FORMAT_R8_UINT;
    case EFormat::R8_Snorm:              return VK_FORMAT_R8_SNORM;
    case EFormat::R8_Sint:               return VK_FORMAT_R8_SINT;
    
    default:                             return VK_FORMAT_UNDEFINED;
    }
}

inline const char* ToString(VkFormat Format)
{
    switch (Format)
    {
    case VK_FORMAT_R4G4_UNORM_PACK8:                            return "VK_FORMAT_R4G4_UNORM_PACK8";
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:                       return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:                       return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
    case VK_FORMAT_R5G6B5_UNORM_PACK16:                         return "VK_FORMAT_R5G6B5_UNORM_PACK16";
    case VK_FORMAT_B5G6R5_UNORM_PACK16:                         return "VK_FORMAT_B5G6R5_UNORM_PACK16";
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:                       return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:                       return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:                       return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
    case VK_FORMAT_R8_UNORM:                                    return "VK_FORMAT_R8_UNORM";
    case VK_FORMAT_R8_SNORM:                                    return "VK_FORMAT_R8_SNORM";
    case VK_FORMAT_R8_USCALED:                                  return "VK_FORMAT_R8_USCALED";
    case VK_FORMAT_R8_SSCALED:                                  return "VK_FORMAT_R8_SSCALED";
    case VK_FORMAT_R8_UINT:                                     return "VK_FORMAT_R8_UINT";
    case VK_FORMAT_R8_SINT:                                     return "VK_FORMAT_R8_SINT";
    case VK_FORMAT_R8_SRGB:                                     return "VK_FORMAT_R8_SRGB";
    case VK_FORMAT_R8G8_UNORM:                                  return "VK_FORMAT_R8G8_UNORM";
    case VK_FORMAT_R8G8_SNORM:                                  return "VK_FORMAT_R8G8_SNORM";
    case VK_FORMAT_R8G8_USCALED:                                return "VK_FORMAT_R8G8_USCALED";
    case VK_FORMAT_R8G8_SSCALED:                                return "VK_FORMAT_R8G8_SSCALED";
    case VK_FORMAT_R8G8_UINT:                                   return "VK_FORMAT_R8G8_UINT";
    case VK_FORMAT_R8G8_SINT:                                   return "VK_FORMAT_R8G8_SINT";
    case VK_FORMAT_R8G8_SRGB:                                   return "VK_FORMAT_R8G8_SRGB";
    case VK_FORMAT_R8G8B8_UNORM:                                return "VK_FORMAT_R8G8B8_UNORM";
    case VK_FORMAT_R8G8B8_SNORM:                                return "VK_FORMAT_R8G8B8_SNORM";
    case VK_FORMAT_R8G8B8_USCALED:                              return "VK_FORMAT_R8G8B8_USCALED";
    case VK_FORMAT_R8G8B8_SSCALED:                              return "VK_FORMAT_R8G8B8_SSCALED";
    case VK_FORMAT_R8G8B8_UINT:                                 return "VK_FORMAT_R8G8B8_UINT";
    case VK_FORMAT_R8G8B8_SINT:                                 return "VK_FORMAT_R8G8B8_SINT";
    case VK_FORMAT_R8G8B8_SRGB:                                 return "VK_FORMAT_R8G8B8_SRGB";
    case VK_FORMAT_B8G8R8_UNORM:                                return "VK_FORMAT_B8G8R8_UNORM";
    case VK_FORMAT_B8G8R8_SNORM:                                return "VK_FORMAT_B8G8R8_SNORM";
    case VK_FORMAT_B8G8R8_USCALED:                              return "VK_FORMAT_B8G8R8_USCALED";
    case VK_FORMAT_B8G8R8_SSCALED:                              return "VK_FORMAT_B8G8R8_SSCALED";
    case VK_FORMAT_B8G8R8_UINT:                                 return "VK_FORMAT_B8G8R8_UINT";
    case VK_FORMAT_B8G8R8_SINT:                                 return "VK_FORMAT_B8G8R8_SINT";
    case VK_FORMAT_B8G8R8_SRGB:                                 return "VK_FORMAT_B8G8R8_SRGB";
    case VK_FORMAT_R8G8B8A8_UNORM:                              return "VK_FORMAT_R8G8B8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SNORM:                              return "VK_FORMAT_R8G8B8A8_SNORM";
    case VK_FORMAT_R8G8B8A8_USCALED:                            return "VK_FORMAT_R8G8B8A8_USCALED";
    case VK_FORMAT_R8G8B8A8_SSCALED:                            return "VK_FORMAT_R8G8B8A8_SSCALED";
    case VK_FORMAT_R8G8B8A8_UINT:                               return "VK_FORMAT_R8G8B8A8_UINT";
    case VK_FORMAT_R8G8B8A8_SINT:                               return "VK_FORMAT_R8G8B8A8_SINT";
    case VK_FORMAT_R8G8B8A8_SRGB:                               return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_UNORM:                              return "VK_FORMAT_B8G8R8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_SNORM:                              return "VK_FORMAT_B8G8R8A8_SNORM";
    case VK_FORMAT_B8G8R8A8_USCALED:                            return "VK_FORMAT_B8G8R8A8_USCALED";
    case VK_FORMAT_B8G8R8A8_SSCALED:                            return "VK_FORMAT_B8G8R8A8_SSCALED";
    case VK_FORMAT_B8G8R8A8_UINT:                               return "VK_FORMAT_B8G8R8A8_UINT";
    case VK_FORMAT_B8G8R8A8_SINT:                               return "VK_FORMAT_B8G8R8A8_SINT";
    case VK_FORMAT_B8G8R8A8_SRGB:                               return "VK_FORMAT_B8G8R8A8_SRGB";
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:                       return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:                       return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:                     return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:                     return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:                        return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:                        return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:                        return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:                    return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:                    return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:                  return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:                  return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:                     return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:                     return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:                    return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:                    return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:                  return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:                  return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:                     return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:                     return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
    case VK_FORMAT_R16_UNORM:                                   return "VK_FORMAT_R16_UNORM";
    case VK_FORMAT_R16_SNORM:                                   return "VK_FORMAT_R16_SNORM";
    case VK_FORMAT_R16_USCALED:                                 return "VK_FORMAT_R16_USCALED";
    case VK_FORMAT_R16_SSCALED:                                 return "VK_FORMAT_R16_SSCALED";
    case VK_FORMAT_R16_UINT:                                    return "VK_FORMAT_R16_UINT";
    case VK_FORMAT_R16_SINT:                                    return "VK_FORMAT_R16_SINT";
    case VK_FORMAT_R16_SFLOAT:                                  return "VK_FORMAT_R16_SFLOAT";
    case VK_FORMAT_R16G16_UNORM:                                return "VK_FORMAT_R16G16_UNORM";
    case VK_FORMAT_R16G16_SNORM:                                return "VK_FORMAT_R16G16_SNORM";
    case VK_FORMAT_R16G16_USCALED:                              return "VK_FORMAT_R16G16_USCALED";
    case VK_FORMAT_R16G16_SSCALED:                              return "VK_FORMAT_R16G16_SSCALED";
    case VK_FORMAT_R16G16_UINT:                                 return "VK_FORMAT_R16G16_UINT";
    case VK_FORMAT_R16G16_SINT:                                 return "VK_FORMAT_R16G16_SINT";
    case VK_FORMAT_R16G16_SFLOAT:                               return "VK_FORMAT_R16G16_SFLOAT";
    case VK_FORMAT_R16G16B16_UNORM:                             return "VK_FORMAT_R16G16B16_UNORM";
    case VK_FORMAT_R16G16B16_SNORM:                             return "VK_FORMAT_R16G16B16_SNORM";
    case VK_FORMAT_R16G16B16_USCALED:                           return "VK_FORMAT_R16G16B16_USCALED";
    case VK_FORMAT_R16G16B16_SSCALED:                           return "VK_FORMAT_R16G16B16_SSCALED";
    case VK_FORMAT_R16G16B16_UINT:                              return "VK_FORMAT_R16G16B16_UINT";
    case VK_FORMAT_R16G16B16_SINT:                              return "VK_FORMAT_R16G16B16_SINT";
    case VK_FORMAT_R16G16B16_SFLOAT:                            return "VK_FORMAT_R16G16B16_SFLOAT";
    case VK_FORMAT_R16G16B16A16_UNORM:                          return "VK_FORMAT_R16G16B16A16_UNORM";
    case VK_FORMAT_R16G16B16A16_SNORM:                          return "VK_FORMAT_R16G16B16A16_SNORM";
    case VK_FORMAT_R16G16B16A16_USCALED:                        return "VK_FORMAT_R16G16B16A16_USCALED";
    case VK_FORMAT_R16G16B16A16_SSCALED:                        return "VK_FORMAT_R16G16B16A16_SSCALED";
    case VK_FORMAT_R16G16B16A16_UINT:                           return "VK_FORMAT_R16G16B16A16_UINT";
    case VK_FORMAT_R16G16B16A16_SINT:                           return "VK_FORMAT_R16G16B16A16_SINT";
    case VK_FORMAT_R16G16B16A16_SFLOAT:                         return "VK_FORMAT_R16G16B16A16_SFLOAT";
    case VK_FORMAT_R32_UINT:                                    return "VK_FORMAT_R32_UINT";
    case VK_FORMAT_R32_SINT:                                    return "VK_FORMAT_R32_SINT";
    case VK_FORMAT_R32_SFLOAT:                                  return "VK_FORMAT_R32_SFLOAT";
    case VK_FORMAT_R32G32_UINT:                                 return "VK_FORMAT_R32G32_UINT";
    case VK_FORMAT_R32G32_SINT:                                 return "VK_FORMAT_R32G32_SINT";
    case VK_FORMAT_R32G32_SFLOAT:                               return "VK_FORMAT_R32G32_SFLOAT";
    case VK_FORMAT_R32G32B32_UINT:                              return "VK_FORMAT_R32G32B32_UINT";
    case VK_FORMAT_R32G32B32_SINT:                              return "VK_FORMAT_R32G32B32_SINT";
    case VK_FORMAT_R32G32B32_SFLOAT:                            return "VK_FORMAT_R32G32B32_SFLOAT";
    case VK_FORMAT_R32G32B32A32_UINT:                           return "VK_FORMAT_R32G32B32A32_UINT";
    case VK_FORMAT_R32G32B32A32_SINT:                           return "VK_FORMAT_R32G32B32A32_SINT";
    case VK_FORMAT_R32G32B32A32_SFLOAT:                         return "VK_FORMAT_R32G32B32A32_SFLOAT";
    case VK_FORMAT_R64_UINT:                                    return "VK_FORMAT_R64_UINT";
    case VK_FORMAT_R64_SINT:                                    return "VK_FORMAT_R64_SINT";
    case VK_FORMAT_R64_SFLOAT:                                  return "VK_FORMAT_R64_SFLOAT";
    case VK_FORMAT_R64G64_UINT:                                 return "VK_FORMAT_R64G64_UINT";
    case VK_FORMAT_R64G64_SINT:                                 return "VK_FORMAT_R64G64_SINT";
    case VK_FORMAT_R64G64_SFLOAT:                               return "VK_FORMAT_R64G64_SFLOAT";
    case VK_FORMAT_R64G64B64_UINT:                              return "VK_FORMAT_R64G64B64_UINT";
    case VK_FORMAT_R64G64B64_SINT:                              return "VK_FORMAT_R64G64B64_SINT";
    case VK_FORMAT_R64G64B64_SFLOAT:                            return "VK_FORMAT_R64G64B64_SFLOAT";
    case VK_FORMAT_R64G64B64A64_UINT:                           return "VK_FORMAT_R64G64B64A64_UINT";
    case VK_FORMAT_R64G64B64A64_SINT:                           return "VK_FORMAT_R64G64B64A64_SINT";
    case VK_FORMAT_R64G64B64A64_SFLOAT:                         return "VK_FORMAT_R64G64B64A64_SFLOAT";
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:                     return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:                      return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
    case VK_FORMAT_D16_UNORM:                                   return "VK_FORMAT_D16_UNORM";
    case VK_FORMAT_X8_D24_UNORM_PACK32:                         return "VK_FORMAT_X8_D24_UNORM_PACK32";
    case VK_FORMAT_D32_SFLOAT:                                  return "VK_FORMAT_D32_SFLOAT";
    case VK_FORMAT_S8_UINT:                                     return "VK_FORMAT_S8_UINT";
    case VK_FORMAT_D16_UNORM_S8_UINT:                           return "VK_FORMAT_D16_UNORM_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT:                           return "VK_FORMAT_D24_UNORM_S8_UINT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT:                          return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:                         return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:                          return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:                        return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:                         return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
    case VK_FORMAT_BC2_UNORM_BLOCK:                             return "VK_FORMAT_BC2_UNORM_BLOCK";
    case VK_FORMAT_BC2_SRGB_BLOCK:                              return "VK_FORMAT_BC2_SRGB_BLOCK";
    case VK_FORMAT_BC3_UNORM_BLOCK:                             return "VK_FORMAT_BC3_UNORM_BLOCK";
    case VK_FORMAT_BC3_SRGB_BLOCK:                              return "VK_FORMAT_BC3_SRGB_BLOCK";
    case VK_FORMAT_BC4_UNORM_BLOCK:                             return "VK_FORMAT_BC4_UNORM_BLOCK";
    case VK_FORMAT_BC4_SNORM_BLOCK:                             return "VK_FORMAT_BC4_SNORM_BLOCK";
    case VK_FORMAT_BC5_UNORM_BLOCK:                             return "VK_FORMAT_BC5_UNORM_BLOCK";
    case VK_FORMAT_BC5_SNORM_BLOCK:                             return "VK_FORMAT_BC5_SNORM_BLOCK";
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:                           return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:                           return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
    case VK_FORMAT_BC7_UNORM_BLOCK:                             return "VK_FORMAT_BC7_UNORM_BLOCK";
    case VK_FORMAT_BC7_SRGB_BLOCK:                              return "VK_FORMAT_BC7_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:                     return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:                      return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:                   return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:                    return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:                   return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:                    return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:                         return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:                         return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:                      return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:                      return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:                        return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:                         return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:                       return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:                        return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:                      return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:                       return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
    case VK_FORMAT_G8B8G8R8_422_UNORM:                          return "VK_FORMAT_G8B8G8R8_422_UNORM";
    case VK_FORMAT_B8G8R8G8_422_UNORM:                          return "VK_FORMAT_B8G8R8G8_422_UNORM";
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:                    return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:                    return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:                   return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
    case VK_FORMAT_R10X6_UNORM_PACK16:                          return "VK_FORMAT_R10X6_UNORM_PACK16";
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:                    return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:          return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:      return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:      return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:   return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:   return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:  return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
    case VK_FORMAT_R12X4_UNORM_PACK16:                          return "VK_FORMAT_R12X4_UNORM_PACK16";
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:                    return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:          return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:      return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:      return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:   return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:   return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:  return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
    case VK_FORMAT_G16B16G16R16_422_UNORM:                      return "VK_FORMAT_G16B16G16R16_422_UNORM";
    case VK_FORMAT_B16G16R16G16_422_UNORM:                      return "VK_FORMAT_B16G16R16G16_422_UNORM";
    case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
    case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:                 return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:                 return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
    case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:                return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:                 return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:                  return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
    case VK_FORMAT_UNDEFINED:
    default:                                                    return "VK_FORMAT_UNDEFINED";
    }
}

inline const char* ToString(VkPresentModeKHR PresentMode)
{
    switch (PresentMode)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return "VK_PRESENT_MODE_IMMEDIATE_KHR";
    case VK_PRESENT_MODE_MAILBOX_KHR:                   return "VK_PRESENT_MODE_MAILBOX_KHR";
    case VK_PRESENT_MODE_FIFO_KHR:                      return "VK_PRESENT_MODE_FIFO_KHR";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
    case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
    case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
    default:                                            return "Unknown VkPresentModeKHR";
    }
}