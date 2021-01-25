#pragma once
#include "Core.h"

enum class EFormat
{
    Format_Unknown                    = 0,
    Format_R32G32B32A32_Typeless    = 1,
    Format_R32G32B32A32_Float        = 2,
    Format_R32G32B32A32_Uint        = 3,
    Format_R32G32B32A32_Sint        = 4,
    Format_R32G32B32_Typeless        = 5,
    Format_R32G32B32_Float            = 6,
    Format_R32G32B32_Uint            = 7,
    Format_R32G32B32_Sint            = 8,
    Format_R16G16B16A16_Typeless    = 9,
    Format_R16G16B16A16_Float        = 10,
    Format_R16G16B16A16_Unorm        = 11,
    Format_R16G16B16A16_Uint        = 12,
    Format_R16G16B16A16_Snorm        = 13,
    Format_R16G16B16A16_Sint        = 14,
    Format_R32G32_Typeless            = 15,
    Format_R32G32_Float                = 16,
    Format_R32G32_Uint                = 17,
    Format_R32G32_Sint                = 18,
    Format_R32G8X24_Typeless        = 19,
    Format_D32_Float_S8X24_Uint        = 20,
    Format_R32_Float_X8X24_Typeless    = 21,
    Format_X32_Typeless_G8X24_Uint    = 22,
    Format_R10G10B10A2_Typeless        = 23,
    Format_R10G10B10A2_Unorm        = 24,
    Format_R10G10B10A2_Uint            = 25,
    Format_R11G11B10_Float            = 26,
    Format_R8G8B8A8_Typeless        = 27,
    Format_R8G8B8A8_Unorm            = 28,
    Format_R8G8B8A8_Unorm_SRGB        = 29,
    Format_R8G8B8A8_Uint            = 30,
    Format_R8G8B8A8_Snorm            = 31,
    Format_R8G8B8A8_Sint            = 32,
    Format_R16G16_Typeless            = 33,
    Format_R16G16_Float                = 34,
    Format_R16G16_Unorm                = 35,
    Format_R16G16_Uint                = 36,
    Format_R16G16_Snorm                = 37,
    Format_R16G16_Sint                = 38,
    Format_R32_Typeless                = 39,
    Format_D32_Float                = 40,
    Format_R32_Float                = 41,
    Format_R32_Uint                    = 42,
    Format_R32_Sint                    = 43,
    Format_R24G8_Typeless            = 44,
    Format_D24_Unorm_S8_Uint        = 45,
    Format_R24_Unorm_X8_Typeless    = 46,
    Format_X24_Typeless_G8_Uint        = 47,
    Format_R8G8_Typeless            = 48,
    Format_R8G8_Unorm                = 49,
    Format_R8G8_Uint                = 50,
    Format_R8G8_Snorm                = 51,
    Format_R8G8_Sint                = 52,
    Format_R16_Typeless                = 53,
    Format_R16_Float                = 54,
    Format_D16_Unorm                = 55,
    Format_R16_Unorm                = 56,
    Format_R16_Uint                    = 57,
    Format_R16_Snorm                = 58,
    Format_R16_Sint                    = 59,
    Format_R8_Typeless                = 60,
    Format_R8_Unorm                    = 61,
    Format_R8_Uint                    = 62,
    Format_R8_Snorm                    = 63,
    Format_R8_Sint                    = 64,
    Format_A8_Unorm                    = 65,
    Format_R1_Unorm                    = 66,
    Format_B5G6R5_Unorm                = 85,
    Format_B5G5R5A1_Unorm            = 86,
    Format_B8G8R8A8_Unorm            = 87,
    Format_B8G8R8X8_Unorm            = 88,
    Format_B8G8R8A8_Typeless        = 90,
    Format_B8G8R8A8_Unorm_SRGB        = 91,
    Format_B8G8R8X8_Typeless        = 92,
    Format_B8G8R8X8_Unorm_SRGB        = 93,
};

