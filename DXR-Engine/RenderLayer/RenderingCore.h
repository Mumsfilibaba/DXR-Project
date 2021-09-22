#pragma once
#include "Core.h"

// TODO: Move to rendering
#include "Assets/TextureFormat.h"

#include "Core/Math/Color.h"

enum class ECubeFace
{
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5,
};

inline uint32 GetCubeFaceIndex( ECubeFace CubeFace )
{
    return static_cast<uint32>(CubeFace);
}

inline ECubeFace GetCubeFaceFromIndex( uint32 Index )
{
    if ( Index > GetCubeFaceIndex( ECubeFace::NegZ ) )
    {
        return static_cast<ECubeFace>(-1);
    }
    else
    {
        return static_cast<ECubeFace>(Index);
    }
}

enum class EComparisonFunc
{
    Never = 1,
    Less = 2,
    Equal = 3,
    LessEqual = 4,
    Greater = 5,
    NotEqual = 6,
    GreaterEqual = 7,
    Always = 8
};

inline const char* ToString( EComparisonFunc ComparisonFunc )
{
    switch ( ComparisonFunc )
    {
        case EComparisonFunc::Never:        return "Never";
        case EComparisonFunc::Less:         return "Less";
        case EComparisonFunc::Equal:        return "Equal";
        case EComparisonFunc::LessEqual:    return "LessEqual";
        case EComparisonFunc::Greater:      return "Greater";
        case EComparisonFunc::NotEqual:     return "NotEqual";
        case EComparisonFunc::GreaterEqual: return "GreaterEqual";
        case EComparisonFunc::Always:       return "Always";
        default: return "Unknown";
    }
}

enum class EPrimitiveTopologyType
{
    Undefined = 0,
    Point = 1,
    Line = 2,
    Triangle = 3,
    Patch = 4
};

inline const char* ToString( EPrimitiveTopologyType PrimitveTopologyType )
{
    switch ( PrimitveTopologyType )
    {
        case EPrimitiveTopologyType::Undefined: return "Undefined";
        case EPrimitiveTopologyType::Point:     return "Point";
        case EPrimitiveTopologyType::Line:      return "Line";
        case EPrimitiveTopologyType::Triangle:  return "Triangle";
        case EPrimitiveTopologyType::Patch:     return "Patch";
        default: return "Unknown";
    }
}

enum class EResourceState
{
    Common = 0,
    VertexAndConstantBuffer = 1,
    IndexBuffer = 2,
    RenderTarget = 3,
    UnorderedAccess = 4,
    DepthWrite = 5,
    DepthRead = 6,
    NonPixelShaderResource = 7,
    PixelShaderResource = 8,
    CopyDest = 9,
    CopySource = 10,
    ResolveDest = 11,
    ResolveSource = 12,
    RayTracingAccelerationStructure = 13,
    ShadingRateSource = 14,
    Present = 15,
    GenericRead = 16,
};

inline const char* ToString( EResourceState ResourceState )
{
    switch ( ResourceState )
    {
        case EResourceState::Common:                          return "Common";
        case EResourceState::VertexAndConstantBuffer:         return "VertexAndConstantBuffer";
        case EResourceState::IndexBuffer:                     return "IndexBuffer";
        case EResourceState::RenderTarget:                    return "RenderTarget";
        case EResourceState::UnorderedAccess:                 return "UnorderedAccess";
        case EResourceState::DepthWrite:                      return "DepthWrite";
        case EResourceState::DepthRead:                       return "DepthRead";
        case EResourceState::NonPixelShaderResource:          return "NonPixelShaderResource";
        case EResourceState::PixelShaderResource:             return "PixelShaderResource";
        case EResourceState::CopyDest:                        return "CopyDest";
        case EResourceState::CopySource:                      return "CopySource";
        case EResourceState::ResolveDest:                     return "ResolveDest";
        case EResourceState::ResolveSource:                   return "ResolveSource";
        case EResourceState::RayTracingAccelerationStructure: return "RayTracingAccelerationStructure";
        case EResourceState::ShadingRateSource:               return "ShadingRateSource";
        case EResourceState::Present:                         return "Present";
        default: return "Unknown";
    }
}

enum class EPrimitiveTopology
{
    Undefined = 0,
    PointList = 1,
    LineList = 2,
    LineStrip = 3,
    TriangleList = 4,
    TriangleStrip = 5,
};

inline const char* ToString( EPrimitiveTopology ResourceState )
{
    switch ( ResourceState )
    {
        case EPrimitiveTopology::Undefined:     return "Undefined";
        case EPrimitiveTopology::PointList:     return "PointList";
        case EPrimitiveTopology::LineList:      return "LineList";
        case EPrimitiveTopology::LineStrip:     return "LineStrip";
        case EPrimitiveTopology::TriangleList:  return "TriangleList";
        case EPrimitiveTopology::TriangleStrip: return "TriangleStrip";
        default: return "Unknown";
    }
}

enum class EShadingRate
{
    VRS_1x1 = 0x0,
    VRS_1x2 = 0x1,
    VRS_2x1 = 0x4,
    VRS_2x2 = 0x5,
    VRS_2x4 = 0x6,
    VRS_4x2 = 0x9,
    VRS_4x4 = 0xa,
};

