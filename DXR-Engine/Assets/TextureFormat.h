#pragma once
#include "Core/Types.h"

enum class EFormat : uint32
{
    Unknown = 0,
    R32G32B32A32_Typeless = 1,
    R32G32B32A32_Float = 2,
    R32G32B32A32_Uint = 3,
    R32G32B32A32_Sint = 4,
    R32G32B32_Typeless = 5,
    R32G32B32_Float = 6,
    R32G32B32_Uint = 7,
    R32G32B32_Sint = 8,
    R16G16B16A16_Typeless = 9,
    R16G16B16A16_Float = 10,
    R16G16B16A16_Unorm = 11,
    R16G16B16A16_Uint = 12,
    R16G16B16A16_Snorm = 13,
    R16G16B16A16_Sint = 14,
    R32G32_Typeless = 15,
    R32G32_Float = 16,
    R32G32_Uint = 17,
    R32G32_Sint = 18,
    R10G10B10A2_Typeless = 23,
    R10G10B10A2_Unorm = 24,
    R10G10B10A2_Uint = 25,
    R11G11B10_Float = 26,
    R8G8B8A8_Typeless = 27,
    R8G8B8A8_Unorm = 28,
    R8G8B8A8_Unorm_SRGB = 29,
    R8G8B8A8_Uint = 30,
    R8G8B8A8_Snorm = 31,
    R8G8B8A8_Sint = 32,
    R16G16_Typeless = 33,
    R16G16_Float = 34,
    R16G16_Unorm = 35,
    R16G16_Uint = 36,
    R16G16_Snorm = 37,
    R16G16_Sint = 38,
    R32_Typeless = 39,
    D32_Float = 40,
    R32_Float = 41,
    R32_Uint = 42,
    R32_Sint = 43,
    R24G8_Typeless = 44,
    D24_Unorm_S8_Uint = 45,
    R24_Unorm_X8_Typeless = 46,
    X24_Typeless_G8_Uint = 47,
    R8G8_Typeless = 48,
    R8G8_Unorm = 49,
    R8G8_Uint = 50,
    R8G8_Snorm = 51,
    R8G8_Sint = 52,
    R16_Typeless = 53,
    R16_Float = 54,
    D16_Unorm = 55,
    R16_Unorm = 56,
    R16_Uint = 57,
    R16_Snorm = 58,
    R16_Sint = 59,
    R8_Typeless = 60,
    R8_Unorm = 61,
    R8_Uint = 62,
    R8_Snorm = 63,
    R8_Sint = 64,
};

inline const char* ToString( EFormat Format )
{
    switch ( Format )
    {
    case EFormat::R32G32B32A32_Typeless:    return "R32G32B32A32_Typeless";
    case EFormat::R32G32B32A32_Float:       return "R32G32B32A32_Float";
    case EFormat::R32G32B32A32_Uint:        return "R32G32B32A32_Uint";
    case EFormat::R32G32B32A32_Sint:        return "R32G32B32A32_Sint";
    case EFormat::R32G32B32_Typeless:       return "R32G32B32_Typeless";
    case EFormat::R32G32B32_Float:          return "R32G32B32_Float";
    case EFormat::R32G32B32_Uint:           return "R32G32B32_Uint";
    case EFormat::R32G32B32_Sint:           return "R32G32B32_Sint";
    case EFormat::R16G16B16A16_Typeless:    return "R16G16B16A16_Typeless";
    case EFormat::R16G16B16A16_Float:       return "R16G16B16A16_Float";
    case EFormat::R16G16B16A16_Unorm:       return "R16G16B16A16_Unorm";
    case EFormat::R16G16B16A16_Uint:        return "R16G16B16A16_Uint";
    case EFormat::R16G16B16A16_Snorm:       return "R16G16B16A16_Snorm";
    case EFormat::R16G16B16A16_Sint:        return "R16G16B16A16_Sint";
    case EFormat::R32G32_Typeless:          return "R32G32_Typeless";
    case EFormat::R32G32_Float:             return "R32G32_Float";
    case EFormat::R32G32_Uint:              return "R32G32_Uint";
    case EFormat::R32G32_Sint:              return "R32G32_Sint";
    case EFormat::R10G10B10A2_Typeless:     return "R10G10B10A2_Typeless";
    case EFormat::R10G10B10A2_Unorm:        return "R10G10B10A2_Unorm";
    case EFormat::R10G10B10A2_Uint:         return "R10G10B10A2_Uint";
    case EFormat::R11G11B10_Float:          return "R11G11B10_Float";
    case EFormat::R8G8B8A8_Typeless:        return "R8G8B8A8_Typeless";
    case EFormat::R8G8B8A8_Unorm:           return "R8G8B8A8_Unorm";
    case EFormat::R8G8B8A8_Unorm_SRGB:      return "R8G8B8A8_Unorm_SRGB";
    case EFormat::R8G8B8A8_Uint:            return "R8G8B8A8_Uint";
    case EFormat::R8G8B8A8_Snorm:           return "R8G8B8A8_Snorm";
    case EFormat::R8G8B8A8_Sint:            return "R8G8B8A8_Sint";
    case EFormat::R16G16_Typeless:          return "R16G16_Typeless";
    case EFormat::R16G16_Float:             return "R16G16_Float";
    case EFormat::R16G16_Unorm:             return "R16G16_Unorm";
    case EFormat::R16G16_Uint:              return "R16G16_Uint";
    case EFormat::R16G16_Snorm:             return "R16G16_Snorm";
    case EFormat::R16G16_Sint:              return "R16G16_Sint";
    case EFormat::R32_Typeless:             return "R32_Typeless";
    case EFormat::D32_Float:                return "D32_Float";
    case EFormat::R32_Float:                return "R32_Float";
    case EFormat::R32_Uint:                 return "R32_Uint";
    case EFormat::R32_Sint:                 return "R32_Sint";
    case EFormat::R24G8_Typeless:           return "R24G8_Typeless";
    case EFormat::D24_Unorm_S8_Uint:        return "D24_Unorm_S8_Uint";
    case EFormat::R24_Unorm_X8_Typeless:    return "R24_Unorm_X8_Typeless";
    case EFormat::X24_Typeless_G8_Uint:     return "X24_Typeless_G8_Uint";
    case EFormat::R8G8_Typeless:            return "R8G8_Typeless";
    case EFormat::R8G8_Unorm:               return "R8G8_Unorm";
    case EFormat::R8G8_Uint:                return "R8G8_Uint";
    case EFormat::R8G8_Snorm:               return "R8G8_Snorm";
    case EFormat::R8G8_Sint:                return "R8G8_Sint";
    case EFormat::R16_Typeless:             return "R16_Typeless";
    case EFormat::R16_Float:                return "R16_Float";
    case EFormat::D16_Unorm:                return "D16_Unorm";
    case EFormat::R16_Unorm:                return "R16_Unorm";
    case EFormat::R16_Uint:                 return "R16_Uint";
    case EFormat::R16_Snorm:                return "R16_Snorm";
    case EFormat::R16_Sint:                 return "R16_Sint";
    case EFormat::R8_Typeless:              return "R8_Typeless";
    case EFormat::R8_Unorm:                 return "R8_Unorm";
    case EFormat::R8_Uint:                  return "R8_Uint";
    case EFormat::R8_Snorm:                 return "R8_Snorm";
    case EFormat::R8_Sint:                  return "R8_Sint";
    default: return "Unknown";
    }
}