inline const Char* ToString(EFormat Format)
{
    switch (Format)
    {
    case EFormat::Format_R32G32B32A32_Typeless:        return "Format_R32G32B32A32_Typeless";
    case EFormat::Format_R32G32B32A32_Float:        return "Format_R32G32B32A32_Float";
    case EFormat::Format_R32G32B32A32_Uint:            return "Format_R32G32B32A32_Uint";
    case EFormat::Format_R32G32B32A32_Sint:            return "Format_R32G32B32A32_Sint";
    case EFormat::Format_R32G32B32_Typeless:        return "Format_R32G32B32_Typeless";
    case EFormat::Format_R32G32B32_Float:            return "Format_R32G32B32_Float";
    case EFormat::Format_R32G32B32_Uint:            return "Format_R32G32B32_Uint";
    case EFormat::Format_R32G32B32_Sint:            return "Format_R32G32B32_Sint";
    case EFormat::Format_R16G16B16A16_Typeless:        return "Format_R16G16B16A16_Typeless";
    case EFormat::Format_R16G16B16A16_Float:        return "Format_R16G16B16A16_Float";
    case EFormat::Format_R16G16B16A16_Unorm:        return "Format_R16G16B16A16_Unorm";
    case EFormat::Format_R16G16B16A16_Uint:            return "Format_R16G16B16A16_Uint";
    case EFormat::Format_R16G16B16A16_Snorm:        return "Format_R16G16B16A16_Snorm";
    case EFormat::Format_R16G16B16A16_Sint:            return "Format_R16G16B16A16_Sint";
    case EFormat::Format_R32G32_Typeless:            return "Format_R32G32_Typeless";
    case EFormat::Format_R32G32_Float:                return "Format_R32G32_Float";
    case EFormat::Format_R32G32_Uint:                return "Format_R32G32_Uint";
    case EFormat::Format_R32G32_Sint:                return "Format_R32G32_Sint";
    case EFormat::Format_R32G8X24_Typeless:            return "Format_R32G8X24_Typeless";
    case EFormat::Format_D32_Float_S8X24_Uint:        return "Format_D32_Float_S8X24_Uint";
    case EFormat::Format_R32_Float_X8X24_Typeless:    return "Format_R32_Float_X8X24_Typeless";
    case EFormat::Format_X32_Typeless_G8X24_Uint:    return "Format_X32_Typeless_G8X24_Uint";
    case EFormat::Format_R10G10B10A2_Typeless:        return "Format_R10G10B10A2_Typeless";
    case EFormat::Format_R10G10B10A2_Unorm:            return "Format_R10G10B10A2_Unorm";
    case EFormat::Format_R10G10B10A2_Uint:            return "Format_R10G10B10A2_Uint";
    case EFormat::Format_R11G11B10_Float:            return "Format_R11G11B10_Float";
    case EFormat::Format_R8G8B8A8_Typeless:            return "Format_R8G8B8A8_Typeless";
    case EFormat::Format_R8G8B8A8_Unorm:            return "Format_R8G8B8A8_Unorm";
    case EFormat::Format_R8G8B8A8_Unorm_SRGB:        return "Format_R8G8B8A8_Unorm_SRGB";
    case EFormat::Format_R8G8B8A8_Uint:                return "Format_R8G8B8A8_Uint";
    case EFormat::Format_R8G8B8A8_Snorm:            return "Format_R8G8B8A8_Snorm";
    case EFormat::Format_R8G8B8A8_Sint:                return "Format_R8G8B8A8_Sint";
    case EFormat::Format_R16G16_Typeless:            return "Format_R16G16_Typeless";
    case EFormat::Format_R16G16_Float:                return "Format_R16G16_Float";
    case EFormat::Format_R16G16_Unorm:                return "Format_R16G16_Unorm";
    case EFormat::Format_R16G16_Uint:                return "Format_R16G16_Uint";
    case EFormat::Format_R16G16_Snorm:                return "Format_R16G16_Snorm";
    case EFormat::Format_R16G16_Sint:                return "Format_R16G16_Sint";
    case EFormat::Format_R32_Typeless:                return "Format_R32_Typeless";
    case EFormat::Format_D32_Float:                    return "Format_D32_Float";
    case EFormat::Format_R32_Float:                    return "Format_R32_Float";
    case EFormat::Format_R32_Uint:                    return "Format_R32_Uint";
    case EFormat::Format_R32_Sint:                    return "Format_R32_Sint";
    case EFormat::Format_R24G8_Typeless:            return "Format_R24G8_Typeless";
    case EFormat::Format_D24_Unorm_S8_Uint:            return "Format_D24_Unorm_S8_Uint";
    case EFormat::Format_R24_Unorm_X8_Typeless:        return "Format_R24_Unorm_X8_Typeless";
    case EFormat::Format_X24_Typeless_G8_Uint:        return "Format_X24_Typeless_G8_Uint";
    case EFormat::Format_R8G8_Typeless:                return "Format_R8G8_Typeless";
    case EFormat::Format_R8G8_Unorm:                return "Format_R8G8_Unorm";
    case EFormat::Format_R8G8_Uint:                    return "Format_R8G8_Uint";
    case EFormat::Format_R8G8_Snorm:                return "Format_R8G8_Snorm";
    case EFormat::Format_R8G8_Sint:                    return "Format_R8G8_Sint";
    case EFormat::Format_R16_Typeless:                return "Format_R16_Typeless";
    case EFormat::Format_R16_Float:                    return "Format_R16_Float";
    case EFormat::Format_D16_Unorm:                    return "Format_D16_Unorm";
    case EFormat::Format_R16_Unorm:                    return "Format_R16_Unorm";
    case EFormat::Format_R16_Uint:                    return "Format_R16_Uint";
    case EFormat::Format_R16_Snorm:                    return "Format_R16_Snorm";
    case EFormat::Format_R16_Sint:                    return "Format_R16_Sint";
    case EFormat::Format_R8_Typeless:                return "Format_R8_Typeless";
    case EFormat::Format_R8_Unorm:                    return "Format_R8_Unorm";
    case EFormat::Format_R8_Uint:                    return "Format_R8_Uint";
    case EFormat::Format_R8_Snorm:                    return "Format_R8_Snorm";
    case EFormat::Format_R8_Sint:                    return "Format_R8_Sint";
    case EFormat::Format_A8_Unorm:                    return "Format_A8_Unorm";
    case EFormat::Format_R1_Unorm:                    return "Format_R1_Unorm";
    case EFormat::Format_B5G6R5_Unorm:                return "Format_B5G6R5_Unorm";
    case EFormat::Format_B5G5R5A1_Unorm:            return "Format_B5G5R5A1_Unorm";
    case EFormat::Format_B8G8R8A8_Unorm:            return "Format_B8G8R8A8_Unorm";
    case EFormat::Format_B8G8R8X8_Unorm:            return "Format_B8G8R8X8_Unorm";
    case EFormat::Format_B8G8R8A8_Typeless:            return "Format_B8G8R8A8_Typeless";
    case EFormat::Format_B8G8R8A8_Unorm_SRGB:        return "Format_B8G8R8A8_Unorm_SRGB";
    case EFormat::Format_B8G8R8X8_Typeless:            return "Format_B8G8R8X8_Typeless";
    case EFormat::Format_B8G8R8X8_Unorm_SRGB:        return "Format_B8G8R8X8_Unorm_SRGB";
    default:                                        return "Format_UNKNOWN";
    }
}

