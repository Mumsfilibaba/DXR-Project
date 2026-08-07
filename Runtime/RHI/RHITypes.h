#pragma once
#include "Core/Math/Color.h"
#include "Core/Math/IntVector3.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHICore.h"

class FRHIBuffer;
class FRHITexture;
struct FRHIGeometryAccelerationStructureInstance;
class FRHIShader;
class FRHIVertexShader;
class FRHIHullShader;
class FRHIDomainShader;
class FRHIGeometryShader;
class FRHIPixelShader;
class FRHIMeshShader;
class FRHIAmplificationShader;
class FRHIComputeShader;
class FRHIRayTracingShader;
class FRHIRayGenShader;
class FRHIRayCallableShader;
class FRHIRayMissShader;
class FRHIRayAnyHitShader;
class FRHIRayClosestHitShader;
class FRHIRayIntersectionShader;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIRenderTargetView;
class FRHIDepthStencilView;
class FRHIFence;
struct IRHITextureData;

typedef TSharedRef<class FRHIBuffer>                  FRHIBufferRef;
typedef TSharedRef<class FRHITexture>                 FRHITextureRef;
typedef TSharedRef<FRHIShaderResourceView>            FRHIShaderResourceViewRef;
typedef TSharedRef<FRHIUnorderedAccessView>           FRHIUnorderedAccessViewRef;
typedef TSharedRef<FRHIRenderTargetView>              FRHIRenderTargetViewRef;
typedef TSharedRef<FRHIDepthStencilView>              FRHIDepthStencilViewRef;
typedef TSharedRef<class FRHISamplerState>            FRHISamplerStateRef;
typedef TSharedRef<class FRHISwapChain>               FRHISwapChainRef;
typedef TSharedRef<class FRHIQuery>                   FRHIQueryRef;
typedef TSharedRef<class FRHIFence>                   FRHIFenceRef;
typedef TSharedRef<class FRHIRasterizerState>         FRHIRasterizerStateRef;
typedef TSharedRef<class FRHIBlendState>              FRHIBlendStateRef;
typedef TSharedRef<class FRHIDepthStencilState>       FRHIDepthStencilStateRef;
typedef TSharedRef<class FRHIInputLayout>             FRHIInputLayoutRef;
typedef TSharedRef<class FRHIGraphicsPipelineState>   FRHIGraphicsPipelineStateRef;
typedef TSharedRef<class FRHIComputePipelineState>    FRHIComputePipelineStateRef;
typedef TSharedRef<class FRHIMeshletPipelineState>    FRHIMeshletPipelineStateRef;
typedef TSharedRef<class FRHIRayTracingPipelineState> FRHIRayTracingPipelineStateRef;

enum class EFormat : uint8
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
    BC1_Typeless          = 71,
    BC1_UNorm             = 72,
    BC1_UNorm_SRGB        = 73,
    BC2_Typeless          = 74,
    BC2_UNorm             = 75,
    BC2_UNorm_SRGB        = 76,
    BC3_Typeless          = 77,
    BC3_UNorm             = 78,
    BC3_UNorm_SRGB        = 79,
    BC4_Typeless          = 80,
    BC4_UNorm             = 81,
    BC4_SNorm             = 82,
    BC5_Typeless          = 83,
    BC5_UNorm             = 84,
    BC5_SNorm             = 85,
    BC6H_Typeless         = 86,
    BC6H_UF16             = 87,
    BC6H_SF16             = 88,
    BC7_Typeless          = 89,
    BC7_UNorm             = 90,
    BC7_UNorm_SRGB        = 91,

    SamplerFeedbackMinMipOpaque        = 189,
    SamplerFeedbackMipRegionUsedOpaque = 190,
};

NODISCARD constexpr const CHAR* ToString(EFormat Format)
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
        case EFormat::BC1_Typeless:             return "BC1_Typeless";
        case EFormat::BC1_UNorm:                return "BC1_UNorm";
        case EFormat::BC1_UNorm_SRGB:           return "BC1_UNorm_SRGB";
        case EFormat::BC2_Typeless:             return "BC2_Typeless";
        case EFormat::BC2_UNorm:                return "BC2_UNorm";
        case EFormat::BC2_UNorm_SRGB:           return "BC2_UNorm_SRGB";
        case EFormat::BC3_Typeless:             return "BC3_Typeless";
        case EFormat::BC3_UNorm:                return "BC3_UNorm";
        case EFormat::BC3_UNorm_SRGB:           return "BC3_UNorm_SRGB";
        case EFormat::BC4_Typeless:             return "BC4_Typeless";
        case EFormat::BC4_UNorm:                return "BC4_UNorm";
        case EFormat::BC4_SNorm:                return "BC4_SNorm";
        case EFormat::BC5_Typeless:             return "BC5_Typeless";
        case EFormat::BC5_UNorm:                return "BC5_UNorm";
        case EFormat::BC5_SNorm:                return "BC5_SNorm";
        case EFormat::BC6H_Typeless:            return "BC6H_Typeless";
        case EFormat::BC6H_UF16:                return "BC6H_UF16";
        case EFormat::BC6H_SF16:                return "BC6H_SF16";
        case EFormat::BC7_Typeless:             return "BC7_Typeless";
        case EFormat::BC7_UNorm:                return "BC7_UNorm";
        case EFormat::BC7_UNorm_SRGB:           return "BC7_UNorm_SRGB";

        case EFormat::SamplerFeedbackMinMipOpaque:        return "SamplerFeedbackMinMipOpaque";
        case EFormat::SamplerFeedbackMipRegionUsedOpaque: return "SamplerFeedbackMipRegionUsedOpaque";

        default: return "Unknown";
    }
}

