#pragma once
#include "Format.h"

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
