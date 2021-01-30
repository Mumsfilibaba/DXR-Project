#pragma once
#include "Core.h"

enum class EFormat
{
    Format_Unknown                  = 0,
    Format_R32G32B32A32_Typeless    = 1,
    Format_R32G32B32A32_Float       = 2,
    Format_R32G32B32A32_Uint        = 3,
    Format_R32G32B32A32_Sint        = 4,
    Format_R32G32B32_Typeless       = 5,
    Format_R32G32B32_Float          = 6,
    Format_R32G32B32_Uint           = 7,
    Format_R32G32B32_Sint           = 8,
    Format_R16G16B16A16_Typeless    = 9,
    Format_R16G16B16A16_Float       = 10,
    Format_R16G16B16A16_Unorm       = 11,
    Format_R16G16B16A16_Uint        = 12,
    Format_R16G16B16A16_Snorm       = 13,
    Format_R16G16B16A16_Sint        = 14,
    Format_R32G32_Typeless          = 15,
    Format_R32G32_Float             = 16,
    Format_R32G32_Uint              = 17,
    Format_R32G32_Sint              = 18,
    Format_R32G8X24_Typeless        = 19,
    Format_D32_Float_S8X24_Uint     = 20,
    Format_R32_Float_X8X24_Typeless = 21,
    Format_X32_Typeless_G8X24_Uint  = 22,
    Format_R10G10B10A2_Typeless     = 23,
    Format_R10G10B10A2_Unorm        = 24,
    Format_R10G10B10A2_Uint         = 25,
    Format_R11G11B10_Float          = 26,
    Format_R8G8B8A8_Typeless        = 27,
    Format_R8G8B8A8_Unorm           = 28,
    Format_R8G8B8A8_Unorm_SRGB      = 29,
    Format_R8G8B8A8_Uint            = 30,
    Format_R8G8B8A8_Snorm           = 31,
    Format_R8G8B8A8_Sint            = 32,
    Format_R16G16_Typeless          = 33,
    Format_R16G16_Float             = 34,
    Format_R16G16_Unorm             = 35,
    Format_R16G16_Uint              = 36,
    Format_R16G16_Snorm             = 37,
    Format_R16G16_Sint              = 38,
    Format_R32_Typeless             = 39,
    Format_D32_Float                = 40,
    Format_R32_Float                = 41,
    Format_R32_Uint                 = 42,
    Format_R32_Sint                 = 43,
    Format_R24G8_Typeless           = 44,
    Format_D24_Unorm_S8_Uint        = 45,
    Format_R24_Unorm_X8_Typeless    = 46,
    Format_X24_Typeless_G8_Uint     = 47,
    Format_R8G8_Typeless            = 48,
    Format_R8G8_Unorm               = 49,
    Format_R8G8_Uint                = 50,
    Format_R8G8_Snorm               = 51,
    Format_R8G8_Sint                = 52,
    Format_R16_Typeless             = 53,
    Format_R16_Float                = 54,
    Format_D16_Unorm                = 55,
    Format_R16_Unorm                = 56,
    Format_R16_Uint                 = 57,
    Format_R16_Snorm                = 58,
    Format_R16_Sint                 = 59,
    Format_R8_Typeless              = 60,
    Format_R8_Unorm                 = 61,
    Format_R8_Uint                  = 62,
    Format_R8_Snorm                 = 63,
    Format_R8_Sint                  = 64,
    Format_A8_Unorm                 = 65,
    Format_R1_Unorm                 = 66,
    Format_B5G6R5_Unorm             = 85,
    Format_B5G5R5A1_Unorm           = 86,
    Format_B8G8R8A8_Unorm           = 87,
    Format_B8G8R8X8_Unorm           = 88,
    Format_B8G8R8A8_Typeless        = 90,
    Format_B8G8R8A8_Unorm_SRGB      = 91,
    Format_B8G8R8X8_Typeless        = 92,
    Format_B8G8R8X8_Unorm_SRGB      = 93,
};