NODISCARD constexpr uint32 GetByteStrideFromFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Typeless:
        case EFormat::R32G32B32A32_Float:
        case EFormat::R32G32B32A32_Uint:
        case EFormat::R32G32B32A32_Sint:
            return 16;

        case EFormat::R32G32B32_Typeless:
        case EFormat::R32G32B32_Float:
        case EFormat::R32G32B32_Uint:
        case EFormat::R32G32B32_Sint:
            return 12;

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
            return 8;

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
            return 4;

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
            return 2;

        case EFormat::R8_Typeless:
        case EFormat::R8_Unorm:
        case EFormat::R8_Uint:
        case EFormat::R8_Snorm:
        case EFormat::R8_Sint:
            return 1;

        default:
            return 0;
    }
}

NODISCARD constexpr bool IsBlockCompressed(EFormat Format)
{
    return UnderlyingTypeValue(Format) >= UnderlyingTypeValue(EFormat::BC1_Typeless)
        && UnderlyingTypeValue(Format) <= UnderlyingTypeValue(EFormat::BC7_UNorm_SRGB);
}

NODISCARD constexpr bool IsSamplerFeedbackFormat(EFormat Format)
{
    return Format == EFormat::SamplerFeedbackMinMipOpaque
        || Format == EFormat::SamplerFeedbackMipRegionUsedOpaque;
}

NODISCARD constexpr bool IsStencilFormat(EFormat Format)
{
    return Format == EFormat::D24_Unorm_S8_Uint;
}

// BlockCompressed images must be aligned to 4 pixels in all dimensions
NODISCARD constexpr bool IsBlockCompressedAligned(uint32 Extent)
{
    return Extent % 4 == 0;
}

NODISCARD constexpr bool IsTypelessFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Typeless:
        case EFormat::R32G32B32_Typeless:
        case EFormat::R16G16B16A16_Typeless:
        case EFormat::R32G32_Typeless:
        case EFormat::R10G10B10A2_Typeless:
        case EFormat::R8G8B8A8_Typeless:
        case EFormat::R16G16_Typeless:
        case EFormat::R32_Typeless:
        case EFormat::R24G8_Typeless:
        case EFormat::R24_Unorm_X8_Typeless:
        case EFormat::X24_Typeless_G8_Uint:
        case EFormat::R8G8_Typeless:
        case EFormat::R16_Typeless:
        case EFormat::R8_Typeless:
            return true;

        default:
            return false;
    }
}

enum class EColorSpace : uint8
{
    Unknown                   = 0,
    RGB_Full_G22_None_P709    = 1,
    RGB_Full_G10_None_P709    = 2,
    RGB_Full_G2084_None_P2020 = 3,
    RGB_Full_G22_None_P2020   = 4,
};

NODISCARD constexpr const CHAR* ToString(EColorSpace ColorSpace)
{
    switch (ColorSpace)
    {
        case EColorSpace::RGB_Full_G22_None_P709:    return "RGB_Full_G22_None_P709";
        case EColorSpace::RGB_Full_G10_None_P709:    return "RGB_Full_G10_None_P709";
        case EColorSpace::RGB_Full_G2084_None_P2020: return "RGB_Full_G2084_None_P2020";
        case EColorSpace::RGB_Full_G22_None_P2020:   return "RGB_Full_G22_None_P2020";

        default: return "Unknown";
    }
}

enum class EIndexFormat : uint8
{
    Unknown = 0,
    uint16  = 1,
    uint32  = 2,
};

NODISCARD constexpr const CHAR* ToString(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
        case EIndexFormat::uint16: return "uint16";
        case EIndexFormat::uint32: return "uint32";
        
        default: return "Unknown";
    }
}

NODISCARD constexpr EIndexFormat GetIndexFormatFromStride(uint32 StrideInBytes)
{
    switch (StrideInBytes)
    {
        case 2: return EIndexFormat::uint16;
        case 4: return EIndexFormat::uint32;

        default: return EIndexFormat::Unknown;
    }
}

NODISCARD constexpr uint32 GetStrideFromIndexFormat(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
        case EIndexFormat::uint16: return 2;
        case EIndexFormat::uint32: return 4;
        
        default: return 0;
    }
}

enum class ECubeFace : uint8
{
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5,
};

NODISCARD constexpr uint32 GetCubeFaceIndex(ECubeFace CubeFace)
{
    return UnderlyingTypeValue(CubeFace);
}

NODISCARD constexpr ECubeFace GetCubeFaceFromIndex(uint32 Index)
{
    return Index > UnderlyingTypeValue(ECubeFace::NegZ) ? static_cast<ECubeFace>(-1) : static_cast<ECubeFace>(Index);
}

enum class EComparisonFunc : uint8
{
    Unknown      = 0,
    Never        = 1,
    Less         = 2,
    Equal        = 3,
    LessEqual    = 4,
    Greater      = 5,
    NotEqual     = 6,
    GreaterEqual = 7,
    Always       = 8,
};

NODISCARD constexpr const CHAR* ToString(EComparisonFunc ComparisonFunc)
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
    
    default: return "Unknown";
    }
}

enum class ERHIResourceState : uint32
{
    Common                          = 0,
    ConstantBuffer                  = FLAG(0),
    IndexBuffer                     = FLAG(1),
    VertexBuffer                    = FLAG(2),
    RenderTarget                    = FLAG(3),
    UnorderedAccess                 = FLAG(4),
    DepthWrite                      = FLAG(5),
    DepthRead                       = FLAG(6),
    NonPixelShaderResource          = FLAG(7),
    PixelShaderResource             = FLAG(8),
    CopyDest                        = FLAG(9),
    CopySource                      = FLAG(10),
    ResolveDest                     = FLAG(11),
    ResolveSource                   = FLAG(12),
    RayTracingAccelerationStructure = FLAG(13),
    ShadingRateSource               = FLAG(14),
    Present                         = FLAG(15),
    GenericRead                     = FLAG(16),
    StreamOutput                    = FLAG(17),
    IndirectArgument                = FLAG(18),
    ShaderResource                  = NonPixelShaderResource | PixelShaderResource,
};

