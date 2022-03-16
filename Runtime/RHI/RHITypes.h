#pragma once
#include "TextureFormat.h"

#include "Core/Math/Color.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHICubeFace

enum class ERHICubeFace
{
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5,
};

inline uint32 GetCubeFaceIndex(ERHICubeFace CubeFace)
{
    return static_cast<uint32>(CubeFace);
}

inline ERHICubeFace GetCubeFaceFromIndex(uint32 Index)
{
    if (Index > GetCubeFaceIndex(ERHICubeFace::NegZ))
    {
        return static_cast<ERHICubeFace>(-1);
    }
    else
    {
        return static_cast<ERHICubeFace>(Index);
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIComparisonFunc

enum class ERHIComparisonFunc
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

inline const char* ToString(ERHIComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
    case ERHIComparisonFunc::Never:        return "Never";
    case ERHIComparisonFunc::Less:         return "Less";
    case ERHIComparisonFunc::Equal:        return "Equal";
    case ERHIComparisonFunc::LessEqual:    return "LessEqual";
    case ERHIComparisonFunc::Greater:      return "Greater";
    case ERHIComparisonFunc::NotEqual:     return "NotEqual";
    case ERHIComparisonFunc::GreaterEqual: return "GreaterEqual";
    case ERHIComparisonFunc::Always:       return "Always";
    default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIPrimitiveTopologyType

enum class ERHIPrimitiveTopologyType
{
    Undefined = 0,
    Point     = 1,
    Line      = 2,
    Triangle  = 3,
    Patch     = 4
};

inline const char* ToString(ERHIPrimitiveTopologyType PrimitveTopologyType)
{
    switch (PrimitveTopologyType)
    {
    case ERHIPrimitiveTopologyType::Undefined: return "Undefined";
    case ERHIPrimitiveTopologyType::Point:     return "Point";
    case ERHIPrimitiveTopologyType::Line:      return "Line";
    case ERHIPrimitiveTopologyType::Triangle:  return "Triangle";
    case ERHIPrimitiveTopologyType::Patch:     return "Patch";
    default:                                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIResourceState

enum class ERHIResourceState
{
    Common                          = 0,
    VertexAndConstantBuffer         = 1,
    IndexBuffer                     = 2,
    RenderTarget                    = 3,
    RenderTargetClear               = 4,
    UnorderedAccess                 = 5,
    DepthClear                      = 6,
    DepthWrite                      = 7,
    DepthRead                       = 8,
    NonPixelShaderResource          = 9,
    PixelShaderResource             = 10,
    CopyDest                        = 11,
    CopySource                      = 12,
    ResolveDest                     = 13,
    ResolveSource                   = 14,
    RayTracingAccelerationStructure = 15,
    ShadingRateSource               = 16,
    Present                         = 17,
    GenericRead                     = 18,
};

inline const char* ToString(ERHIResourceState ResourceState)
{
    switch (ResourceState)
    {
    case ERHIResourceState::Common:                          return "Common";
    case ERHIResourceState::VertexAndConstantBuffer:         return "VertexAndConstantBuffer";
    case ERHIResourceState::IndexBuffer:                     return "IndexBuffer";
    case ERHIResourceState::RenderTarget:                    return "RenderTarget";
    case ERHIResourceState::RenderTargetClear:               return "RenderTargetClear";
    case ERHIResourceState::UnorderedAccess:                 return "UnorderedAccess";
    case ERHIResourceState::DepthClear:                      return "DepthClear";
    case ERHIResourceState::DepthWrite:                      return "DepthWrite";
    case ERHIResourceState::DepthRead:                       return "DepthRead";
    case ERHIResourceState::NonPixelShaderResource:          return "NonPixelShaderResource";
    case ERHIResourceState::PixelShaderResource:             return "PixelShaderResource";
    case ERHIResourceState::CopyDest:                        return "CopyDest";
    case ERHIResourceState::CopySource:                      return "CopySource";
    case ERHIResourceState::ResolveDest:                     return "ResolveDest";
    case ERHIResourceState::ResolveSource:                   return "ResolveSource";
    case ERHIResourceState::RayTracingAccelerationStructure: return "RayTracingAccelerationStructure";
    case ERHIResourceState::ShadingRateSource:               return "ShadingRateSource";
    case ERHIResourceState::Present:                         return "Present";
    default:                                                  return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIPrimitiveTopology

enum class ERHIPrimitiveTopology
{
    Undefined     = 0,
    PointList     = 1,
    LineList      = 2,
    LineStrip     = 3,
    TriangleList  = 4,
    TriangleStrip = 5,
};

inline const char* ToString(ERHIPrimitiveTopology ResourceState)
{
    switch (ResourceState)
    {
    case ERHIPrimitiveTopology::Undefined:     return "Undefined";
    case ERHIPrimitiveTopology::PointList:     return "PointList";
    case ERHIPrimitiveTopology::LineList:      return "LineList";
    case ERHIPrimitiveTopology::LineStrip:     return "LineStrip";
    case ERHIPrimitiveTopology::TriangleList:  return "TriangleList";
    case ERHIPrimitiveTopology::TriangleStrip: return "TriangleStrip";
    default:                                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShadingRate

enum class ERHIShadingRate
{
    VRS_1x1 = 0x0,
    VRS_1x2 = 0x1,
    VRS_2x1 = 0x4,
    VRS_2x2 = 0x5,
    VRS_2x4 = 0x6,
    VRS_4x2 = 0x9,
    VRS_4x4 = 0xa,
};

inline const char* ToString(ERHIShadingRate ShadingRate)
{
    switch (ShadingRate)
    {
    case ERHIShadingRate::VRS_1x1: return "VRS_1x1";
    case ERHIShadingRate::VRS_1x2: return "VRS_1x2";
    case ERHIShadingRate::VRS_2x1: return "VRS_2x1";
    case ERHIShadingRate::VRS_2x2: return "VRS_2x2";
    case ERHIShadingRate::VRS_2x4: return "VRS_2x4";
    case ERHIShadingRate::VRS_4x2: return "VRS_4x2";
    case ERHIShadingRate::VRS_4x4: return "VRS_4x4";
    default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIDepthStencil

struct SRHIDepthStencil
{
    SRHIDepthStencil() = default;

    FORCEINLINE SRHIDepthStencil(float InDepth, uint8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    {
    }

    FORCEINLINE bool operator==(const SRHIDepthStencil& RHS) const
    {
        return (Depth == RHS.Depth) && (Stencil && RHS.Stencil);
    }

    FORCEINLINE bool operator!=(const SRHIDepthStencil& RHS) const
    {
        return !(*this == RHS);
    }

    float Depth   = 1.0f;
    uint8 Stencil = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIClearValue

class CRHIClearValue
{
public:

    enum class EType : uint8
    {
        Color        = 1,
        DepthStencil = 2
    };

    // NOTE: Default clear color is black
    FORCEINLINE CRHIClearValue()
        : Type(EType::Color)
        , Format(ERHIFormat::Unknown)
        , Color(0.0f, 0.0f, 0.0f, 1.0f)
    {
    }

    FORCEINLINE CRHIClearValue(ERHIFormat InFormat, float Depth, uint8 Stencil)
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencil(Depth, Stencil)
    {
    }

    FORCEINLINE CRHIClearValue(ERHIFormat InFormat, float r, float g, float b, float a)
        : Type(EType::Color)
        , Format(InFormat)
        , Color(r, g, b, a)
    {
    }

    FORCEINLINE CRHIClearValue(const CRHIClearValue& Other)
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

    FORCEINLINE SColorF& AsColor()
    {
        Assert(Type == EType::Color);
        return Color;
    }

    FORCEINLINE const SColorF& AsColor() const
    {
        Assert(Type == EType::Color);
        return Color;
    }

    FORCEINLINE SRHIDepthStencil& AsDepthStencil()
    {
        Assert(Type == EType::DepthStencil);
        return DepthStencil;
    }

    FORCEINLINE const SRHIDepthStencil& AsDepthStencil() const
    {
        Assert(Type == EType::DepthStencil);
        return DepthStencil;
    }

    FORCEINLINE EType GetType() const
    {
        return Type;
    }

    FORCEINLINE ERHIFormat GetFormat() const
    {
        return Format;
    }

    FORCEINLINE CRHIClearValue& operator=(const CRHIClearValue& RHS)
    {
        Type   = RHS.Type;
        Format = RHS.Format;

        if (RHS.Type == EType::Color)
        {
            Color = RHS.Color;
        }
        else if (RHS.Type == EType::DepthStencil)
        {
            DepthStencil = RHS.DepthStencil;
        }

        return *this;
    }

    FORCEINLINE bool operator==(const CRHIClearValue& RHS) const
    {
        if (Type != RHS.Type)
        {
            return false;
        }

        if (Format != RHS.Format)
        {
            return false;
        }

        if (Type == EType::Color)
        {
            return (Color == RHS.Color);
        }

        return (DepthStencil == RHS.DepthStencil);
    }

    FORCEINLINE bool operator!=(const CRHIClearValue& RHS) const
    {
        return !(*this == RHS);
    }

private:
    EType      Type;
    ERHIFormat Format;
    
    union
    {
        SColorF          Color;
        SRHIDepthStencil DepthStencil;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResourceData

class CRHIResourceData
{
public:

    FORCEINLINE CRHIResourceData()
        : Data(nullptr)
    {
    }

    FORCEINLINE CRHIResourceData(const void* InData, uint32 InSizeInBytes)
        : Data(InData)
        , SizeInBytes(InSizeInBytes)
    {
    }

    FORCEINLINE CRHIResourceData(const void* InData, ERHIFormat InFormat, uint32 InWidth)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(0)
    {
    }

    FORCEINLINE CRHIResourceData(const void* InData, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    FORCEINLINE void Set(const void* InData, uint32 InSizeInBytes)
    {
        Data        = InData;
        SizeInBytes = InSizeInBytes;
    }

    FORCEINLINE void Set(const void* InData, ERHIFormat InFormat, uint32 InWidth)
    {
        Data   = InData;
        Format = InFormat;
        Width  = InWidth;
    }

    FORCEINLINE void Set(const void* InData, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
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

    FORCEINLINE bool operator==(const CRHIResourceData& RHS) const
    {
        return (Data == RHS.Data) && (Format == RHS.Format) && (Width == RHS.Width) && (Height == RHS.Height);
    }

    FORCEINLINE bool operator!=(const CRHIResourceData& RHS) const
    {
        return !(*this == RHS);
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
            ERHIFormat Format;
            uint32     Width;
            uint32     Height;
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
    {
    }

    FORCEINLINE bool operator==(const SRHICopyBufferInfo& RHS) const
    {
        return (SourceOffset == RHS.SourceOffset) && (DestinationOffset == RHS.DestinationOffset) && (SizeInBytes == RHS.SizeInBytes);
    }

    FORCEINLINE bool operator!=(const SRHICopyBufferInfo& RHS) const
    {
        return !(*this == RHS);
    }

    uint64 SourceOffset      = 0;
    uint32 DestinationOffset = 0;
    uint32 SizeInBytes       = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureSubresourceInfo

struct SRHICopyTextureSubresourceInfo
{
    SRHICopyTextureSubresourceInfo() = default;

    FORCEINLINE SRHICopyTextureSubresourceInfo(uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex)
        : SubresourceIndex(InSubresourceIndex)
        , x(InX)
        , y(InY)
        , z(InZ)
    {
    }

    FORCEINLINE bool operator==(const SRHICopyTextureSubresourceInfo& RHS) const
    {
        return (SubresourceIndex == RHS.SubresourceIndex) && (x == RHS.x) && (y == RHS.y) && (z == RHS.z);
    }

    FORCEINLINE bool operator==(const SRHICopyTextureSubresourceInfo& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 SubresourceIndex = 0;
    uint32 x                = 0;
    uint32 y                = 0;
    uint32 z                = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHICopyTextureInfo

struct SRHICopyTextureInfo
{
    SRHICopyTextureInfo() = default;

    FORCEINLINE bool operator==(const SRHICopyTextureInfo& RHS) const
    {
        return (Source == RHS.Source) && (Destination == RHS.Destination) && (Width == RHS.Width) && (Height == RHS.Height) && (Depth == RHS.Depth);
    }

    FORCEINLINE bool operator==(const SRHICopyTextureInfo& RHS) const
    {
        return !(*this == RHS);
    }

    SRHICopyTextureSubresourceInfo Source;
    SRHICopyTextureSubresourceInfo Destination;
    
    uint32 Width  = 0;
    uint32 Height = 0;
    uint32 Depth  = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRenderTargetEntry

struct SRHIRenderTargetEntry
{
    enum class EType : uint8
    {
        Texture = 1,
        View    = 2
    };

    SRHIRenderTargetEntry()
        : Type(EType::Texture)
        , Texture(nullptr)
    {
    }

    explicit SRHIRenderTargetEntry(CRHITexture2D* InTexture)
        : Type(EType::Texture)
        , Texture(InTexture)
    {
    }

    explicit SRHIRenderTargetEntry(CRHIRenderTargetView* InView)
        : Type(EType::View)
        , View(InView)
    {
    }

    SRHIRenderTargetEntry(const SRHIRenderTargetEntry& Other)
        : Type(Other.Type)
        , Texture(Other.Texture)
    {
    }

    SRHIRenderTargetEntry& operator=(const SRHIRenderTargetEntry& RHS)
    {
        Type = RHS.Type;
        Texture = RHS.Texture;
        return *this;
    }

    bool operator==(const SRHIRenderTargetEntry& RHS) const
    {
        return (Type == RHS.Type) && (Texture == RHS.Texture);
    }

    bool operator!=(const SRHIRenderTargetEntry& RHS) const
    {
        return !(*this == RHS);
    }

    EType Type;

    union
    {
        CRHITexture2D*        Texture;
        CRHIRenderTargetView* View;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIDepthStencilEntry

struct SRHIDepthStencilEntry
{
    enum class EType : uint8
    {
        Texture = 1,
        View    = 2
    };

    SRHIDepthStencilEntry()
        : Type(EType::Texture)
        , Texture(nullptr)
    {
    }

    explicit SRHIDepthStencilEntry(CRHITexture2D* InTexture)
        : Type(EType::Texture)
        , Texture(InTexture)
    {
    }

    explicit SRHIDepthStencilEntry(CRHIDepthStencilView* InView)
        : Type(EType::View)
        , View(InView)
    {
    }

    SRHIDepthStencilEntry(const SRHIDepthStencilEntry& Other)
        : Type(Other.Type)
        , Texture(Other.Texture)
    {
    }

    SRHIDepthStencilEntry& operator=(const SRHIDepthStencilEntry& RHS)
    {
        Type = RHS.Type;
        Texture = RHS.Texture;
        return *this;
    }

    bool operator==(const SRHIDepthStencilEntry& RHS) const
    {
        return (Type == RHS.Type) && (Texture == RHS.Texture);
    }

    bool operator!=(const SRHIDepthStencilEntry& RHS) const
    {
        return !(*this == RHS);
    }

    EType Type;

    union
    {
        CRHITexture2D*        Texture;
        CRHIDepthStencilView* View;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderPassDesc

class CRHIRenderPass
{
public:

    CRHIRenderPass()
    {
    }

    CRHIRenderPass(CRHITexture2D* const* InRenderTargets, uint32 InNumRenderTargets, CRHITexture2D* InDepthStencil)
        : RenderTargets()
        , NumRenderTargets(0)
        , DepthStencil(InDepthStencil)
    {
        SetRenderTargets(InRenderTargets, InNumRenderTargets);
    }

    CRHIRenderPass(CRHIRenderTargetView* const* InRenderTargetViews, uint32 InNumRenderTargetViews, CRHIDepthStencilView* InDepthStencilView)
        : RenderTargets()
        , NumRenderTargets(0)
        , DepthStencil(InDepthStencilView)
    {
        SetRenderTargets(InRenderTargetViews, InNumRenderTargetViews);
    }

    void SetRenderTargets(CRHITexture2D* const* InRenderTargets, uint32 InNumRenderTargets)
    {
        Assert(InRenderTargetViews < ArrayCount(RenderTargets));

        for (uint32 Index = 0; Index < InNumRenderTargets; ++Index)
        {
            RenderTargets[Index] = SRHIRenderTargetEntry(InRenderTargets[Index]);
        }

        NumRenderTargets = InNumRenderTargets;
    }

    void SetRenderTargets(CRHIRenderTargetView* const* InRenderTargetViews, uint32 InNumRenderTargetViews)
    {
        Assert(InRenderTargetViews < ArrayCount(RenderTargets));
        
        for (uint32 Index = 0; Index < InNumRenderTargetViews; ++Index)
        {
            RenderTargets[Index] = SRHIRenderTargetEntry(InRenderTargetViews[Index]);
        }

        NumRenderTargets = InNumRenderTargetViews;
    }

    void SetDepthStencil(CRHITexture2D* InDepthStencil)
    {
        DepthStencil = SRHIDepthStencilEntry(InDepthStencil);
    }

    void SetDepthStencil(CRHIDepthStencilView* InDepthStencilView)
    {
        DepthStencil = SRHIDepthStencilEntry(InDepthStencilView);
    }

    bool operator==(const CRHIRenderPass& RHS) const
    {
        if (NumRenderTargets != RHS.NumRenderTargets)
        {
            return false;
        }

        for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
        {
            if (RenderTargets[Index] != RHS.RenderTargets[Index])
            {
                return false;
            }
        }

        return (DepthStencil == RHS.DepthStencil);
    }

    bool operator==(const CRHIRenderPass& RHS) const
    {
        return !(*this == RHS);
    }

    SRHIRenderTargetEntry RenderTargets[8];
    uint32                NumRenderTargets;
    SRHIDepthStencilEntry DepthStencil;
};