inline UInt32 GetStrideFromFormat(EFormat Format)
{
    switch (Format)
    {
    case EFormat::Format_R32G32B32A32_Typeless:
    case EFormat::Format_R32G32B32A32_Float:
    case EFormat::Format_R32G32B32A32_Uint:
    case EFormat::Format_R32G32B32A32_Sint:
    {
        return 16;
    }

    case EFormat::Format_R32G32B32_Typeless:
    case EFormat::Format_R32G32B32_Float:
    case EFormat::Format_R32G32B32_Uint:
    case EFormat::Format_R32G32B32_Sint:
    {
        return 12;
    }

    case EFormat::Format_R16G16B16A16_Typeless:
    case EFormat::Format_R16G16B16A16_Float:
    case EFormat::Format_R16G16B16A16_Unorm:
    case EFormat::Format_R16G16B16A16_Uint:
    case EFormat::Format_R16G16B16A16_Snorm:
    case EFormat::Format_R16G16B16A16_Sint:
    case EFormat::Format_R32G32_Typeless:
    case EFormat::Format_R32G32_Float:
    case EFormat::Format_R32G32_Uint:
    case EFormat::Format_R32G32_Sint:
    case EFormat::Format_R32G8X24_Typeless:
    case EFormat::Format_D32_Float_S8X24_Uint:
    case EFormat::Format_R32_Float_X8X24_Typeless:
    case EFormat::Format_X32_Typeless_G8X24_Uint:
    {
        return 8;
    }

    case EFormat::Format_R10G10B10A2_Typeless:
    case EFormat::Format_R10G10B10A2_Unorm:
    case EFormat::Format_R10G10B10A2_Uint:
    case EFormat::Format_R11G11B10_Float:
    case EFormat::Format_R8G8B8A8_Typeless:
    case EFormat::Format_R8G8B8A8_Unorm:
    case EFormat::Format_R8G8B8A8_Unorm_SRGB:
    case EFormat::Format_R8G8B8A8_Uint:
    case EFormat::Format_R8G8B8A8_Snorm:
    case EFormat::Format_R8G8B8A8_Sint:
    case EFormat::Format_R16G16_Typeless:
    case EFormat::Format_R16G16_Float:
    case EFormat::Format_R16G16_Unorm:
    case EFormat::Format_R16G16_Uint:
    case EFormat::Format_R16G16_Snorm:
    case EFormat::Format_R16G16_Sint:
    case EFormat::Format_R32_Typeless:
    case EFormat::Format_D32_Float:
    case EFormat::Format_R32_Float:
    case EFormat::Format_R32_Uint:
    case EFormat::Format_R32_Sint:
    case EFormat::Format_R24G8_Typeless:
    case EFormat::Format_D24_Unorm_S8_Uint:
    case EFormat::Format_R24_Unorm_X8_Typeless:
    case EFormat::Format_X24_Typeless_G8_Uint:
    {
        return 4;
    }

    case EFormat::Format_R8G8_Typeless:
    case EFormat::Format_R8G8_Unorm:
    case EFormat::Format_R8G8_Uint:
    case EFormat::Format_R8G8_Snorm:
    case EFormat::Format_R8G8_Sint:
    case EFormat::Format_R16_Typeless:
    case EFormat::Format_R16_Float:
    case EFormat::Format_D16_Unorm:
    case EFormat::Format_R16_Unorm:
    case EFormat::Format_R16_Uint:
    case EFormat::Format_R16_Snorm:
    case EFormat::Format_R16_Sint:
    {
        return 2;
    }

    case EFormat::Format_R8_Typeless:
    case EFormat::Format_R8_Unorm:
    case EFormat::Format_R8_Uint:
    case EFormat::Format_R8_Snorm:
    case EFormat::Format_R8_Sint:
    case EFormat::Format_A8_Unorm:
    {
        return 1;
    }

    default:
    {
        return 0;
    }
    }
}