ENUM_CLASS_OPERATORS(ERHIResourceState);

NODISCARD constexpr const CHAR* ToString(ERHIResourceState ResourceState)
{
    switch (ResourceState)
    {
    case ERHIResourceState::Common:                          return "Common";
    case ERHIResourceState::ConstantBuffer:                  return "ConstantBuffer";
    case ERHIResourceState::IndexBuffer:                     return "IndexBuffer";
    case ERHIResourceState::VertexBuffer:                    return "VertexBuffer";
    case ERHIResourceState::RenderTarget:                    return "RenderTarget";
    case ERHIResourceState::UnorderedAccess:                 return "UnorderedAccess";
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
    case ERHIResourceState::GenericRead:                     return "GenericRead";
    case ERHIResourceState::StreamOutput:                    return "StreamOutput";
    case ERHIResourceState::IndirectArgument:                return "IndirectArgument";
    case ERHIResourceState::ShaderResource:                  return "ShaderResource";
    
    default: return "Unknown";
    }
}

enum class EPrimitiveTopology : uint8
{
    Undefined              = 0,
    PointList              = 1,
    LineList               = 2,
    LineStrip              = 3,
    TriangleList           = 4,
    TriangleStrip          = 5,
    LineListAdjacency      = 6,
    LineStripAdjacency     = 7,
    TriangleListAdjacency  = 8,
    TriangleStripAdjacency = 9,
    PatchList_1            = 10,
    PatchList_2            = 11,
    PatchList_3            = 12,
    PatchList_4            = 13,
    PatchList_5            = 14,
    PatchList_6            = 15,
    PatchList_7            = 16,
    PatchList_8            = 17,
    PatchList_9            = 18,
    PatchList_10           = 19,
    PatchList_11           = 20,
    PatchList_12           = 21,
    PatchList_13           = 22,
    PatchList_14           = 23,
    PatchList_15           = 24,
    PatchList_16           = 25,
    PatchList_17           = 26,
    PatchList_18           = 27,
    PatchList_19           = 28,
    PatchList_20           = 29,
    PatchList_21           = 30,
    PatchList_22           = 31,
    PatchList_23           = 32,
    PatchList_24           = 33,
    PatchList_25           = 34,
    PatchList_26           = 35,
    PatchList_27           = 36,
    PatchList_28           = 37,
    PatchList_29           = 38,
    PatchList_30           = 39,
    PatchList_31           = 40,
    PatchList_32           = 41,
};

static_assert(UnderlyingTypeValue(EPrimitiveTopology::PatchList_32) - UnderlyingTypeValue(EPrimitiveTopology::PatchList_1) == (RHI_MAX_PATCH_CONTROL_POINTS - 1),
    "PatchList enumerators must be contiguous and cover exactly RHI_MAX_PATCH_CONTROL_POINTS counts");

NODISCARD constexpr const CHAR* ToString(EPrimitiveTopology ResourceState)
{
    switch (ResourceState)
    {
    case EPrimitiveTopology::Undefined:              return "Undefined";
    case EPrimitiveTopology::PointList:              return "PointList";
    case EPrimitiveTopology::LineList:               return "LineList";
    case EPrimitiveTopology::LineStrip:              return "LineStrip";
    case EPrimitiveTopology::TriangleList:           return "TriangleList";
    case EPrimitiveTopology::TriangleStrip:          return "TriangleStrip";
    case EPrimitiveTopology::LineListAdjacency:      return "LineListAdjacency";
    case EPrimitiveTopology::LineStripAdjacency:     return "LineStripAdjacency";
    case EPrimitiveTopology::TriangleListAdjacency:  return "TriangleListAdjacency";
    case EPrimitiveTopology::TriangleStripAdjacency: return "TriangleStripAdjacency";
    case EPrimitiveTopology::PatchList_1:            return "PatchList_1";
    case EPrimitiveTopology::PatchList_2:            return "PatchList_2";
    case EPrimitiveTopology::PatchList_3:            return "PatchList_3";
    case EPrimitiveTopology::PatchList_4:            return "PatchList_4";
    case EPrimitiveTopology::PatchList_5:            return "PatchList_5";
    case EPrimitiveTopology::PatchList_6:            return "PatchList_6";
    case EPrimitiveTopology::PatchList_7:            return "PatchList_7";
    case EPrimitiveTopology::PatchList_8:            return "PatchList_8";
    case EPrimitiveTopology::PatchList_9:            return "PatchList_9";
    case EPrimitiveTopology::PatchList_10:           return "PatchList_10";
    case EPrimitiveTopology::PatchList_11:           return "PatchList_11";
    case EPrimitiveTopology::PatchList_12:           return "PatchList_12";
    case EPrimitiveTopology::PatchList_13:           return "PatchList_13";
    case EPrimitiveTopology::PatchList_14:           return "PatchList_14";
    case EPrimitiveTopology::PatchList_15:           return "PatchList_15";
    case EPrimitiveTopology::PatchList_16:           return "PatchList_16";
    case EPrimitiveTopology::PatchList_17:           return "PatchList_17";
    case EPrimitiveTopology::PatchList_18:           return "PatchList_18";
    case EPrimitiveTopology::PatchList_19:           return "PatchList_19";
    case EPrimitiveTopology::PatchList_20:           return "PatchList_20";
    case EPrimitiveTopology::PatchList_21:           return "PatchList_21";
    case EPrimitiveTopology::PatchList_22:           return "PatchList_22";
    case EPrimitiveTopology::PatchList_23:           return "PatchList_23";
    case EPrimitiveTopology::PatchList_24:           return "PatchList_24";
    case EPrimitiveTopology::PatchList_25:           return "PatchList_25";
    case EPrimitiveTopology::PatchList_26:           return "PatchList_26";
    case EPrimitiveTopology::PatchList_27:           return "PatchList_27";
    case EPrimitiveTopology::PatchList_28:           return "PatchList_28";
    case EPrimitiveTopology::PatchList_29:           return "PatchList_29";
    case EPrimitiveTopology::PatchList_30:           return "PatchList_30";
    case EPrimitiveTopology::PatchList_31:           return "PatchList_31";
    case EPrimitiveTopology::PatchList_32:           return "PatchList_32";
    
    default: return "Unknown";
    }
}