inline uint32 GetByteStrideFromFormat( EFormat Format )
{
    switch ( Format )
    {
    case EFormat::R32G32B32A32_Typeless:
    case EFormat::R32G32B32A32_Float:
    case EFormat::R32G32B32A32_Uint:
    case EFormat::R32G32B32A32_Sint:
    {
        return 16;
    }

    case EFormat::R32G32B32_Typeless:
    case EFormat::R32G32B32_Float:
    case EFormat::R32G32B32_Uint:
    case EFormat::R32G32B32_Sint:
    {
        return 12;
    }

    case EFormat::R16G16B16A16_Typeless:
    case EFormat::R16G16B16A16_Float:
    case EFormat::R16G16B16A16_Unorm:
    case EFormat::R16G16B16A16_Uint:
    case EFormat::R16G16B16A16_Snorm:
    case EFormat::R16G16B16A16_Sint:
    case EFormat::R32G32_Typeless:
    case EFormat::R32G32_Float:
    case EFormat::R32G32_Uint:
    case EFormat::R32G32_Sint:
    {
        return 8;
    }

    case EFormat::R10G10B10A2_Typeless:
    case EFormat::R10G10B10A2_Unorm:
    case EFormat::R10G10B10A2_Uint:
    case EFormat::R11G11B10_Float:
    case EFormat::R8G8B8A8_Typeless:
    case EFormat::R8G8B8A8_Unorm:
    case EFormat::R8G8B8A8_Unorm_SRGB:
    case EFormat::R8G8B8A8_Uint:
    case EFormat::R8G8B8A8_Snorm:
    case EFormat::R8G8B8A8_Sint:
    case EFormat::R16G16_Typeless:
    case EFormat::R16G16_Float:
    case EFormat::R16G16_Unorm:
    case EFormat::R16G16_Uint:
    case EFormat::R16G16_Snorm:
    case EFormat::R16G16_Sint:
    case EFormat::R32_Typeless:
    case EFormat::D32_Float:
    case EFormat::R32_Float:
    case EFormat::R32_Uint:
    case EFormat::R32_Sint:
    case EFormat::R24G8_Typeless:
    case EFormat::D24_Unorm_S8_Uint:
    case EFormat::R24_Unorm_X8_Typeless:
    case EFormat::X24_Typeless_G8_Uint:
    {
        return 4;
    }

    case EFormat::R8G8_Typeless:
    case EFormat::R8G8_Unorm:
    case EFormat::R8G8_Uint:
    case EFormat::R8G8_Snorm:
    case EFormat::R8G8_Sint:
    case EFormat::R16_Typeless:
    case EFormat::R16_Float:
    case EFormat::D16_Unorm:
    case EFormat::R16_Unorm:
    case EFormat::R16_Uint:
    case EFormat::R16_Snorm:
    case EFormat::R16_Sint:
    {
        return 2;
    }

    case EFormat::R8_Typeless:
    case EFormat::R8_Unorm:
    case EFormat::R8_Uint:
    case EFormat::R8_Snorm:
    case EFormat::R8_Sint:
    {
        return 1;
    }

    default:
    {
        return 0;
    }
    }
}