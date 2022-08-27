#pragma once
#include "Core/Math/Color.h"
#include "Core/Templates/EnumUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EFormat

enum class EFormat : uint16
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

CONSTEXPR const CHAR* ToString(EFormat Format)
{
    switch (Format)
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
        default:                                return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

CONSTEXPR uint32 GetByteStrideFromFormat(EFormat Format)
{
    switch (Format)
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

CONSTEXPR uint32 GetCubeFaceIndex(ECubeFace CubeFace)
{
    return static_cast<uint32>(CubeFace);
}

CONSTEXPR ECubeFace GetCubeFaceFromIndex(uint32 Index)
{
    return (Index > GetCubeFaceIndex(ECubeFace::NegZ)) ? static_cast<ECubeFace>(-1) : static_cast<ECubeFace>(Index);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EComparisonFunc

enum class EComparisonFunc
{
    Unknown      = 0,
    Never        = 1,
    Less         = 2,
    Equal        = 3,
    LessEqual    = 4,
    Greater      = 5,
    NotEqual     = 6,
    GreaterEqual = 7,
    Always       = 8
};

CONSTEXPR const CHAR* ToString(EComparisonFunc ComparisonFunc)
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

CONSTEXPR const CHAR* ToString(EPrimitiveTopologyType PrimitveTopologyType)
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

CONSTEXPR const CHAR* ToString(EResourceAccess ResourceState)
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

CONSTEXPR const CHAR* ToString(EPrimitiveTopology ResourceState)
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

CONSTEXPR const CHAR* ToString(EShadingRate ShadingRate)
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

CONSTEXPR const CHAR* ToString(EDescriptorType DescriptorType)
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
// FRHIDescriptorHandle

class FRHIDescriptorHandle
{
    enum : uint32
    {
		// Be specific in order to cancel warnings about truncation
        InvalidHandle = ((1 << 24) - 1)
    };

public:

    /**
     * @brief: Default Constructor
     */
    FRHIDescriptorHandle()
        : Index(InvalidHandle)
	    , Type(EDescriptorType::Unknown)
    { }

    /**
     * @brief: Constructor that creates a descriptor-handle
     *
     * @param InType: Type of descriptor
     * @param InIndex: Index to identify the descriptor-handle inside the backend (Descriptor-Heap)
     */
    FRHIDescriptorHandle(EDescriptorType InType, uint32 InIndex)
        : Index(InIndex)
	    , Type(InType)
    { }

    /** @return: Returns true if the handle is valid */
    bool IsValid() const { return (Type != EDescriptorType::Unknown) && (Index != InvalidHandle); }

    /**
     * @brief: Compare two descriptor-handles to see if the reference the same resource
     *
     * @return: Returns true if the handles are equal
     */
    bool operator==(const FRHIDescriptorHandle& RHS) const
    {
        return (Type == RHS.Type) && (Index == RHS.Index);
    }

    /**
     * @brief: Compare two descriptor-handles to see if the reference the same resource
     *
     * @return: Returns false if the handles are equal
     */
    bool operator!=(const FRHIDescriptorHandle& RHS) const
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
// FTextureDepthStencilValue

struct FTextureDepthStencilValue
{
    /**
     * @brief: Default Constructor
     */
    FTextureDepthStencilValue()
        : Depth(1.0f)
        , Stencil(0)
    { }

    /**
     * @brief: Constructor taking depth and stencil value
     *
     * @param InDepth: Depth-value
     * @param InStencil: Stencil-value
     */
    FTextureDepthStencilValue(float InDepth, uint8 InStencil)
        : Depth(InDepth)
        , Stencil(InStencil)
    { }

    /** @return: Returns and calculates the hash for this type */
    uint64 GetHash() const
    {
        uint64 Hash = Stencil;
        HashCombine(Hash, Depth);
        return Hash;
    }

    /**
     * @brief: Compare with another instance
     *
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const FTextureDepthStencilValue& RHS) const
    {
        return (Depth == RHS.Depth) && (Stencil && RHS.Stencil);
    }

    /**
     * @brief: Compare with another instance
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const FTextureDepthStencilValue& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Value to clear the depth portion of a texture with */
    float Depth;

    /** @brief: Value to clear the stencil portion of a texture with */
    uint8 Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureClearValue

struct FTextureClearValue
{
    enum class EType : uint8
    {
        Color        = 1,
        DepthStencil = 2,
    };

    /**
     * @brief: Default Constructor that creates a black clear color
     */
    FTextureClearValue()
        : Type(EType::Color)
        , Format(EFormat::Unknown)
        , ColorValue(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    /**
     * @brief: Constructor that creates a DepthStencil-ClearValue
     *
     * @param InFormat: Format to clear
     * @param InDepth: Depth-value
     * @param InStencil: Stencil-value
     */
    FTextureClearValue(EFormat InFormat, float InDepth, uint8 InStencil)
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencilValue(InDepth, InStencil)
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
    FTextureClearValue(EFormat InFormat, float InR, float InG, float InB, float InA)
        : Type(EType::Color)
        , Format(InFormat)
        , ColorValue(InR, InG, InB, InA)
    { }

    /**
     * @brief: Copy-constructor
     *
     * @param Other: Instance to copy
     */
    FTextureClearValue(const FTextureClearValue& Other)
        : Type(Other.Type)
        , Format(Other.Format)
        , ColorValue()
    {
        if (Other.IsColorValue())
        {
            ColorValue = Other.ColorValue;
        }
        else
        {
            Check(Other.IsDepthStencilValue());
            DepthStencilValue = Other.DepthStencilValue;
        }
    }

    /** @return: Returns a true if the value is a FloatColor */
    bool IsColorValue() const { return (Type == EType::Color); }

    /** @return: Returns a true if the value is a DepthStencilClearValue */
    bool IsDepthStencilValue() const { return (Type == EType::DepthStencil); }

    /** @return: Returns a FloatColor */
    FFloatColor& AsColor()
    {
        Check(IsColorValue());
        return ColorValue;
    }

    /** @return: Returns a FloatColor */
    const FFloatColor& AsColor() const
    {
        Check(IsColorValue());
        return ColorValue;
    }

    /** @return: Returns a DepthStencilClearValue */
    FTextureDepthStencilValue& AsDepthStencil()
    {
        Check(IsDepthStencilValue());
        return DepthStencilValue;
    }

    /** @return: Returns a DepthStencilClearValue */
    const FTextureDepthStencilValue& AsDepthStencil() const
    {
        Check(IsDepthStencilValue());
        return DepthStencilValue;
    }

    /**
     * @brief: Copy-assignment operator
     *
     * @param RHS: Instance to copy
     * @return: Returns a reference to this instance
     */
    FTextureClearValue& operator=(const FTextureClearValue& RHS)
    {
        Type = RHS.Type;
        Format = RHS.Format;

        if (RHS.IsColorValue())
        {
            ColorValue = RHS.ColorValue;
        }
        else
        {
            Check(RHS.IsDepthStencilValue());
            DepthStencilValue = RHS.DepthStencilValue;
        }

        return *this;
    }

    /**
     * @brief: Compare with another instance
     *
     * @param RHS: Instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const FTextureClearValue& RHS) const
    {
        if ((Type != RHS.Type) || (Format != RHS.Format))
        {
            return false;
        }

        if (IsColorValue())
        {
            return (ColorValue == RHS.ColorValue);
        }

        Check(IsDepthStencilValue());
        return (DepthStencilValue == RHS.DepthStencilValue);
    }

    /**
     * @brief: Compare with another instance
     *
     * @param RHS: Instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const FTextureClearValue& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Type of ClearValue */
    EType Type;

    /** @brief: Format of the ClearValue */
    EFormat Format;

    union
    {
        /** @brief: Color-value */
        FFloatColor ColorValue;

        /** @brief: DepthStencil-value */
        FTextureDepthStencilValue DepthStencilValue;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICopyBufferInfo

struct FRHICopyBufferInfo
{
    FRHICopyBufferInfo() = default;

    FORCEINLINE FRHICopyBufferInfo(uint64 InSourceOffset, uint32 InDestinationOffset, uint32 InSizeInBytes)
        : SourceOffset(InSourceOffset)
        , DestinationOffset(InDestinationOffset)
        , SizeInBytes(InSizeInBytes)
    { }

    uint64 SourceOffset = 0;
    uint32 DestinationOffset = 0;
    uint32 SizeInBytes = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICopyTextureSubresourceInfo

struct FRHICopyTextureSubresourceInfo
{
    FRHICopyTextureSubresourceInfo() = default;

    FORCEINLINE FRHICopyTextureSubresourceInfo(uint32 InX, uint32 InY, uint32 InZ, uint32 InSubresourceIndex)
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
// FRHICopyTextureInfo

struct FRHICopyTextureInfo
{
    FRHICopyTextureSubresourceInfo Source;
    FRHICopyTextureSubresourceInfo Destination;
    uint32 Width  = 0;
    uint32 Height = 0;
    uint32 Depth  = 0;
};