NODISCARD constexpr bool IsPatchTopology(EPrimitiveTopology PrimitiveTopology)
{
    return PrimitiveTopology >= EPrimitiveTopology::PatchList_1 && PrimitiveTopology <= EPrimitiveTopology::PatchList_32;
}

NODISCARD constexpr bool IsAdjacencyTopology(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
    case EPrimitiveTopology::LineListAdjacency:
    case EPrimitiveTopology::LineStripAdjacency:
    case EPrimitiveTopology::TriangleListAdjacency:
    case EPrimitiveTopology::TriangleStripAdjacency: return true;
    
    default: return false;
    }
}

NODISCARD constexpr bool IsStripTopology(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
    case EPrimitiveTopology::LineStrip:
    case EPrimitiveTopology::TriangleStrip:
    case EPrimitiveTopology::LineStripAdjacency:
    case EPrimitiveTopology::TriangleStripAdjacency: return true;
    
    default: return false;
    }
}

NODISCARD constexpr uint32 GetNumPatchControlPoints(EPrimitiveTopology PrimitiveTopology)
{
    if (!IsPatchTopology(PrimitiveTopology))
    {
        return 0;
    }

    return uint32(UnderlyingTypeValue(PrimitiveTopology) - UnderlyingTypeValue(EPrimitiveTopology::PatchList_1)) + 1;
}

NODISCARD constexpr EPrimitiveTopology MakePatchListTopology(uint32 NumPatchControlPoints)
{
    if (NumPatchControlPoints == 0 || NumPatchControlPoints > RHI_MAX_PATCH_CONTROL_POINTS)
    {
        return EPrimitiveTopology::Undefined;
    }

    return static_cast<EPrimitiveTopology>(UnderlyingTypeValue(EPrimitiveTopology::PatchList_1) + (NumPatchControlPoints - 1));
}

enum class EShadingRate : uint8
{
    VRS_1x1 = 0x0,
    VRS_1x2 = 0x1,
    VRS_2x1 = 0x4,
    VRS_2x2 = 0x5,
    VRS_2x4 = 0x6,
    VRS_4x2 = 0x9,
    VRS_4x4 = 0xa,
};

NODISCARD constexpr const CHAR* ToString(EShadingRate ShadingRate)
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
    
    default: return "Unknown";
    }
}

enum class EDescriptorType : uint32
{
    Unknown         = 0,
    UnorderedAccess = 1,
    ShaderResource  = 2,
    ConstantBuffer  = 3,
    Sampler         = 4,
};

NODISCARD constexpr const CHAR* ToString(EDescriptorType DescriptorType)
{
    switch (DescriptorType)
    {
        case EDescriptorType::UnorderedAccess: return "UnorderedAccess";
        case EDescriptorType::ShaderResource:  return "ShaderResource";
        case EDescriptorType::ConstantBuffer:  return "ConstantBuffer";
        case EDescriptorType::Sampler:         return "Sampler";
        
        default: return "Unknown";
    }
}

struct FRHIDescriptorHandle
{
    // NOTE: Be specific in terms of bits in order to cancel warnings about truncation
    static constexpr uint32 InvalidHandle = ((1 << 24) - 1);

    constexpr FRHIDescriptorHandle() noexcept
        : Handle(0)
    {
    }

    constexpr FRHIDescriptorHandle(EDescriptorType InType, uint32 InIndex) noexcept
        : Index(InIndex)
        , Type(InType)
    {
    }

    NODISCARD constexpr bool IsValid() const noexcept
    { 
        return Type != EDescriptorType::Unknown && Index != InvalidHandle; 
    }

    constexpr bool operator==(const FRHIDescriptorHandle& Other) const noexcept
    {
        return Handle == Other.Handle;
    }

    constexpr bool operator!=(const FRHIDescriptorHandle& Other) const noexcept
    {
        return Handle != Other.Handle;
    }

    union
    {
        struct
        {
            uint32          Index : 24;
            EDescriptorType Type  : 8;
        };

        uint32 Handle;
    };
};

struct FDepthStencilValue
{
    constexpr FDepthStencilValue() noexcept = default;

    constexpr FDepthStencilValue(float InDepth, uint32 InStencil) noexcept
        : Depth(InDepth)
        , Stencil(InStencil)
    {
    }

    constexpr bool operator==(const FDepthStencilValue& Other) const noexcept = default;

    float  Depth   = 1.0f;
    uint32 Stencil = 0;
};

template<>
struct THash<FDepthStencilValue>
{
    NODISCARD static constexpr uint64 GetHash(const FDepthStencilValue& Value)
    {
        uint64 Result = Value.Stencil;
        HashCombine(Result, Value.Depth);
        return Result;
    }
};

