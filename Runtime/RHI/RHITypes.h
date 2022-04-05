#pragma once
#include "RHICore.h"

#include "Core/Math/Color.h"
#include "Core/Templates/EnumUtilities.h"

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
// ERayTracingStructureBuildType

enum class ERayTracingStructureBuildType : uint8
{
    Update = 1,
    Build  = 2,
};

inline const char* ToString(ERayTracingStructureBuildType BuildType)
{
    switch (BuildType)
    {
    case ERayTracingStructureBuildType::Update: return "Update";
    case ERayTracingStructureBuildType::Build:  return "Build";
    default:                                    return "Unknown";
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

class CRHIDepthStencilClearValue
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIDepthStencilClearValue()
        : Depth(1.0f)
        , Stencil(0)
    { }

    /**
     * @brief: Constructor taking depth and stencil value
     * 
     * @param InDepth: Depth-value
     * @param InStencil: Stencil-value
     */
    CRHIDepthStencilClearValue(float InDepth, uint8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    { }

    /**
     * @brief: Compare with another instance
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIDepthStencilClearValue& RHS) const
    {
        return (Depth == RHS.Depth) && (Stencil && RHS.Stencil);
    }

    /**
     * @brief: Compare with another instance
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIDepthStencilClearValue& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Value to clear the depth portion of a texture with */
    float Depth;

    /** @brief: Value to clear the stencil portion of a texture with */
    uint8 Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureClearValue

class CRHITextureClearValue
{
public:

    enum class EType : uint8
    {
        Color        = 1,
        DepthStencil = 2
    };

    /**
     * @brief: Default Constructor that creates a black clear color
     */
    CRHITextureClearValue()
        : Type(EType::Color)
        , Format(ERHIFormat::Unknown)
        , Color(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    /**
     * @brief: Constructor that creates a DepthStencil-ClearValue
     * 
     * @param InFormat: Format to clear
     * @param InDepth: Depth-value
     * @param InStencil: Stencil-value
     */
    CRHITextureClearValue(ERHIFormat InFormat, float InDepth, uint8 InStencil)
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencil(Depth, Stencil)
    { }

    /**
     * @brief: Constructor that creates Color-ClearValue
     * 
     * @param InFormat: Format to clear
     * @param InR: Red-Channel value
     * @param InG: Green-Channel value
     * @param InB: Blue-Channel value
     * @param InA: Alpha-Channel value
     */
    CRHITextureClearValue(ERHIFormat InFormat, float InR, float InG, float InB, float InA)
        : Type(EType::Color)
        , Format(InFormat)
        , Color(r, g, b, a)
    { }

    /**
     * @brief: Copy-constructor
     * 
     * @param Other: Instance to copy
     */
    CRHITextureClearValue(const CRHITextureClearValue& Other)
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

    /**
     * @brief: Check if the clear value is a Color-Value
     * 
     * @return: Returns a true if the value is a FloatColor
     */
    bool IsColorValue() const { return (Type == EType::Color); }

        /**
     * @brief: Check if the clear value is a DepthStencil-Value
     * 
     * @return: Returns a true if the value is a DepthStencilClearValue
     */
    bool IsDepthStencilValue() const { return (Type == EType::DepthStencil); }

    /**
     * @brief: Retrieve Color-Value
     * 
     * @return: Returns a FloatColor
     */
    CFloatColor& AsColor()
    {
        Check(IsColorValue());
        return Color;
    }

    /**
     * @brief: Retrieve Color-Value
     * 
     * @return: Returns a FloatColor
     */
    const CFloatColor& AsColor() const
    {
        Check(IsColorValue());
        return Color;
    }

    /**
     * @brief: Retrieve DepthStencil-Value
     * 
     * @return: Returns a DepthStencilClearValue
     */
    CRHIDepthStencilClearValue& AsDepthStencil()
    {
        Check(IsDepthStencilValue());
        return DepthStencil;
    }

    /**
     * @brief: Retrieve DepthStencil-Value
     * 
     * @return: Returns a DepthStencilClearValue
     */
    const CRHIDepthStencilClearValue& AsDepthStencil() const
    {
        Check(IsDepthStencilValue());
        return DepthStencil;
    }

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: Instance to copy
     * @return: Returns a reference to this instance
     */
    CRHITextureClearValue& operator=(const CRHITextureClearValue& RHS)
    {
        Type   = RHS.Type;
        Format = RHS.Format;

        if (RHS.IsColorValue())
        {
            Color = RHS.Color;
        }
        else if (RHS.IsDepthStencilValue())
        {
            DepthStencil = RHS.DepthStencil;
        }
        else
        {
            Check(false);
        }

        return *this;
    }

    /**
     * @brief: Compare with another instance
     * 
     * @param RHS: Instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHITextureClearValue& RHS) const
    {
        if ((Type != RHS.Type) || (Format != RHS.Format))
        {
            return false;
        }

        if (IsColorValue())
        {
            return (Color == RHS.Color);
        }
        else if (IsDepthStencilValue())
        {
            return (DepthStencil == RHS.DepthStencil);
        }

        Check(false);
        return false;
    }

    /**
     * @brief: Compare with another instance
     * 
     * @param RHS: Instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHITextureClearValue& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Type of ClearValue */
    EType Type;

    /** @brief: Format of the ClearValue */
    ERHIFormat Format;
    
    union
    {
        /** @brief: Color-value */
        CFloatColor Color;

        /** @brief: DepthStencil-value */
        CRHIDepthStencilClearValue DepthStencil;
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
