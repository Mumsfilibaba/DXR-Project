#pragma once
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIFormat

enum class ERHIFormat : uint16
{
    Unknown               = 0,
    
    R32G32B32A32_Typeless = 1,
    R32G32B32A32_Float    = 2,
    R32G32B32A32_Uint     = 3,
    R32G32B32A32_Sint     = 4,
    
    R32G32B32_Typeless    = 5,
    R32G32B32_Float       = 6,
    R32G32B32_Uint        = 7,
    R32G32B32_Sint        = 8,
    
    R16G16B16A16_Typeless = 9,
    R16G16B16A16_Float    = 10,
    R16G16B16A16_Unorm    = 11,
    R16G16B16A16_Uint     = 12,
    R16G16B16A16_Snorm    = 13,
    R16G16B16A16_Sint     = 14,
    
    R32G32_Typeless       = 15,
    R32G32_Float          = 16,
    R32G32_Uint           = 17,
    R32G32_Sint           = 18,
    
    R10G10B10A2_Typeless  = 23,
    R10G10B10A2_Unorm     = 24,
    R10G10B10A2_Uint      = 25,
    
    R11G11B10_Float       = 26,
    
    R8G8B8A8_Typeless     = 27,
    R8G8B8A8_Unorm        = 28,
    R8G8B8A8_Unorm_SRGB   = 29,
    R8G8B8A8_Uint         = 30,
    R8G8B8A8_Snorm        = 31,
    R8G8B8A8_Sint         = 32,
    B8G8R8A8_Typeless     = 33,
    B8G8R8A8_Unorm        = 34,
    B8G8R8A8_Unorm_SRGB   = 35,
    B8G8R8A8_Uint         = 36,
    B8G8R8A8_Snorm        = 37,
    B8G8R8A8_Sint         = 38,
    
    R16G16_Typeless       = 39,
    R16G16_Float          = 40,
    R16G16_Unorm          = 41,
    R16G16_Uint           = 42,
    R16G16_Snorm          = 43,
    R16G16_Sint           = 44,
    
    R32_Typeless          = 45,
    D32_Float             = 46,
    R32_Float             = 47,
    R32_Uint              = 48,
    R32_Sint              = 49,
    
    R24G8_Typeless        = 50,
    D24_Unorm_S8_Uint     = 51,
    R24_Unorm_X8_Typeless = 52,
    X24_Typeless_G8_Uint  = 53,
    
    R8G8_Typeless         = 54,
    R8G8_Unorm            = 55,
    R8G8_Uint             = 56,
    R8G8_Snorm            = 57,
    R8G8_Sint             = 58,
    
    R16_Typeless          = 59,
    R16_Float             = 60,
    D16_Unorm             = 61,
    R16_Unorm             = 62,
    R16_Uint              = 63,
    R16_Snorm             = 64,
    R16_Sint              = 65,

    R8_Typeless           = 66,
    R8_Unorm              = 67,
    R8_Uint               = 68,
    R8_Snorm              = 69,
    R8_Sint               = 70,
};