struct FClearValue
{
    enum class EType : uint8
    {
        Color        = 1,
        DepthStencil = 2,
    };

    FClearValue() noexcept
        : Type(EType::Color)
        , Format(EFormat::Unknown)
        , ColorValue(0.0f, 0.0f, 0.0f, 1.0f)
    {
    }

    FClearValue(EFormat InFormat, float InDepth, uint8 InStencil) noexcept
        : Type(EType::DepthStencil)
        , Format(InFormat)
        , DepthStencilValue(InDepth, InStencil)
    {
    }

    FClearValue(EFormat InFormat, float InR, float InG, float InB, float InA) noexcept
        : Type(EType::Color)
        , Format(InFormat)
        , ColorValue(InR, InG, InB, InA)
    {
    }

    FClearValue(const FClearValue& Other) noexcept
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
            CHECK(Other.IsDepthStencilValue());
            DepthStencilValue = Other.DepthStencilValue;
        }
    }

    NODISCARD bool IsColorValue()        const noexcept { return Type == EType::Color; }
    NODISCARD bool IsDepthStencilValue() const noexcept { return Type == EType::DepthStencil; }

    NODISCARD FFloatColor& AsColor() noexcept 
    {
        CHECK(IsColorValue());
        return ColorValue;
    }

    NODISCARD const FFloatColor& AsColor() const noexcept
    {
        CHECK(IsColorValue());
        return ColorValue;
    }

    NODISCARD FDepthStencilValue& AsDepthStencil() noexcept
    {
        CHECK(IsDepthStencilValue());
        return DepthStencilValue;
    }

    NODISCARD const FDepthStencilValue& AsDepthStencil() const noexcept
    {
        CHECK(IsDepthStencilValue());
        return DepthStencilValue;
    }

    FClearValue& operator=(const FClearValue& Other) noexcept
    {
        Type   = Other.Type;
        Format = Other.Format;

        if (Other.IsColorValue())
        {
            ColorValue = Other.ColorValue;
        }
        else
        {
            CHECK(Other.IsDepthStencilValue());
            DepthStencilValue = Other.DepthStencilValue;
        }

        return *this;
    }

    bool operator==(const FClearValue& Other) const noexcept
    {
        if (Type != Other.Type || Format != Other.Format)
        {
            return false;
        }

        if (IsColorValue())
        {
            return ColorValue == Other.ColorValue;
        }

        CHECK(IsDepthStencilValue());
        return DepthStencilValue == Other.DepthStencilValue;
    }

    bool operator!=(const FClearValue& Other) const noexcept
    {
        return !(*this == Other);
    }

    EType   Type;
    EFormat Format;

    union
    {
        FFloatColor        ColorValue;
        FDepthStencilValue DepthStencilValue;
    };
};

struct FBufferRegion
{
    constexpr FBufferRegion() noexcept = default;

    constexpr FBufferRegion(uint64 InOffset, uint64 InSize) noexcept
        : Offset(InOffset)
        , Size(InSize)
    {
    }

    /** The entire buffer. Distinct from the default-constructed region, whose zero Size means "no bytes" to UpdateBuffer */
    NODISCARD static constexpr FBufferRegion Whole() noexcept
    {
        return FBufferRegion(0, RHI_WHOLE_SIZE);
    }

    NODISCARD constexpr bool IsWholeResource() const noexcept
    {
        return Offset == 0 && Size == RHI_WHOLE_SIZE;
    }

    uint64 Offset = 0;
    uint64 Size   = 0;
};

struct FTextureRegion2D
{
    constexpr FTextureRegion2D() noexcept = default;

    constexpr FTextureRegion2D(uint32 InWidth, uint32 InHeight, uint32 InPositionX = 0, uint32 InPositionY = 0) noexcept
        : Width(InWidth)
        , Height(InHeight)
        , PositionX(InPositionX)
        , PositionY(InPositionY)
    {
    }

    uint32 Width     = 0;
    uint32 Height    = 0;
    uint32 PositionX = 0;
    uint32 PositionY = 0;
};

struct FTextureRegion3D
{
    constexpr FTextureRegion3D() noexcept = default;

    constexpr FTextureRegion3D(uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InPositionX = 0, uint32 InPositionY = 0, uint32 InPositionZ = 0) noexcept
        : Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
        , PositionX(InPositionX)
        , PositionY(InPositionY)
        , PositionZ(InPositionZ)
    {
    }

    uint32 Width     = 0;
    uint32 Height    = 0;
    uint32 Depth     = 0;
    uint32 PositionX = 0;
    uint32 PositionY = 0;
    uint32 PositionZ = 0;
};

struct FRHIBufferCopyDesc
{
    constexpr FRHIBufferCopyDesc() noexcept = default;

    constexpr FRHIBufferCopyDesc(uint64 InSrcOffset, uint64 InDstOffset, uint64 InSize) noexcept
        : SrcOffset(InSrcOffset)
        , DstOffset(InDstOffset)
        , Size(InSize)
    {
    }

    uint64 SrcOffset = 0;
    uint64 DstOffset = 0;
    uint64 Size      = 0;
};

struct FRHITextureCopyDesc
{
    IntVector3 DstPosition    = {};
    uint32     DstArraySlice  = 0;
    uint32     DstMipSlice    = 0;
    IntVector3 SrcPosition    = {};
    uint32     SrcArraySlice  = 0;
    uint32     SrcMipSlice    = 0;
    IntVector3 Size           = {};
    uint32     NumArraySlices = 0;
    uint32     NumMipLevels   = 0;
};

struct FViewportRegion
{
    constexpr FViewportRegion() noexcept = default;

