#pragma once
#include "TextureFormat.h"

#include "Core/Math/Color.h"
#include "Core/Templates/EnumUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Constants

enum
{
    kRHIMaxVertexBuffers = 32
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ECubeFace

enum class ECubeFace
{
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5,
};

inline uint32 GetCubeFaceIndex(ECubeFace CubeFace)
{
    return static_cast<uint32>(CubeFace);
}

inline ECubeFace GetCubeFaceFromIndex(uint32 Index)
{
    if (Index > GetCubeFaceIndex(ECubeFace::NegZ))
    {
        return static_cast<ECubeFace>(-1);
    }
    else
    {
        return static_cast<ECubeFace>(Index);
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EComparisonFunc

enum class EComparisonFunc
{
    Never        = 1,
    Less         = 2,
    Equal        = 3,
    LessEqual    = 4,
    Greater      = 5,
    NotEqual     = 6,
    GreaterEqual = 7,
    Always       = 8
};

inline const char* ToString(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
    case EComparisonFunc::Never:        return "Never";
    case EComparisonFunc::Less:         return "Less";
    case EComparisonFunc::Equal:        return "Equal";
    case EComparisonFunc::LessEqual:    return "LessEqual";
    case EComparisonFunc::Greater:      return "Greater";
    case EComparisonFunc::NotEqual:     return "NotEqual";
    case EComparisonFunc::GreaterEqual: return "GreaterEqual";
    case EComparisonFunc::Always:       return "Always";
    default:                            return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EPrimitiveTopologyType

enum class EPrimitiveTopologyType
{
    Undefined = 0,
    Point     = 1,
    Line      = 2,
    Triangle  = 3,
    Patch     = 4
};

inline const char* ToString(EPrimitiveTopologyType PrimitveTopologyType)
{
    switch (PrimitveTopologyType)
    {
    case EPrimitiveTopologyType::Undefined: return "Undefined";
    case EPrimitiveTopologyType::Point:     return "Point";
    case EPrimitiveTopologyType::Line:      return "Line";
    case EPrimitiveTopologyType::Triangle:  return "Triangle";
    case EPrimitiveTopologyType::Patch:     return "Patch";
    default:                                return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EResourceAccess

// TODO: These should be flags

enum class EResourceAccess
{
    Common                          = 0,
    VertexAndConstantBuffer         = 1,
    IndexBuffer                     = 2,
    RenderTarget                    = 3,
    UnorderedAccess                 = 4,
    DepthWrite                      = 5,
    DepthRead                       = 6,
    NonPixelShaderResource          = 7,
    PixelShaderResource             = 8,
    CopyDest                        = 9,
    CopySource                      = 10,
    ResolveDest                     = 11,
    ResolveSource                   = 12,
    RayTracingAccelerationStructure = 13,
    ShadingRateSource               = 14,
    Present                         = 15,
    GenericRead                     = 16,
};

ENUM_CLASS_OPERATORS(EResourceAccess);

inline const char* ToString(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
    case EResourceAccess::Common:                          return "Common";
    case EResourceAccess::VertexAndConstantBuffer:         return "VertexAndConstantBuffer";
    case EResourceAccess::IndexBuffer:                     return "IndexBuffer";
    case EResourceAccess::RenderTarget:                    return "RenderTarget";
    case EResourceAccess::UnorderedAccess:                 return "UnorderedAccess";
    case EResourceAccess::DepthWrite:                      return "DepthWrite";
    case EResourceAccess::DepthRead:                       return "DepthRead";
    case EResourceAccess::NonPixelShaderResource:          return "NonPixelShaderResource";
    case EResourceAccess::PixelShaderResource:             return "PixelShaderResource";
    case EResourceAccess::CopyDest:                        return "CopyDest";
    case EResourceAccess::CopySource:                      return "CopySource";
    case EResourceAccess::ResolveDest:                     return "ResolveDest";
    case EResourceAccess::ResolveSource:                   return "ResolveSource";
    case EResourceAccess::RayTracingAccelerationStructure: return "RayTracingAccelerationStructure";
    case EResourceAccess::ShadingRateSource:               return "ShadingRateSource";
    case EResourceAccess::Present:                         return "Present";
    default:                                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EPrimitiveTopology

enum class EPrimitiveTopology
{
    Undefined     = 0,
    PointList     = 1,
    LineList      = 2,
    LineStrip     = 3,
    TriangleList  = 4,
    TriangleStrip = 5,
};

inline const char* ToString(EPrimitiveTopology ResourceState)
{
    switch (ResourceState)
    {
    case EPrimitiveTopology::Undefined:     return "Undefined";
    case EPrimitiveTopology::PointList:     return "PointList";
    case EPrimitiveTopology::LineList:      return "LineList";
    case EPrimitiveTopology::LineStrip:     return "LineStrip";
    case EPrimitiveTopology::TriangleList:  return "TriangleList";
    case EPrimitiveTopology::TriangleStrip: return "TriangleStrip";
    default:                                return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShadingRate

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

inline const char* ToString(EShadingRate ShadingRate)
{
    switch (ShadingRate)
    {
    case EShadingRate::VRS_1x1: return "VRS_1x1";
    case EShadingRate::VRS_1x2: return "VRS_1x2";
    case EShadingRate::VRS_2x1: return "VRS_2x1";
    case EShadingRate::VRS_2x2: return "VRS_2x2";
    case EShadingRate::VRS_2x4: return "VRS_2x4";
    case EShadingRate::VRS_4x2: return "VRS_4x2";
    case EShadingRate::VRS_4x4: return "VRS_4x4";
    default:                    return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EDescriptorType

enum class EDescriptorType : uint32
{
    Unknown         = 0,
    UnorderedAccess = 1,
    ShaderResource  = 2,
    ConstantBuffer  = 3,
    Sampler         = 4
};

inline const char* ToString(EDescriptorType DescriptorType)
{
    switch (DescriptorType)
    {
        case EDescriptorType::UnorderedAccess: return "UnorderedAccess";
        case EDescriptorType::ShaderResource:  return "ShaderResource";
        case EDescriptorType::ConstantBuffer:  return "ConstantBuffer";
        case EDescriptorType::Sampler:         return "Sampler";
        default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDescriptorHandle

class CRHIDescriptorHandle
{
    enum : uint32
    {
        InvalidHandle = uint32(~0)
    };

public:

    /**
     * @brief: Default Constructor
     */
    CRHIDescriptorHandle()
        : Type(EDescriptorType::Unknown)
        , Index(InvalidHandle)
    { }

    /**
     * @brief: Constructor that creates a descriptor-handle
     *
     * @param InType: Type of descriptor
     * @param InIndex: Index to identify the descriptor-handle inside the backend (Descriptor-Heap)
     */
    CRHIDescriptorHandle(EDescriptorType InType, uint32 InIndex)
        : Type(InType)
        , Index(InIndex)
    { }

    /** @return: Returns true if the handle is valid */
    bool IsValid() const { return (Type != EDescriptorType::Unknown) && (Index != InvalidHandle); }

    /**
     * @brief: Compare two descriptor-handles to see if the reference the same resource
     *
     * @return: Returns true if the handles are equal
     */
    bool operator==(const CRHIDescriptorHandle& RHS) const
    {
        return (Type == RHS.Type) && (Index == RHS.Index);
    }

    /**
     * @brief: Compare two descriptor-handles to see if the reference the same resource
     *
     * @return: Returns false if the handles are equal
     */
    bool operator!=(const CRHIDescriptorHandle& RHS) const
    {
        return !(*this == RHS);
    }

private:
    union
    {
        struct
        {
            uint32          Index : 24;
            EDescriptorType Type  : 8;
        };

        uint32 Data;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDepthStencil

struct SDepthStencil
{
    SDepthStencil() = default;

    FORCEINLINE SDepthStencil(float InDepth, uint8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    { }

    float Depth = 1.0f;
    uint8 Stencil = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SClearValue

struct SClearValue
{
public:

    enum class EType
    {
        Color = 1,
        DepthStencil = 2
    };

    // NOTE: Default clear color is black
    FORCEINLINE SClearValue()
        : Type(EType::Color)
        , Format(EFormat::Unknown)
        , Color(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    FORCEINLINE SClearValue(EFormat InFormat, float Depth, uint8 Stencil)
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencil(Depth, Stencil)
    { }

    FORCEINLINE SClearValue(EFormat InFormat, float r, float g, float b, float a)
        : Type(EType::Color)
        , Format(InFormat)
        , Color(r, g, b, a)
    { }

    FORCEINLINE SClearValue(const SClearValue& Other)
        : Type(Other.Type)
        , Format(Other.Format)
        , Color()
    {
        if (Other.Type == EType::Color)
        {
            Color = Other.Color;
        }
        else if (Other.Type == EType::DepthStencil)
        {
            DepthStencil = Other.DepthStencil;
        }
    }

    FORCEINLINE SClearValue& operator=(const SClearValue& Other)
    {
        Type = Other.Type;
        Format = Other.Format;

        if (Other.Type == EType::Color)
        {
            Color = Other.Color;
        }
        else if (Other.Type == EType::DepthStencil)
        {
            DepthStencil = Other.DepthStencil;
        }

        return *this;
    }

    FORCEINLINE EType GetType() const
    {
        return Type;
    }

    FORCEINLINE EFormat GetFormat() const
    {
        return Format;
    }

    FORCEINLINE CFloatColor& AsColor()
    {
        Assert(Type == EType::Color);
        return Color;
    }

    FORCEINLINE const CFloatColor& AsColor() const
    {
        Assert(Type == EType::Color);
        return Color;
    }

    FORCEINLINE SDepthStencil& AsDepthStencil()
    {
        Assert(Type == EType::DepthStencil);
        return DepthStencil;
    }

    FORCEINLINE const SDepthStencil& AsDepthStencil() const
    {
        Assert(Type == EType::DepthStencil);
        return DepthStencil;
    }

private:
    EType   Type;
    EFormat Format;
    union
    {
        CFloatColor       Color;
        SDepthStencil DepthStencil;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIResourceData

struct SRHIResourceData
{
    FORCEINLINE SRHIResourceData()
        : Data(nullptr)
    { }

    FORCEINLINE SRHIResourceData(const void* InData, uint32 InSizeInBytes)
        : Data(InData)
        , SizeInBytes(InSizeInBytes)
    { }

    FORCEINLINE SRHIResourceData(const void* InData, EFormat InFormat, uint32 InWidth)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(0)
    { }

    FORCEINLINE SRHIResourceData(const void* InData, EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
    { }

    FORCEINLINE void Set(const void* InData, uint32 InSizeInBytes)
    {
        Data = InData;
        SizeInBytes = InSizeInBytes;
    }

    FORCEINLINE void Set(const void* InData, EFormat InFormat, uint32 InWidth)
    {
        Data = InData;
        Format = InFormat;
        Width = InWidth;
    }

    FORCEINLINE void Set(const void* InData, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    {
        Set(InData, InFormat, InWidth);
        Height = InHeight;
    }

    FORCEINLINE const void* GetData() const
    {
        return Data;
    }

    FORCEINLINE uint32 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    FORCEINLINE uint32 GetPitch() const
    {
        return GetByteStrideFromFormat(Format) * Width;
    }

    FORCEINLINE uint32 GetSlicePitch() const
    {
        return GetByteStrideFromFormat(Format) * Width * Height;
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyBufferInfo

struct SRHICopyBufferInfo
{
    SRHICopyBufferInfo() = default;

    FORCEINLINE SRHICopyBufferInfo(uint64 InSourceOffset, uint32 InDestinationOffset, uint32 InSizeInBytes)
        : SourceOffset(InSourceOffset)
        , DestinationOffset(InDestinationOffset)
        , SizeInBytes(InSizeInBytes)
    { }

    uint64 SourceOffset = 0;
    uint32 DestinationOffset = 0;
    uint32 SizeInBytes = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureSubresourceInfo

struct SRHICopyTextureSubresourceInfo
{
    SRHICopyTextureSubresourceInfo() = default;

    FORCEINLINE SRHICopyTextureSubresourceInfo(uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex)
        : x(InX)
        , y(InY)
        , z(InZ)
        , SubresourceIndex(InSubresourceIndex)
    { }

    uint32 x = 0;
    uint32 y = 0;
    uint32 z = 0;
    uint32 SubresourceIndex = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureInfo

struct SRHICopyTextureInfo
{
    SRHICopyTextureSubresourceInfo Source;
    SRHICopyTextureSubresourceInfo Destination;
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 Depth = 0;
};
