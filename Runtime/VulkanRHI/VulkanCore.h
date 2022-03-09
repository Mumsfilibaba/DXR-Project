#pragma once
#include "Core/Core.h"
#include "Core/Debug/Debug.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/String.h"

#include "RHI/RHITypes.h"
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
// Vulkan Error

#if !PRODUCTION_BUILD
#define VULKAN_ERROR_ALWAYS(ErrorMessage)                         \
    do                                                            \
    {                                                             \
        LOG_ERROR(String("[VulkanRHI] ") + String(ErrorMessage)); \
        CDebug::DebugBreak();                                     \
    } while (false)

#define VULKAN_ERROR(Condition, ErrorMessage)  \
    do                                         \
    {                                          \
        if (!(Condition))                      \
        {                                      \
            VULKAN_ERROR_ALWAYS(ErrorMessage); \
        }                                      \
    } while (false)

#define VULKAN_WARNING(Message)                                \
    do                                                         \
    {                                                          \
        LOG_WARNING(String("[VulkanRHI] ") + String(Message)); \
    } while (false)

#define VULKAN_INFO(Message)                                \
    do                                                      \
    {                                                       \
        LOG_INFO(String("[VulkanRHI] ") + String(Message)); \
    } while (false)

#else
#define VULKAN_ERROR_ALWAYS(ErrorString)    do {} while(false)
#define VULKAN_ERROR(Condtion, ErrorString) do {} while(false)
#define VULKAN_WARNING(Message)             do {} while(false)
#endif

#ifndef VULKAN_SUCCEEDED
    #define VULKAN_SUCCEEDED(Result) (Result == VK_SUCCESS)
#endif

#ifndef VULKAN_FAILED
    #define VULKAN_FAILED(Result) (Result != VK_SUCCESS)
#endif

#ifndef VULKAN_CHECK_RESULT
    #define VULKAN_CHECK_RESULT(Result, ErrorMessage)     \
        do                                                \
        {                                                 \
            if (VULKAN_FAILED(Result))                    \
            {                                             \
                VULKAN_ERROR_ALWAYS(ErrorMessage);        \
                return false;                             \
            }                                             \
        } while(false)
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

inline const char* ToString(VkResult Result)
{
    switch (Result)
    {
    case VK_SUCCESS:                                            return "VK_SUCCESS";
    case VK_NOT_READY:                                          return "VK_NOT_READY";
    case VK_TIMEOUT:                                            return "VK_TIMEOUT";
    case VK_EVENT_SET:                                          return "VK_EVENT_SET";
    case VK_EVENT_RESET:                                        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:                                         return "VK_INCOMPLETE";
    case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:                return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
    case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:                        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:                                  return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:                            return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:                            return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:                          return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:                             return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:                              return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:                                     return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:                              return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:                            return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_FRAGMENTATION:                                return "VK_ERROR_FRAGMENTATION_EXT";
    case VK_ERROR_NOT_PERMITTED_EXT:                            return "VK_ERROR_NOT_PERMITTED_EXT";
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:                   return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:                                    return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:                                    return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:                             return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:                         return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_UNKNOWN:
    default:                                                    return "VK_ERROR_UNKNOWN";
    }
}

inline const char* GetVkErrorString(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:                                            return "Command successfully completed";
    case VK_NOT_READY:                                          return "A fence or query has not yet completed";
    case VK_TIMEOUT:                                            return "A wait operation has not completed in the specified time";
    case VK_EVENT_SET:                                          return "An event is signaled";
    case VK_EVENT_RESET:                                        return "An event is unsignaled";
    case VK_INCOMPLETE:                                         return "A return array was too small for the result";
    case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:                return "A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";
    case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "A host memory allocation has failed.";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "A device memory allocation has failed.";
    case VK_ERROR_INITIALIZATION_FAILED:                        return "Initialization of an object could not be completed for implementation-specific reasons.";
    case VK_ERROR_DEVICE_LOST:                                  return "The logical or physical device has been lost.";
    case VK_ERROR_MEMORY_MAP_FAILED:                            return "Mapping of a memory object has failed.";
    case VK_ERROR_LAYER_NOT_PRESENT:                            return "A requested layer is not present or could not be loaded.";
    case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "A requested extension is not supported.";
    case VK_ERROR_FEATURE_NOT_PRESENT:                          return "A requested feature is not supported.";
    case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
    case VK_ERROR_TOO_MANY_OBJECTS:                             return "Too many objects of the type have already been created.";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "A requested format is not supported on this device.";
    case VK_ERROR_FRAGMENTED_POOL:                              return "A descriptor pool creation has failed due to fragmentation.";
    case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "A pool memory allocation has failed. ";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:                                     return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
    case VK_ERROR_OUT_OF_DATE_KHR:                              return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
    case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:                            return "One or more shaders failed to compile or link. ";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_FRAGMENTATION:                                return "A descriptor pool creation has failed due to fragmentation.";
    case VK_ERROR_NOT_PERMITTED_EXT:                            return "VK_ERROR_NOT_PERMITTED_EXT";
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:                   return "A buffer creation failed because the requested address is not available.";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the applications control.";
    case VK_THREAD_IDLE_KHR:                                    return "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
    case VK_THREAD_DONE_KHR:                                    return "A deferred operation is not complete but there is no work remaining to assign to additional threads.";
    case VK_OPERATION_DEFERRED_KHR:                             return "A deferred operation was requested and at least some of the work was deferred.";
    case VK_OPERATION_NOT_DEFERRED_KHR:                         return "A deferred operation was requested and no operations were deferred.";
    case VK_ERROR_UNKNOWN:
    default:                                                    return "An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    }
}

inline VkImageLayout ConvertResourceStateToImageLayout(ERHIResourceState ResourceState)
{
	switch (ResourceState)
	{
		case ERHIResourceState::Common:					return VK_IMAGE_LAYOUT_GENERAL;
		case ERHIResourceState::RenderTarget:			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ERHIResourceState::RenderTargetClear:		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ERHIResourceState::UnorderedAccess:		return VK_IMAGE_LAYOUT_GENERAL;
		case ERHIResourceState::DepthClear:				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ERHIResourceState::DepthWrite:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ERHIResourceState::DepthRead:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ERHIResourceState::PixelShaderResource:    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ERHIResourceState::NonPixelShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ERHIResourceState::CopyDest:				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ERHIResourceState::CopySource:				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ERHIResourceState::ResolveDest:			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ERHIResourceState::ResolveSource:			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ERHIResourceState::Present:				return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case ERHIResourceState::ShadingRateSource:		return VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
		default:										return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}