    constexpr FViewportRegion(float InWidth, float InHeight, float InPositionX, float InPositionY, float InMinDepth, float InMaxDepth) noexcept
        : Width(InWidth)
        , Height(InHeight)
        , PositionX(InPositionX)
        , PositionY(InPositionY)
        , MinDepth(InMinDepth)
        , MaxDepth(InMaxDepth)
    {
    }

    constexpr bool operator==(const FViewportRegion& Other) const noexcept = default;

    float Width     = 0.0f;
    float Height    = 0.0f;
    float PositionX = 0.0f;
    float PositionY = 0.0f;
    float MinDepth  = 0.0f;
    float MaxDepth  = 1.0f;
};

struct FScissorRegion
{
    constexpr FScissorRegion() noexcept = default;

    constexpr FScissorRegion(float InWidth, float InHeight, float InPositionX, float InPositionY) noexcept
        : Width(InWidth)
        , Height(InHeight)
        , PositionX(InPositionX)
        , PositionY(InPositionY)
    {
    }

    constexpr bool operator==(const FScissorRegion& Other) const noexcept = default;

    float Width     = 0.0f;
    float Height    = 0.0f;
    float PositionX = 0.0f;
    float PositionY = 0.0f;
};

struct FRHISamplePosition
{
    constexpr FRHISamplePosition() noexcept = default;

    constexpr FRHISamplePosition(float InX, float InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    constexpr bool operator==(const FRHISamplePosition& Other) const noexcept = default;

    // Offset from the pixel center in pixels, +Y down. Valid range [-0.5, 0.5).
    float X = 0.0f;
    float Y = 0.0f;
};

struct FRHISamplePositionsDesc
{
    constexpr FRHISamplePositionsDesc() noexcept = default;

    constexpr bool operator==(const FRHISamplePositionsDesc& Other) const noexcept = default;

    // Pixel-major: Positions[PixelIndex * NumSamplesPerPixel + SampleIndex],
    // where PixelIndex walks the GridWidth x GridHeight block in row-major order.
    FRHISamplePosition Positions[RHI_MAX_SAMPLE_POSITIONS] = { };

    // 0 restores the hardware default positions.
    uint8 NumSamplesPerPixel = 0;
    uint8 GridWidth          = 1;
    uint8 GridHeight         = 1;
};

enum class ERayTracingGeometryType : uint8
{
    Triangles       = 0,
    ProceduralAABBs = 1,
};

NODISCARD constexpr const CHAR* ToString(ERayTracingGeometryType Type)
{
    switch (Type)
    {
        case ERayTracingGeometryType::Triangles:       return "Triangles";
        case ERayTracingGeometryType::ProceduralAABBs: return "ProceduralAABBs";
        default:                                       return "Unknown";
    }
}

struct FRHIRayTracingAABB
{
    float MinX, MinY, MinZ;
    float MaxX, MaxY, MaxZ;
};

static_assert(sizeof(FRHIRayTracingAABB) == 24, "FRHIRayTracingAABB must match D3D12_RAYTRACING_AABB / VkAabbPositionsKHR");

struct FRHISceneAccelerationStructureBuildDesc
{
    constexpr FRHISceneAccelerationStructureBuildDesc() noexcept = default;

    constexpr FRHISceneAccelerationStructureBuildDesc(const FRHIGeometryAccelerationStructureInstance* Instances, uint32 NumInstances, bool bUpdate) noexcept
        : Instances(Instances)
        , NumInstances(NumInstances)
        , bUpdate(bUpdate)
    {
    }

    const FRHIGeometryAccelerationStructureInstance* Instances    = nullptr;
    uint32                                           NumInstances = 0;
    bool                                             bUpdate      = false;
};

struct FRHIGeometryAccelerationStructureBuildDesc
{
    constexpr FRHIGeometryAccelerationStructureBuildDesc() noexcept = default;

    constexpr FRHIGeometryAccelerationStructureBuildDesc(FRHIBuffer* VertexBuffer, uint32 NumVertices, FRHIBuffer* IndexBuffer, uint32 NumIndices, EIndexFormat IndexFormat, bool bUpdate) noexcept
        : VertexBuffer(VertexBuffer)
        , NumVertices(NumVertices)
        , IndexBuffer(IndexBuffer)
        , NumIndices(NumIndices)
        , IndexFormat(IndexFormat)
        , bUpdate(bUpdate)
    {
    }

    constexpr FRHIGeometryAccelerationStructureBuildDesc(FRHIBuffer* AABBBuffer, uint32 NumAABBs, uint32 AABBStride, bool bUpdate) noexcept
        : AABBBuffer(AABBBuffer)
        , NumAABBs(NumAABBs)
        , AABBStride(AABBStride)
        , GeometryType(ERayTracingGeometryType::ProceduralAABBs)
        , bUpdate(bUpdate)
    {
    }

    FRHIBuffer*             VertexBuffer = nullptr;
    uint32                  NumVertices  = 0;
    FRHIBuffer*             IndexBuffer  = nullptr;
    uint32                  NumIndices   = 0;
    EIndexFormat            IndexFormat  = EIndexFormat::uint32;
    FRHIBuffer*             AABBBuffer   = nullptr;
    uint32                  NumAABBs     = 0;
    uint32                  AABBStride   = sizeof(FRHIRayTracingAABB);
    ERayTracingGeometryType GeometryType = ERayTracingGeometryType::Triangles;
    bool                    bUpdate      = false;
};

enum class ERHIResourceStateTrackingMode : uint8
{
    /** Backend tracks state per subresource and infers BeforeState. The desc's BeforeState is only validated */
    Tracked = 0,

    /** Resource never transitions. All transition requests targeting it are dropped */
    Static = 1,

