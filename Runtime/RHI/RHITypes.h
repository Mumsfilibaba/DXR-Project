#pragma once
#include "RHICore.h"
#include "TextureFormat.h"

#include "Core/Math/Color.h"
#include "Core/Templates/EnumUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShadingRateTier

enum class EShadingRateTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier2        = 2,
};

inline const char* ToString(EShadingRateTier Tier)
{
    switch (Tier)
    {
    case EShadingRateTier::NotSupported: return "NotSupported";
    case EShadingRateTier::Tier1:        return "Tier1";
    case EShadingRateTier::Tier2:        return "Tier2";
    default:                             return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShadingRateSupport

struct SRHIShadingRateSupport
{
    SRHIShadingRateSupport()
        : Tier(EShadingRateTier::NotSupported)
        , ShadingRateImageTileSize(0)
    { }

    bool operator==(const SRHIShadingRateSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (ShadingRateImageTileSize == RHS.ShadingRateImageTileSize);
    }

    bool operator!=(const SRHIShadingRateSupport& RHS) const
    {
        return !(*this == RHS);
    }

    EShadingRateTier Tier;
    uint8            ShadingRateImageTileSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingTier

enum class ERayTracingTier : uint8
{
    NotSupported = 0,
    Tier1        = 1,
    Tier1_1      = 2,
};

inline const char* ToString(ERayTracingTier Tier)
{
    switch (Tier)
    {
    case ERayTracingTier::NotSupported: return "NotSupported";
    case ERayTracingTier::Tier1:        return "Tier1";
    case ERayTracingTier::Tier1_1:      return "Tier1_1";
    default:                            return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRayTracingSupport

struct SRHIRayTracingSupport
{
    SRHIRayTracingSupport()
        : Tier(ERayTracingTier::NotSupported)
        , MaxRecursionDepth(0)
    { }

    bool operator==(const SRHIRayTracingSupport& RHS) const
    {
        return (Tier == RHS.Tier) && (MaxRecursionDepth == RHS.MaxRecursionDepth);
    }

    bool operator!=(const SRHIRayTracingSupport& RHS) const
    {
        return !(*this == RHS);
    }

    ERayTracingTier Tier;
    uint16          MaxRecursionDepth;
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ECubeFace

enum class ECubeFace : uint8
{
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5,
};

inline auto GetCubeFaceIndex(ECubeFace CubeFace)
{
    return static_cast<TUnderlyingType<ECubeFace>::Type>(CubeFace);
}

inline ECubeFace GetCubeFaceFromIndex(uint32 Index)
{
    return (Index > GetCubeFaceIndex(ECubeFace::NegZ)) ? static_cast<ECubeFace>(-1) : static_cast<ECubeFace>(Index);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EComparisonFunc

enum class EComparisonFunc : uint8
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

enum class EPrimitiveTopologyType : uint8
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

enum class EResourceAccess
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

inline const char* ToString(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
    case EResourceAccess::Common:                          return "Common";
    case EResourceAccess::VertexAndConstantBuffer:         return "VertexAndConstantBuffer";
    case EResourceAccess::IndexBuffer:                     return "IndexBuffer";
    case EResourceAccess::RenderTarget:                    return "RenderTarget";
    case EResourceAccess::RenderTargetClear:               return "RenderTargetClear";
    case EResourceAccess::UnorderedAccess:                 return "UnorderedAccess";
    case EResourceAccess::DepthClear:                      return "DepthClear";
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
// EAttachmentLoadAction

enum class EAttachmentLoadAction : uint8
{
    None  = 0, // Don't care 
    Load  = 1, // Use the stored data when RenderPass begin
    Clear = 2, // Clear data when RenderPass begin
};

inline const char* ToString(EAttachmentLoadAction LoadAction)
{
    switch (LoadAction)
    {
    case EAttachmentLoadAction::None:  return "None";
    case EAttachmentLoadAction::Load:  return "Load";
    case EAttachmentLoadAction::Clear: return "Clear";
    default:                           return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EAttachmentStoreAction

enum class EAttachmentStoreAction : uint8
{
    None  = 0, // Don't care 
    Store = 1, // Store the data after the RenderPass is finished
};

inline const char* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
    case EAttachmentStoreAction::None:  return "None";
    case EAttachmentStoreAction::Store: return "Store";
    default:                            return "Unknown";
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
};

inline const char* ToString(EDescriptorType DescriptorType)
{
    switch (DescriptorType)
    {
    case EDescriptorType::UnorderedAccess: return "UnorderedAccess";
    case EDescriptorType::ShaderResource:  return "ShaderResource";
    default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDescriptorHandle

class CRHIDescriptorHandle
{
    enum 
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

    /**
     * @brief: Check if the descriptor-handle is valid, that is it has a valid type and index
     * 
     * @return: Returns true if the handle is valid
     */
    bool IsValid() const 
    { 
        return (Type != EDescriptorType::Unknown) && (Index != InvalidHandle);
    }

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
// SRHIDepthStencil

struct SRHIDepthStencilValue
{
    SRHIDepthStencilValue() = default;

    SRHIDepthStencilValue(float InDepth, uint8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    { }

    bool operator==(const SRHIDepthStencilValue& RHS) const
    {
        return (Depth == RHS.Depth) && (Stencil && RHS.Stencil);
    }

    bool operator!=(const SRHIDepthStencilValue& RHS) const
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

    CRHIClearValue()
        : Type(EType::Color)
        , Format(ERHIFormat::Unknown)
        , Color(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    CRHIClearValue(ERHIFormat InFormat, float Depth, uint8 Stencil)
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencil(Depth, Stencil)
    { }

    CRHIClearValue(ERHIFormat InFormat, float r, float g, float b, float a)
        : Type(EType::Color)
        , Format(InFormat)
        , Color(r, g, b, a)
    { }

    CRHIClearValue(const CRHIClearValue& Other)
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

    SColorF& AsColor()
    {
        Check(Type == EType::Color);
        return Color;
    }

    const SColorF& AsColor() const
    {
        Check(Type == EType::Color);
        return Color;
    }

    SRHIDepthStencilValue& AsDepthStencil()
    {
        Check(Type == EType::DepthStencil);
        return DepthStencil;
    }

    const SRHIDepthStencilValue& AsDepthStencil() const
    {
        Check(Type == EType::DepthStencil);
        return DepthStencil;
    }

    EType GetType() const
    {
        return Type;
    }

    ERHIFormat GetFormat() const
    {
        return Format;
    }

    CRHIClearValue& operator=(const CRHIClearValue& RHS)
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

    bool operator==(const CRHIClearValue& RHS) const
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

    bool operator!=(const CRHIClearValue& RHS) const
    {
        return !(*this == RHS);
    }

private:
    EType      Type;
    ERHIFormat Format;
    
    union
    {
        SColorF               Color;
        SRHIDepthStencilValue DepthStencil;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResourceData

class CRHIResourceData
{
public:

    CRHIResourceData()
        : Data(nullptr)
    { }

    CRHIResourceData(const void* InData, uint32 InSizeInBytes)
        : Data(InData)
        , SizeInBytes(InSizeInBytes)
    { }

    CRHIResourceData(const void* InData, ERHIFormat InFormat, uint32 InWidth)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(0)
    { }

    CRHIResourceData(const void* InData, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
        : Data(InData)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
    { }

    void Set(const void* InData, uint32 InSizeInBytes)
    {
        Data        = InData;
        SizeInBytes = InSizeInBytes;
    }

    void Set(const void* InData, ERHIFormat InFormat, uint32 InWidth)
    {
        Data   = InData;
        Format = InFormat;
        Width  = InWidth;
    }

    void Set(const void* InData, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
    {
        Set(InData, InFormat, InWidth);
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
        return GetByteStrideFromFormat(Format) * Width;
    }

    uint32 GetSlicePitch() const
    {
        return GetByteStrideFromFormat(Format) * Width * Height;
    }

    bool operator==(const CRHIResourceData& RHS) const
    {
        return (Data   == RHS.Data) 
            && (Format == RHS.Format) 
            && (Width  == RHS.Width) 
            && (Height == RHS.Height);
    }

    bool operator!=(const CRHIResourceData& RHS) const
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
    SRHICopyBufferInfo()
        : SourceOffset(0)
        , DestinationOffset(0)
        , SizeInBytes(0)
    { }

    SRHICopyBufferInfo(uint64 InSourceOffset, uint32 InDestinationOffset, uint32 InSizeInBytes)
        : SourceOffset(InSourceOffset)
        , DestinationOffset(InDestinationOffset)
        , SizeInBytes(InSizeInBytes)
    { }

    bool operator==(const SRHICopyBufferInfo& RHS) const
    {
        return (SourceOffset      == RHS.SourceOffset) 
            && (DestinationOffset == RHS.DestinationOffset) 
            && (SizeInBytes       == RHS.SizeInBytes);
    }

    bool operator!=(const SRHICopyBufferInfo& RHS) const
    {
        return !(*this == RHS);
    }

    uint64 SourceOffset      = 0;
    uint32 DestinationOffset = 0;
    uint32 SizeInBytes       = 0;
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
    { }

    explicit SRHIRenderTargetEntry(CRHITexture* InTexture)
        : Type(EType::Texture)
        , Texture(InTexture)
    { }

    explicit SRHIRenderTargetEntry(CRHIRenderTargetView* InView)
        : Type(EType::View)
        , View(InView)
    { }

    SRHIRenderTargetEntry(const SRHIRenderTargetEntry& Other)
        : Type(Other.Type)
        , Texture(Other.Texture)
    { }

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
        CRHITexture*        Texture;
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
    { }

    explicit SRHIDepthStencilEntry(CRHITexture* InTexture)
        : Type(EType::Texture)
        , Texture(InTexture)
    { }

    explicit SRHIDepthStencilEntry(CRHIDepthStencilView* InView)
        : Type(EType::View)
        , View(InView)
    { }

    SRHIDepthStencilEntry(const SRHIDepthStencilEntry& Other)
        : Type(Other.Type)
        , Texture(Other.Texture)
    { }

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
        CRHITexture*        Texture;
        CRHIDepthStencilView* View;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderPassDesc

class CRHIRenderPass
{
public:

    CRHIRenderPass()
        : RenderTargets()
        , NumRenderTargets(0)
        , DepthStencil()
    { }

    CRHIRenderPass(CRHITexture* const* InRenderTargets, uint32 InNumRenderTargets, CRHITexture* InDepthStencil)
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

    void SetRenderTargets(CRHITexture* const* InRenderTargets, uint32 InNumRenderTargets)
    {
        Check(InRenderTargetViews < ArrayCount(RenderTargets));

        for (uint32 Index = 0; Index < InNumRenderTargets; ++Index)
        {
            RenderTargets[Index] = SRHIRenderTargetEntry(InRenderTargets[Index]);
        }

        NumRenderTargets = InNumRenderTargets;
    }

    void SetRenderTargets(CRHIRenderTargetView* const* InRenderTargetViews, uint32 InNumRenderTargetViews)
    {
        Check(InRenderTargetViews < ArrayCount(RenderTargets));
        
        for (uint32 Index = 0; Index < InNumRenderTargetViews; ++Index)
        {
            RenderTargets[Index] = SRHIRenderTargetEntry(InRenderTargetViews[Index]);
        }

        NumRenderTargets = InNumRenderTargetViews;
    }

    void SetDepthStencil(CRHITexture* InDepthStencil)
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