inline const char* ToString( EShadingRate ShadingRate )
{
    switch ( ShadingRate )
    {
        case EShadingRate::VRS_1x1: return "VRS_1x1";
        case EShadingRate::VRS_1x2: return "VRS_1x2";
        case EShadingRate::VRS_2x1: return "VRS_2x1";
        case EShadingRate::VRS_2x2: return "VRS_2x2";
        case EShadingRate::VRS_2x4: return "VRS_2x4";
        case EShadingRate::VRS_4x2: return "VRS_4x2";
        case EShadingRate::VRS_4x4: return "VRS_4x4";
        default: return "Unknown";
    }
}

struct DepthStencilF
{
    DepthStencilF() = default;

    DepthStencilF( float InDepth, uint8 InStencil )
        : Depth( InDepth )
        , Stencil( InStencil )
    {
    }

    float Depth = 1.0f;
    uint8 Stencil = 0;
};

struct ClearValue
{
public:
    enum class EType
    {
        Color = 1,
        DepthStencil = 2
    };

    // NOTE: Default clear color is black
    ClearValue()
        : Type( EType::Color )
        , Format( EFormat::Unknown )
        , Color( 0.0f, 0.0f, 0.0f, 1.0f )
    {
    }

    ClearValue( EFormat InFormat, float Depth, uint8 Stencil )
        : Type( EType::DepthStencil )
        , Format( InFormat )
        , DepthStencil( Depth, Stencil )
    {
    }

    ClearValue( EFormat InFormat, float r, float g, float b, float a )
        : Type( EType::Color )
        , Format( InFormat )
        , Color( r, g, b, a )
    {
    }

    ClearValue( const ClearValue& Other )
        : Type( Other.Type )
        , Format( Other.Format )
        , Color()
    {
        if ( Other.Type == EType::Color )
        {
            Color = Other.Color;
        }
        else if ( Other.Type == EType::DepthStencil )
        {
            DepthStencil = Other.DepthStencil;
        }
    }

    ClearValue& operator=( const ClearValue& Other )
    {
        Type = Other.Type;
        Format = Other.Format;

        if ( Other.Type == EType::Color )
        {
            Color = Other.Color;
        }
        else if ( Other.Type == EType::DepthStencil )
        {
            DepthStencil = Other.DepthStencil;
        }

        return *this;
    }

    EType GetType() const
    {
        return Type;
    }

    EFormat GetFormat() const
    {
        return Format;
    }

    ColorF& AsColor()
    {
        Assert( Type == EType::Color );
        return Color;
    }

    const ColorF& AsColor() const
    {
        Assert( Type == EType::Color );
        return Color;
    }

    DepthStencilF& AsDepthStencil()
    {
        Assert( Type == EType::DepthStencil );
        return DepthStencil;
    }

    const DepthStencilF& AsDepthStencil() const
    {
        Assert( Type == EType::DepthStencil );
        return DepthStencil;
    }

private:
    EType Type;
    EFormat Format;
    union
    {
        ColorF        Color;
        DepthStencilF DepthStencil;
    };
};

struct ResourceData
{
    ResourceData()
        : Data( nullptr )
    {
    }

    ResourceData( const void* InData, uint32 InSizeInBytes )
        : Data( InData )
        , SizeInBytes( InSizeInBytes )
    {
    }

    ResourceData( const void* InData, EFormat InFormat, uint32 InWidth )
        : Data( InData )
        , Format( InFormat )
        , Width( InWidth )
        , Height( 0 )
    {
    }

    ResourceData( const void* InData, EFormat InFormat, uint32 InWidth, uint32 InHeight )
        : Data( InData )
        , Format( InFormat )
        , Width( InWidth )
        , Height( InHeight )
    {
    }

    void Set( const void* InData, uint32 InSizeInBytes )
    {
        Data = InData;
        SizeInBytes = InSizeInBytes;
    }

    void Set( const void* InData, EFormat InFormat, uint32 InWidth )
    {
        Data = InData;
        Format = InFormat;
        Width = InWidth;
    }

    void Set( const void* InData, EFormat InFormat, uint32 InWidth, uint32 InHeight )
    {
        Set( InData, InFormat, InWidth );
        Height = InHeight;
    }

    const void* GetData() const
    {
        return Data;
    }

    uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    uint32 GetPitch() const
    {
        return GetByteStrideFromFormat( Format ) * Width;
    }

    uint32 GetSlicePitch() const
    {
        return GetByteStrideFromFormat( Format ) * Width * Height;
    }

private:
    const void* Data;
    union
    {
        struct
        {
            uint32 SizeInBytes;
        };
        struct
        {
            EFormat Format;
            uint32  Width;
            uint32  Height;
        };
    };
};

struct CopyBufferInfo
{
    CopyBufferInfo() = default;

    CopyBufferInfo( uint64 InSourceOffset, uint32 InDestinationOffset, uint32 InSizeInBytes )
        : SourceOffset( InSourceOffset )
        , DestinationOffset( InDestinationOffset )
        , SizeInBytes( InSizeInBytes )
    {
    }

    uint64 SourceOffset = 0;
    uint32 DestinationOffset = 0;
    uint32 SizeInBytes = 0;
};

struct CopyTextureSubresourceInfo
{
    CopyTextureSubresourceInfo() = default;

    CopyTextureSubresourceInfo( uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex )
        : x( InX )
        , y( InY )
        , z( InZ )
        , SubresourceIndex( InSubresourceIndex )
    {
    }

    uint32 x = 0;
    uint32 y = 0;
    uint32 z = 0;
    uint32 SubresourceIndex = 0;
};

struct CopyTextureInfo
{
    CopyTextureSubresourceInfo Source;
    CopyTextureSubresourceInfo Destination;
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 Depth = 0;
};