    /** Backend never tracks and never implicitly transitions. The desc's BeforeState is used verbatim */
    Manual = 2,
};

NODISCARD constexpr const CHAR* ToString(ERHIResourceStateTrackingMode TrackingMode)
{
    switch (TrackingMode)
    {
    case ERHIResourceStateTrackingMode::Tracked: return "Tracked";
    case ERHIResourceStateTrackingMode::Static:  return "Static";
    case ERHIResourceStateTrackingMode::Manual:  return "Manual";

    default: return "Unknown";
    }
}

enum class ERHIBarrierFlags : uint8
{
    None = 0,

    /** Split barrier begin. The transition starts here and must be completed by a matching EndOnly */
    BeginOnly = FLAG(0),

    /** Split barrier end. Completes a transition started by a matching BeginOnly */
    EndOnly = FLAG(1),

    /** Prior contents of the resource are undefined and may be discarded */
    Discard = FLAG(2),

    /** Once the transition lands, install NewTrackingMode with AfterState as the new baseline */
    ChangeTrackingMode = FLAG(3),
};

ENUM_CLASS_OPERATORS(ERHIBarrierFlags);

enum class ERHIBarrierResourceType : uint8
{
    Texture = 0,
    Buffer  = 1,
};

struct FRHITextureSubresourceRange
{
    NODISCARD static constexpr FRHITextureSubresourceRange All() noexcept
    {
        return FRHITextureSubresourceRange{ 0, RHI_ALL_MIP_LEVELS, 0, RHI_ALL_ARRAY_SLICES, 0, RHI_ALL_PLANE_SLICES };
    }

    NODISCARD static constexpr FRHITextureSubresourceRange MakeMip(uint32 MipLevel, uint32 ArraySlice = RHI_ALL_ARRAY_SLICES) noexcept
    {
        const bool bAllMips   = (MipLevel   == RHI_ALL_MIP_LEVELS);
        const bool bAllSlices = (ArraySlice == RHI_ALL_ARRAY_SLICES);

        return FRHITextureSubresourceRange
        {
            bAllMips   ? 0u : MipLevel,
            bAllMips   ? RHI_ALL_MIP_LEVELS   : 1u,
            bAllSlices ? 0u : ArraySlice,
            bAllSlices ? RHI_ALL_ARRAY_SLICES : 1u,
            0,
            RHI_ALL_PLANE_SLICES
        };
    }

    NODISCARD constexpr bool IsAllSubresources() const noexcept
    {
        return FirstMipLevel   == 0 && NumMipLevels   == RHI_ALL_MIP_LEVELS
            && FirstArraySlice == 0 && NumArraySlices == RHI_ALL_ARRAY_SLICES
            && FirstPlaneSlice == 0 && NumPlaneSlices == RHI_ALL_PLANE_SLICES;
    }

    uint32 FirstMipLevel;
    uint32 NumMipLevels;
    uint32 FirstArraySlice;
    uint32 NumArraySlices;
    uint32 FirstPlaneSlice;
    uint32 NumPlaneSlices;
};

struct FRHITransitionBarrierDesc
{
public:
    struct FTextureTransition
    {
        FRHITexture*                Resource;
        FRHITextureSubresourceRange Subresources;
    };

    struct FBufferTransition
    {
        FRHIBuffer*   Resource;
        FBufferRegion Range;
    };

public:
    FRHITransitionBarrierDesc() noexcept { }