inline const Char* ToString(EFormat Format)
{
    switch (Format)
    {
    case EFormat::Format_R32G32B32A32_Typeless:    return "Format_R32G32B32A32_Typeless";
    case EFormat::Format_R32G32B32A32_Float:       return "Format_R32G32B32A32_Float";
    case EFormat::Format_R32G32B32A32_Uint:        return "Format_R32G32B32A32_Uint";
    case EFormat::Format_R32G32B32A32_Sint:        return "Format_R32G32B32A32_Sint";
    case EFormat::Format_R32G32B32_Typeless:       return "Format_R32G32B32_Typeless";
    case EFormat::Format_R32G32B32_Float:          return "Format_R32G32B32_Float";
    case EFormat::Format_R32G32B32_Uint:           return "Format_R32G32B32_Uint";
    case EFormat::Format_R32G32B32_Sint:           return "Format_R32G32B32_Sint";
    case EFormat::Format_R16G16B16A16_Typeless:    return "Format_R16G16B16A16_Typeless";
    case EFormat::Format_R16G16B16A16_Float:       return "Format_R16G16B16A16_Float";
    case EFormat::Format_R16G16B16A16_Unorm:       return "Format_R16G16B16A16_Unorm";
    case EFormat::Format_R16G16B16A16_Uint:        return "Format_R16G16B16A16_Uint";
    case EFormat::Format_R16G16B16A16_Snorm:       return "Format_R16G16B16A16_Snorm";
    case EFormat::Format_R16G16B16A16_Sint:        return "Format_R16G16B16A16_Sint";
    case EFormat::Format_R32G32_Typeless:          return "Format_R32G32_Typeless";
    case EFormat::Format_R32G32_Float:             return "Format_R32G32_Float";
    case EFormat::Format_R32G32_Uint:              return "Format_R32G32_Uint";
    case EFormat::Format_R32G32_Sint:              return "Format_R32G32_Sint";
    case EFormat::Format_R32G8X24_Typeless:        return "Format_R32G8X24_Typeless";
    case EFormat::Format_D32_Float_S8X24_Uint:     return "Format_D32_Float_S8X24_Uint";
    case EFormat::Format_R32_Float_X8X24_Typeless: return "Format_R32_Float_X8X24_Typeless";
    case EFormat::Format_X32_Typeless_G8X24_Uint:  return "Format_X32_Typeless_G8X24_Uint";
    case EFormat::Format_R10G10B10A2_Typeless:     return "Format_R10G10B10A2_Typeless";
    case EFormat::Format_R10G10B10A2_Unorm:        return "Format_R10G10B10A2_Unorm";
    case EFormat::Format_R10G10B10A2_Uint:         return "Format_R10G10B10A2_Uint";
    case EFormat::Format_R11G11B10_Float:          return "Format_R11G11B10_Float";
    case EFormat::Format_R8G8B8A8_Typeless:        return "Format_R8G8B8A8_Typeless";
    case EFormat::Format_R8G8B8A8_Unorm:           return "Format_R8G8B8A8_Unorm";
    case EFormat::Format_R8G8B8A8_Unorm_SRGB:      return "Format_R8G8B8A8_Unorm_SRGB";
    case EFormat::Format_R8G8B8A8_Uint:            return "Format_R8G8B8A8_Uint";
    case EFormat::Format_R8G8B8A8_Snorm:           return "Format_R8G8B8A8_Snorm";
    case EFormat::Format_R8G8B8A8_Sint:            return "Format_R8G8B8A8_Sint";
    case EFormat::Format_R16G16_Typeless:          return "Format_R16G16_Typeless";
    case EFormat::Format_R16G16_Float:             return "Format_R16G16_Float";
    case EFormat::Format_R16G16_Unorm:             return "Format_R16G16_Unorm";
    case EFormat::Format_R16G16_Uint:              return "Format_R16G16_Uint";
    case EFormat::Format_R16G16_Snorm:             return "Format_R16G16_Snorm";
    case EFormat::Format_R16G16_Sint:              return "Format_R16G16_Sint";
    case EFormat::Format_R32_Typeless:             return "Format_R32_Typeless";
    case EFormat::Format_D32_Float:                return "Format_D32_Float";
    case EFormat::Format_R32_Float:                return "Format_R32_Float";
    case EFormat::Format_R32_Uint:                 return "Format_R32_Uint";
    case EFormat::Format_R32_Sint:                 return "Format_R32_Sint";
    case EFormat::Format_R24G8_Typeless:           return "Format_R24G8_Typeless";
    case EFormat::Format_D24_Unorm_S8_Uint:        return "Format_D24_Unorm_S8_Uint";
    case EFormat::Format_R24_Unorm_X8_Typeless:    return "Format_R24_Unorm_X8_Typeless";
    case EFormat::Format_X24_Typeless_G8_Uint:     return "Format_X24_Typeless_G8_Uint";
    case EFormat::Format_R8G8_Typeless:            return "Format_R8G8_Typeless";
    case EFormat::Format_R8G8_Unorm:               return "Format_R8G8_Unorm";
    case EFormat::Format_R8G8_Uint:                return "Format_R8G8_Uint";
    case EFormat::Format_R8G8_Snorm:               return "Format_R8G8_Snorm";
    case EFormat::Format_R8G8_Sint:                return "Format_R8G8_Sint";
    case EFormat::Format_R16_Typeless:             return "Format_R16_Typeless";
    case EFormat::Format_R16_Float:                return "Format_R16_Float";
    case EFormat::Format_D16_Unorm:                return "Format_D16_Unorm";
    case EFormat::Format_R16_Unorm:                return "Format_R16_Unorm";
    case EFormat::Format_R16_Uint:                 return "Format_R16_Uint";
    case EFormat::Format_R16_Snorm:                return "Format_R16_Snorm";
    case EFormat::Format_R16_Sint:                 return "Format_R16_Sint";
    case EFormat::Format_R8_Typeless:              return "Format_R8_Typeless";
    case EFormat::Format_R8_Unorm:                 return "Format_R8_Unorm";
    case EFormat::Format_R8_Uint:                  return "Format_R8_Uint";
    case EFormat::Format_R8_Snorm:                 return "Format_R8_Snorm";
    case EFormat::Format_R8_Sint:                  return "Format_R8_Sint";
    case EFormat::Format_A8_Unorm:                 return "Format_A8_Unorm";
    case EFormat::Format_R1_Unorm:                 return "Format_R1_Unorm";
    case EFormat::Format_B5G6R5_Unorm:             return "Format_B5G6R5_Unorm";
    case EFormat::Format_B5G5R5A1_Unorm:           return "Format_B5G5R5A1_Unorm";
    case EFormat::Format_B8G8R8A8_Unorm:           return "Format_B8G8R8A8_Unorm";
    case EFormat::Format_B8G8R8X8_Unorm:           return "Format_B8G8R8X8_Unorm";
    case EFormat::Format_B8G8R8A8_Typeless:        return "Format_B8G8R8A8_Typeless";
    case EFormat::Format_B8G8R8A8_Unorm_SRGB:      return "Format_B8G8R8A8_Unorm_SRGB";
    case EFormat::Format_B8G8R8X8_Typeless:        return "Format_B8G8R8X8_Typeless";
    case EFormat::Format_B8G8R8X8_Unorm_SRGB:      return "Format_B8G8R8X8_Unorm_SRGB";
    default:                                       return "Format_UNKNOWN";
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

enum class EComparisonFunc
{
    ComparisonFunc_Never        = 1,
    ComparisonFunc_Less         = 2,
    ComparisonFunc_Equal        = 3,
    ComparisonFunc_LessEqual    = 4,
    ComparisonFunc_Greater      = 5,
    ComparisonFunc_NotEqual     = 6,
    ComparisonFunc_GreaterEqual = 7,
    ComparisonFunc_Always       = 8
};

inline const Char* ToString(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
    case EComparisonFunc::ComparisonFunc_Never:        return "ComparisonFunc_Never";
    case EComparisonFunc::ComparisonFunc_Less:         return "ComparisonFunc_Less";
    case EComparisonFunc::ComparisonFunc_Equal:        return "ComparisonFunc_Equal";
    case EComparisonFunc::ComparisonFunc_LessEqual:    return "ComparisonFunc_LessEqual";
    case EComparisonFunc::ComparisonFunc_Greater:      return "ComparisonFunc_Greater";
    case EComparisonFunc::ComparisonFunc_NotEqual:     return "ComparisonFunc_NotEqual";
    case EComparisonFunc::ComparisonFunc_GreaterEqual: return "ComparisonFunc_GreaterEqual";
    case EComparisonFunc::ComparisonFunc_Always:       return "ComparisonFunc_Always";
    default:                                           return "";
    }
}

enum class EPrimitiveTopologyType
{
    PrimitiveTopologyType_Undefined = 0,
    PrimitiveTopologyType_Point     = 1,
    PrimitiveTopologyType_Line      = 2,
    PrimitiveTopologyType_Triangle  = 3,
    PrimitiveTopologyType_Patch     = 4
};

inline const Char* ToString(EPrimitiveTopologyType PrimitveTopologyType)
{
    switch (PrimitveTopologyType)
    {
    case EPrimitiveTopologyType::PrimitiveTopologyType_Undefined: return "PrimitiveTopologyType_Undefined";
    case EPrimitiveTopologyType::PrimitiveTopologyType_Point:     return "PrimitiveTopologyType_Point";
    case EPrimitiveTopologyType::PrimitiveTopologyType_Line:      return "PrimitiveTopologyType_Line";
    case EPrimitiveTopologyType::PrimitiveTopologyType_Triangle:  return "PrimitiveTopologyType_Triangle";
    case EPrimitiveTopologyType::PrimitiveTopologyType_Patch:     return "PrimitiveTopologyType_Patch";
    default:                                                      return "";
    }
}

enum class EResourceState
{
    ResourceState_Common                          = 0,
    ResourceState_VertexAndConstantBuffer         = 1,
    ResourceState_IndexBuffer                     = 2,
    ResourceState_RenderTarget                    = 3,
    ResourceState_UnorderedAccess                 = 4,
    ResourceState_DepthWrite                      = 5,
    ResourceState_DepthRead                       = 6,
    ResourceState_NonPixelShaderResource          = 7,
    ResourceState_PixelShaderResource             = 8,
    ResourceState_CopyDest                        = 9,
    ResourceState_CopySource                      = 10,
    ResourceState_ResolveDest                     = 11,
    ResourceState_ResolveSource                   = 12,
    ResourceState_RayTracingAccelerationStructure = 13,
    ResourceState_ShadingRateSource               = 14,
    ResourceState_Present                         = 15,
    ResourceState_GenericRead                     = 16,
};

inline const Char* ToString(EResourceState ResourceState)
{
    switch (ResourceState)
    {
    case EResourceState::ResourceState_Common:                          return "ResourceState_Common";
    case EResourceState::ResourceState_VertexAndConstantBuffer:         return "ResourceState_VertexAndConstantBuffer";
    case EResourceState::ResourceState_IndexBuffer:                     return "ResourceState_IndexBuffer";
    case EResourceState::ResourceState_RenderTarget:                    return "ResourceState_RenderTarget";
    case EResourceState::ResourceState_UnorderedAccess:                 return "ResourceState_UnorderedAccess";
    case EResourceState::ResourceState_DepthWrite:                      return "ResourceState_DepthWrite";
    case EResourceState::ResourceState_DepthRead:                       return "ResourceState_DepthRead";
    case EResourceState::ResourceState_NonPixelShaderResource:          return "ResourceState_NonPixelShaderResource";
    case EResourceState::ResourceState_PixelShaderResource:             return "ResourceState_PixelShaderResource";
    case EResourceState::ResourceState_CopyDest:                        return "ResourceState_CopyDest";
    case EResourceState::ResourceState_CopySource:                      return "ResourceState_CopySource";
    case EResourceState::ResourceState_ResolveDest:                     return "ResourceState_ResolveDest";
    case EResourceState::ResourceState_ResolveSource:                   return "ResourceState_ResolveSource";
    case EResourceState::ResourceState_RayTracingAccelerationStructure: return "ResourceState_RayTracingAccelerationStructure";
    case EResourceState::ResourceState_ShadingRateSource:               return "ResourceState_ShadingRateSource";
    case EResourceState::ResourceState_Present:                         return "ResourceState_Present";
    default:                                                            return "";
    }
}

enum EPrimitiveTopology
{
    PrimitiveTopology_Undefined     = 0,
    PrimitiveTopology_PointList     = 1,
    PrimitiveTopology_LineList      = 2,
    PrimitiveTopology_LineStrip     = 3,
    PrimitiveTopology_TriangleList  = 4,
    PrimitiveTopology_TriangleStrip = 5,
};

inline const Char* ToString(EPrimitiveTopology ResourceState)
{
    switch (ResourceState)
    {
    case EPrimitiveTopology::PrimitiveTopology_Undefined:     return "PrimitiveTopology_Undefined";
    case EPrimitiveTopology::PrimitiveTopology_PointList:     return "PrimitiveTopology_PointList";
    case EPrimitiveTopology::PrimitiveTopology_LineList:      return "PrimitiveTopology_LineList";
    case EPrimitiveTopology::PrimitiveTopology_LineStrip:     return "PrimitiveTopology_LineStrip";
    case EPrimitiveTopology::PrimitiveTopology_TriangleList:  return "PrimitiveTopology_TriangleList";
    case EPrimitiveTopology::PrimitiveTopology_TriangleStrip: return "PrimitiveTopology_TriangleStrip";
    default:                                                  return "";
    }
}

enum EShadingRate
{
    ShadingRate_1x1 = 1,
    ShadingRate_2x2 = 2,
    ShadingRate_4x4 = 3,
};

inline const Char* ToString(EShadingRate ShadingRate)
{
    switch (ShadingRate)
    {
    case EShadingRate::ShadingRate_1x1: return "ShadingRate_1x1";
    case EShadingRate::ShadingRate_2x2: return "ShadingRate_2x2";
    case EShadingRate::ShadingRate_4x4: return "ShadingRate_4x4";
    default:                            return "";
    }
}

struct ColorClearValue
{
    inline ColorClearValue()
        : R(1.0f)
        , G(1.0f)
        , B(1.0f)
        , A(1.0f)
    {
    }

    inline ColorClearValue(Float InR, Float InG, Float InB, Float InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    union
    {
        struct 
        {
            Float R;
            Float G;
            Float B;
            Float A;
        };

        Float RGBA[4];
    };
};

struct DepthStencilClearValue
{
    DepthStencilClearValue() = default;

    inline DepthStencilClearValue(Float InDepth, UInt8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    {
    }

    Float Depth   = 1.0f;
    UInt8 Stencil = 0;
};

enum class EClearValueType : Byte
{
    ClearValueType_Unknown      = 0,
    ClearValueType_Color        = 1,
    ClearValueType_DepthStencil = 2,
};

struct ClearValue
{
    ClearValue()
        : Color()
        , Type(EClearValueType::ClearValueType_Color)
    {
    }

    ClearValue(const ColorClearValue& InClearColor)
        : Color(InClearColor)
        , Type(EClearValueType::ClearValueType_Color)
    {
    }

    ClearValue(const DepthStencilClearValue& InDepthStencil)
        : DepthStencil(InDepthStencil)
        , Type(EClearValueType::ClearValueType_DepthStencil)
    {
    }

    ClearValue(const ClearValue& Other)
        : Color()
        , Type(Other.Type)
    {
        if (Type == EClearValueType::ClearValueType_Color)
        {
            Color = Other.Color;
        }
        else
        {
            DepthStencil = Other.DepthStencil;
        }
    }

    inline ClearValue& operator=(const ClearValue& Other)
    {
        Type = Other.Type;
        if(Type == EClearValueType::ClearValueType_Color)
        {
            Color = Other.Color;
        }
        else
        {
            DepthStencil = Other.DepthStencil;
        }

        return *this;
    }

    inline ClearValue& operator=(const ColorClearValue& InColor)
    {
        Type  = EClearValueType::ClearValueType_Color;
        Color = InColor;
        return *this;
    }

    inline ClearValue& operator=(const DepthStencilClearValue& InDepthStencil)
    {
        Type         = EClearValueType::ClearValueType_DepthStencil;
        DepthStencil = InDepthStencil;
        return *this;
    }

    EClearValueType Type;
    union
    {
        ColorClearValue Color;
        DepthStencilClearValue DepthStencil;
    };
};

struct CopyBufferInfo
{
    CopyBufferInfo() = default;

    inline CopyBufferInfo(UInt64 InSourceOffset, UInt32 InDestinationOffset, UInt32 InSizeInBytes)
        : SourceOffset(InSourceOffset)
        , DestinationOffset(InDestinationOffset)
        , SizeInBytes(InSizeInBytes)
    {
    }

    UInt64 SourceOffset      = 0;
    UInt32 DestinationOffset = 0;
    UInt32 SizeInBytes       = 0;
};

struct CopyTextureSubresourceInfo
{
    CopyTextureSubresourceInfo() = default;

    inline CopyTextureSubresourceInfo(UInt32 InX, UInt32 InY, UInt32 InZ, UInt32 InSubresourceIndex)
        : x(InX)
        , y(InY)
        , z(InZ)
        , SubresourceIndex(InSubresourceIndex)
    {
    }

    UInt32 x = 0;
    UInt32 y = 0;
    UInt32 z = 0;
    UInt32 SubresourceIndex = 0;
};

struct CopyTextureInfo
{
    CopyTextureSubresourceInfo Source;
    CopyTextureSubresourceInfo Destination;

    UInt32 Width  = 0;
    UInt32 Height = 0;
    UInt32 Depth  = 0;
};