inline const char* ToString(ERHIFormat Format)
{
    switch (Format)
    {
    case ERHIFormat::R32G32B32A32_Typeless:    return "R32G32B32A32_Typeless";
    case ERHIFormat::R32G32B32A32_Float:       return "R32G32B32A32_Float";
    case ERHIFormat::R32G32B32A32_Uint:        return "R32G32B32A32_Uint";
    case ERHIFormat::R32G32B32A32_Sint:        return "R32G32B32A32_Sint";

    case ERHIFormat::R32G32B32_Typeless:       return "R32G32B32_Typeless";
    case ERHIFormat::R32G32B32_Float:          return "R32G32B32_Float";
    case ERHIFormat::R32G32B32_Uint:           return "R32G32B32_Uint";
    case ERHIFormat::R32G32B32_Sint:           return "R32G32B32_Sint";

    case ERHIFormat::R16G16B16A16_Typeless:    return "R16G16B16A16_Typeless";
    case ERHIFormat::R16G16B16A16_Float:       return "R16G16B16A16_Float";
    case ERHIFormat::R16G16B16A16_Unorm:       return "R16G16B16A16_Unorm";
    case ERHIFormat::R16G16B16A16_Uint:        return "R16G16B16A16_Uint";
    case ERHIFormat::R16G16B16A16_Snorm:       return "R16G16B16A16_Snorm";
    case ERHIFormat::R16G16B16A16_Sint:        return "R16G16B16A16_Sint";

    case ERHIFormat::R32G32_Typeless:          return "R32G32_Typeless";
    case ERHIFormat::R32G32_Float:             return "R32G32_Float";
    case ERHIFormat::R32G32_Uint:              return "R32G32_Uint";
    case ERHIFormat::R32G32_Sint:              return "R32G32_Sint";

    case ERHIFormat::R10G10B10A2_Typeless:     return "R10G10B10A2_Typeless";
    case ERHIFormat::R10G10B10A2_Unorm:        return "R10G10B10A2_Unorm";
    case ERHIFormat::R10G10B10A2_Uint:         return "R10G10B10A2_Uint";

    case ERHIFormat::R11G11B10_Float:          return "R11G11B10_Float";
    
    case ERHIFormat::R8G8B8A8_Typeless:        return "R8G8B8A8_Typeless";
    case ERHIFormat::R8G8B8A8_Unorm:           return "R8G8B8A8_Unorm";
    case ERHIFormat::R8G8B8A8_Unorm_SRGB:      return "R8G8B8A8_Unorm_SRGB";
    case ERHIFormat::R8G8B8A8_Uint:            return "R8G8B8A8_Uint";
    case ERHIFormat::R8G8B8A8_Snorm:           return "R8G8B8A8_Snorm";
    case ERHIFormat::R8G8B8A8_Sint:            return "R8G8B8A8_Sint";

    case ERHIFormat::B8G8R8A8_Typeless:        return "B8G8R8A8_Typeless";
    case ERHIFormat::B8G8R8A8_Unorm:           return "B8G8R8A8_Unorm";
    case ERHIFormat::B8G8R8A8_Unorm_SRGB:      return "B8G8R8A8_Unorm_SRGB";
    case ERHIFormat::B8G8R8A8_Uint:            return "B8G8R8A8_Uint";
    case ERHIFormat::B8G8R8A8_Snorm:           return "B8G8R8A8_Snorm";
    case ERHIFormat::B8G8R8A8_Sint:            return "B8G8R8A8_Sint";

    case ERHIFormat::R16G16_Typeless:          return "R16G16_Typeless";
    case ERHIFormat::R16G16_Float:             return "R16G16_Float";
    case ERHIFormat::R16G16_Unorm:             return "R16G16_Unorm";
    case ERHIFormat::R16G16_Uint:              return "R16G16_Uint";
    case ERHIFormat::R16G16_Snorm:             return "R16G16_Snorm";
    case ERHIFormat::R16G16_Sint:              return "R16G16_Sint";

    case ERHIFormat::R32_Typeless:             return "R32_Typeless";
    case ERHIFormat::D32_Float:                return "D32_Float";
    case ERHIFormat::R32_Float:                return "R32_Float";
    case ERHIFormat::R32_Uint:                 return "R32_Uint";
    case ERHIFormat::R32_Sint:                 return "R32_Sint";
    
    case ERHIFormat::R24G8_Typeless:           return "R24G8_Typeless";
    case ERHIFormat::D24_Unorm_S8_Uint:        return "D24_Unorm_S8_Uint";
    case ERHIFormat::R24_Unorm_X8_Typeless:    return "R24_Unorm_X8_Typeless";
    case ERHIFormat::X24_Typeless_G8_Uint:     return "X24_Typeless_G8_Uint";
    
    case ERHIFormat::R8G8_Typeless:            return "R8G8_Typeless";
    case ERHIFormat::R8G8_Unorm:               return "R8G8_Unorm";
    case ERHIFormat::R8G8_Uint:                return "R8G8_Uint";
    case ERHIFormat::R8G8_Snorm:               return "R8G8_Snorm";
    case ERHIFormat::R8G8_Sint:                return "R8G8_Sint";
    
    case ERHIFormat::R16_Typeless:             return "R16_Typeless";
    case ERHIFormat::R16_Float:                return "R16_Float";
    case ERHIFormat::D16_Unorm:                return "D16_Unorm";
    case ERHIFormat::R16_Unorm:                return "R16_Unorm";
    case ERHIFormat::R16_Uint:                 return "R16_Uint";
    case ERHIFormat::R16_Snorm:                return "R16_Snorm";
    case ERHIFormat::R16_Sint:                 return "R16_Sint";
    
    case ERHIFormat::R8_Typeless:              return "R8_Typeless";
    case ERHIFormat::R8_Unorm:                 return "R8_Unorm";
    case ERHIFormat::R8_Uint:                  return "R8_Uint";
    case ERHIFormat::R8_Snorm:                 return "R8_Snorm";
    case ERHIFormat::R8_Sint:                  return "R8_Sint";

    default:                                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline uint32 GetByteStrideFromFormat(ERHIFormat Format)
{
    switch (Format)
    {
    case ERHIFormat::R32G32B32A32_Typeless:
    case ERHIFormat::R32G32B32A32_Float:
    case ERHIFormat::R32G32B32A32_Uint:
    case ERHIFormat::R32G32B32A32_Sint:
    {
        return 16;
    }

    case ERHIFormat::R32G32B32_Typeless:
    case ERHIFormat::R32G32B32_Float:
    case ERHIFormat::R32G32B32_Uint:
    case ERHIFormat::R32G32B32_Sint:
    {
        return 12;
    }

    case ERHIFormat::R16G16B16A16_Typeless:
    case ERHIFormat::R16G16B16A16_Float:
    case ERHIFormat::R16G16B16A16_Unorm:
    case ERHIFormat::R16G16B16A16_Uint:
    case ERHIFormat::R16G16B16A16_Snorm:
    case ERHIFormat::R16G16B16A16_Sint:
    case ERHIFormat::R32G32_Typeless:
    case ERHIFormat::R32G32_Float:
    case ERHIFormat::R32G32_Uint:
    case ERHIFormat::R32G32_Sint:
    {
        return 8;
    }

    case ERHIFormat::R10G10B10A2_Typeless:
    case ERHIFormat::R10G10B10A2_Unorm:
    case ERHIFormat::R10G10B10A2_Uint:
    case ERHIFormat::R11G11B10_Float:
    case ERHIFormat::R8G8B8A8_Typeless:
    case ERHIFormat::R8G8B8A8_Unorm:
    case ERHIFormat::R8G8B8A8_Unorm_SRGB:
    case ERHIFormat::R8G8B8A8_Uint:
    case ERHIFormat::R8G8B8A8_Snorm:
    case ERHIFormat::R8G8B8A8_Sint:
    case ERHIFormat::R16G16_Typeless:
    case ERHIFormat::R16G16_Float:
    case ERHIFormat::R16G16_Unorm:
    case ERHIFormat::R16G16_Uint:
    case ERHIFormat::R16G16_Snorm:
    case ERHIFormat::R16G16_Sint:
    case ERHIFormat::R32_Typeless:
    case ERHIFormat::D32_Float:
    case ERHIFormat::R32_Float:
    case ERHIFormat::R32_Uint:
    case ERHIFormat::R32_Sint:
    case ERHIFormat::R24G8_Typeless:
    case ERHIFormat::D24_Unorm_S8_Uint:
    case ERHIFormat::R24_Unorm_X8_Typeless:
    case ERHIFormat::X24_Typeless_G8_Uint:
    {
        return 4;
    }

    case ERHIFormat::R8G8_Typeless:
    case ERHIFormat::R8G8_Unorm:
    case ERHIFormat::R8G8_Uint:
    case ERHIFormat::R8G8_Snorm:
    case ERHIFormat::R8G8_Sint:
    case ERHIFormat::R16_Typeless:
    case ERHIFormat::R16_Float:
    case ERHIFormat::D16_Unorm:
    case ERHIFormat::R16_Unorm:
    case ERHIFormat::R16_Uint:
    case ERHIFormat::R16_Snorm:
    case ERHIFormat::R16_Sint:
    {
        return 2;
    }

    case ERHIFormat::R8_Typeless:
    case ERHIFormat::R8_Unorm:
    case ERHIFormat::R8_Uint:
    case ERHIFormat::R8_Snorm:
    case ERHIFormat::R8_Sint:
    {
        return 1;
    }

    default:
    {
        return 0;
    }
    }
}