    NODISCARD static FRHITransitionBarrierDesc CreateTexture(FRHITexture* InTexture, ERHIResourceState InAfterState) noexcept
    {
        return CreateTextureSubresource(InTexture, InAfterState, InAfterState, FRHITextureSubresourceRange::All(), ERHIBarrierFlags::None);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTexture(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, FRHITextureSubresourceRange::All(), ERHIBarrierFlags::None);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureMip(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState,
        uint32 InMipLevel, uint32 InArraySlice = RHI_ALL_ARRAY_SLICES) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, FRHITextureSubresourceRange::MakeMip(InMipLevel, InArraySlice), ERHIBarrierFlags::None);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureSubresource(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState,
        const FRHITextureSubresourceRange& InSubresources, ERHIBarrierFlags InFlags = ERHIBarrierFlags::None) noexcept
    {
        FRHITransitionBarrierDesc Desc;
        Desc.BeforeState          = InBeforeState;
        Desc.AfterState           = InAfterState;
        Desc.Flags                = InFlags;
        Desc.ResourceType         = ERHIBarrierResourceType::Texture;
        Desc.NewTrackingMode      = ERHIResourceStateTrackingMode::Tracked;
        Desc.Texture.Resource     = InTexture;
        Desc.Texture.Subresources = InSubresources;
        return Desc;
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureModeChange(FRHITexture* InTexture, ERHIResourceState InBeforeState,
        ERHIResourceState InAfterState, ERHIResourceStateTrackingMode InNewMode) noexcept
    {
        FRHITransitionBarrierDesc Desc = CreateTextureSubresource(InTexture, InBeforeState, InAfterState, 
            FRHITextureSubresourceRange::All(), ERHIBarrierFlags::ChangeTrackingMode);
        Desc.NewTrackingMode = InNewMode;
        return Desc;
    }

    NODISCARD static FRHITransitionBarrierDesc CreateBuffer(FRHIBuffer* InBuffer, ERHIResourceState InAfterState) noexcept
    {
        return CreateBufferRange(InBuffer, InAfterState, InAfterState, FBufferRegion::Whole(), ERHIBarrierFlags::None);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateBuffer(FRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateBufferRange(InBuffer, InBeforeState, InAfterState, FBufferRegion::Whole(), ERHIBarrierFlags::None);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateBufferRange(FRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState,
        const FBufferRegion& InRange, ERHIBarrierFlags InFlags = ERHIBarrierFlags::None) noexcept
    {
        FRHITransitionBarrierDesc Desc;
        Desc.BeforeState     = InBeforeState;
        Desc.AfterState      = InAfterState;
        Desc.Flags           = InFlags;
        Desc.ResourceType    = ERHIBarrierResourceType::Buffer;
        Desc.NewTrackingMode = ERHIResourceStateTrackingMode::Tracked;
        Desc.Buffer.Resource = InBuffer;
        Desc.Buffer.Range    = InRange;
        return Desc;
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureSplitBegin(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, FRHITextureSubresourceRange::All(), ERHIBarrierFlags::BeginOnly);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureSplitEnd(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, FRHITextureSubresourceRange::All(), ERHIBarrierFlags::EndOnly);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureSubresourceSplitBegin(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState,
        const FRHITextureSubresourceRange& InSubresources) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, InSubresources, ERHIBarrierFlags::BeginOnly);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateTextureSubresourceSplitEnd(FRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState,
        const FRHITextureSubresourceRange& InSubresources) noexcept
    {
        return CreateTextureSubresource(InTexture, InBeforeState, InAfterState, InSubresources, ERHIBarrierFlags::EndOnly);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateBufferSplitBegin(FRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateBufferRange(InBuffer, InBeforeState, InAfterState, FBufferRegion::Whole(), ERHIBarrierFlags::BeginOnly);
    }

    NODISCARD static FRHITransitionBarrierDesc CreateBufferSplitEnd(FRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState) noexcept
    {
        return CreateBufferRange(InBuffer, InBeforeState, InAfterState, FBufferRegion::Whole(), ERHIBarrierFlags::EndOnly);
    }

    NODISCARD constexpr bool IsTexture()            const noexcept { return ResourceType == ERHIBarrierResourceType::Texture; }
    NODISCARD constexpr bool IsBuffer()             const noexcept { return ResourceType == ERHIBarrierResourceType::Buffer; }
    NODISCARD constexpr bool IsSplitBegin()         const noexcept { return (Flags & ERHIBarrierFlags::BeginOnly) != ERHIBarrierFlags::None; }
    NODISCARD constexpr bool IsSplitEnd()           const noexcept { return (Flags & ERHIBarrierFlags::EndOnly) != ERHIBarrierFlags::None; }
    NODISCARD constexpr bool IsSplit()              const noexcept { return IsSplitBegin() || IsSplitEnd(); }
    NODISCARD constexpr bool IsDiscard()            const noexcept { return (Flags & ERHIBarrierFlags::Discard) != ERHIBarrierFlags::None; }
    NODISCARD constexpr bool IsTrackingModeChange() const noexcept { return (Flags & ERHIBarrierFlags::ChangeTrackingMode) != ERHIBarrierFlags::None; }

    ERHIResourceState             BeforeState;
    ERHIResourceState             AfterState;
    ERHIBarrierFlags              Flags;
    ERHIBarrierResourceType       ResourceType;
    ERHIResourceStateTrackingMode NewTrackingMode;

    union
    {
        FTextureTransition Texture;
        FBufferTransition  Buffer;
    };
};

struct FRHIUnorderedAccessBarrierDesc
{
public:
    struct FTextureBarrier
    {
        FRHITexture*                Resource;
        FRHITextureSubresourceRange Subresources;
    };

    struct FBufferBarrier
    {
        FRHIBuffer*   Resource;
        FBufferRegion Range;
    };

public:
    FRHIUnorderedAccessBarrierDesc() noexcept { }

    NODISCARD static FRHIUnorderedAccessBarrierDesc CreateTexture(FRHITexture* InTexture) noexcept
    {
        return CreateTextureSubresource(InTexture, FRHITextureSubresourceRange::All());
    }

    NODISCARD static FRHIUnorderedAccessBarrierDesc CreateTextureSubresource(FRHITexture* InTexture, const FRHITextureSubresourceRange& InSubresources) noexcept
    {
        FRHIUnorderedAccessBarrierDesc Desc;
        Desc.ResourceType         = ERHIBarrierResourceType::Texture;
        Desc.Texture.Resource     = InTexture;
        Desc.Texture.Subresources = InSubresources;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessBarrierDesc CreateBuffer(FRHIBuffer* InBuffer) noexcept
    {
        return CreateBufferRange(InBuffer, FBufferRegion::Whole());
    }

    NODISCARD static FRHIUnorderedAccessBarrierDesc CreateBufferRange(FRHIBuffer* InBuffer, const FBufferRegion& InRange) noexcept
    {
        FRHIUnorderedAccessBarrierDesc Desc;
        Desc.ResourceType    = ERHIBarrierResourceType::Buffer;
        Desc.Buffer.Resource = InBuffer;
        Desc.Buffer.Range    = InRange;
        return Desc;
    }

    NODISCARD constexpr bool IsTexture() const noexcept { return ResourceType == ERHIBarrierResourceType::Texture; }
    NODISCARD constexpr bool IsBuffer()  const noexcept { return ResourceType == ERHIBarrierResourceType::Buffer; }

    ERHIBarrierResourceType ResourceType;
    union
    {
        FTextureBarrier Texture;
        FBufferBarrier  Buffer;
    };
};

static_assert(TIsTriviallyCopyable<FRHITransitionBarrierDesc>::Value, "FRHITransitionBarrierDesc must be trivially copyable");
static_assert(TIsTriviallyCopyable<FRHIUnorderedAccessBarrierDesc>::Value, "FRHIUnorderedAccessBarrierDesc must be trivially copyable");
