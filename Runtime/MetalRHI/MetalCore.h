#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Debug.h"
#include "RHI/RHIResources.h"

#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>

#if !PRODUCTION_BUILD
    #define METAL_ERROR(...)                      \
        do                                        \
        {                                         \
            LOG_ERROR("[MetalRHI] " __VA_ARGS__); \
            DEBUG_BREAK();                 \
        } while (false)
    
    #define METAL_ERROR_COND(bCondition, ...) \
        do                                    \
        {                                     \
            if (!(bCondition))                \
            {                                 \
                METAL_ERROR(__VA_ARGS__);     \
            }                                 \
        } while (false)
    
    #define METAL_WARNING(...)                      \
        do                                          \
        {                                           \
            LOG_WARNING("[MetalRHI] " __VA_ARGS__); \
        } while (false)

    #define METAL_WARNING_COND(bCondition, ...) \
        do                                      \
        {                                       \
            if (!(bCondition))                  \
            {                                   \
                METAL_WARNING(__VA_ARGS__);     \
            }                                   \
        } while (false)

    #define METAL_INFO(...)                      \
        do                                       \
        {                                        \
            LOG_INFO("[MetalRHI] " __VA_ARGS__); \
        } while (false)
#else
    #define METAL_ERROR_COND(bCondition, ...) \
        do                                    \
        {                                     \
            (void)(bCondition);               \
        } while(false)

    #define METAL_ERROR(...)   do { (void)(0); } while(false)

    #define METAL_WARNING_COND(bCondition, ...) \
        do                                      \
        {                                       \
            (void)(bCondition);                 \
        } while(false)

    #define METAL_WARNING(...) do { (void)(0); } while(false)

    #define METAL_INFO(...)    do { (void)(0); } while(false)
#endif


enum : uint32
{
    kMaxSRVs            = 16,
    kMaxUAVs            = 16,
    kMaxConstantBuffers = 16,
    kMaxSamplerStates   = 16,
    
    kMaxTextures        = 32,
    kMaxBuffers         = 48,
    
    kBufferAlignment         = 16,
    kConstantBufferAlignment = 256,
};


constexpr MTLLoadAction ConvertAttachmentLoadAction(EAttachmentLoadAction LoadAction)
{
    switch(LoadAction)
    {
        case EAttachmentLoadAction::Clear:    return MTLLoadActionClear;
        case EAttachmentLoadAction::Load:     return MTLLoadActionLoad;
        case EAttachmentLoadAction::DontCare: return MTLLoadActionDontCare;
        default:                              return MTLLoadAction(-1);
    }
}

constexpr MTLStoreAction ConvertAttachmentStoreAction(EAttachmentStoreAction StoreAction)
{
    switch(StoreAction)
    {
        case EAttachmentStoreAction::Store:    return MTLStoreActionStore;
        case EAttachmentStoreAction::DontCare: return MTLStoreActionDontCare;
        default:                               return MTLStoreActionUnknown;
    }
}

constexpr MTLTextureType GetMTLTextureType(ETextureDimension TextureDimension, bool bIsMultisampled)
{
    switch(TextureDimension)
    {
        case ETextureDimension::Texture2D:        return bIsMultisampled ? MTLTextureType2DMultisample      : MTLTextureType2D;
        case ETextureDimension::Texture2DArray:   return bIsMultisampled ? MTLTextureType2DMultisampleArray : MTLTextureType2DArray;
        case ETextureDimension::TextureCube:      return MTLTextureTypeCube;
        case ETextureDimension::TextureCubeArray: return MTLTextureTypeCube;
        case ETextureDimension::Texture3D:        return MTLTextureType3D;

        default:
        {
            CHECK(false);
            return MTLTextureType(-1);
        }
    }
}

constexpr MTLTextureUsage ConvertTextureFlags(ETextureUsageFlags Flag)
{
    MTLTextureUsage Result = MTLTextureUsageUnknown;
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::UnorderedAccess))
    {
        Result |= MTLTextureUsageShaderWrite;
    }
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::UnorderedAccess))
    {
        Result |= MTLTextureUsageRenderTarget;
    }
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::DepthStencil))
    {
        Result |= MTLTextureUsageRenderTarget;
    }
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::ShaderResource))
    {
        Result |= MTLTextureUsageShaderRead;
    }

    return Result;
}

constexpr MTLPixelFormat ConvertFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Typeless: return MTLPixelFormatRGBA32Float;
        case EFormat::R32G32B32A32_Float:    return MTLPixelFormatRGBA32Float;
        case EFormat::R32G32B32A32_Uint:     return MTLPixelFormatRGBA32Uint;
        case EFormat::R32G32B32A32_Sint:     return MTLPixelFormatRGBA32Sint;
            
        case EFormat::R16G16B16A16_Typeless: return MTLPixelFormatRGBA16Unorm;
        case EFormat::R16G16B16A16_Float:    return MTLPixelFormatRGBA16Float;
        case EFormat::R16G16B16A16_Unorm:    return MTLPixelFormatRGBA16Unorm;
        case EFormat::R16G16B16A16_Uint:     return MTLPixelFormatRGBA16Uint;
        case EFormat::R16G16B16A16_Snorm:    return MTLPixelFormatRGBA16Snorm;
        case EFormat::R16G16B16A16_Sint:     return MTLPixelFormatRGBA16Sint;
    
        case EFormat::R32G32_Typeless:       return MTLPixelFormatRG32Float;
        case EFormat::R32G32_Float:          return MTLPixelFormatRG32Float;
        case EFormat::R32G32_Uint:           return MTLPixelFormatRG32Uint;
        case EFormat::R32G32_Sint:           return MTLPixelFormatRG32Sint;
    
        case EFormat::R10G10B10A2_Typeless:  return MTLPixelFormatRGB10A2Unorm;
        case EFormat::R10G10B10A2_Unorm:     return MTLPixelFormatRGB10A2Unorm;
        case EFormat::R10G10B10A2_Uint:      return MTLPixelFormatRGB10A2Uint;
    
        case EFormat::R11G11B10_Float:       return MTLPixelFormatRG11B10Float;
    
        case EFormat::R8G8B8A8_Typeless:     return MTLPixelFormatRGBA8Unorm;
        case EFormat::R8G8B8A8_Unorm:        return MTLPixelFormatRGBA8Unorm;
        case EFormat::R8G8B8A8_Unorm_SRGB:   return MTLPixelFormatRGBA8Unorm_sRGB;
        case EFormat::R8G8B8A8_Uint:         return MTLPixelFormatRGBA8Uint;
        case EFormat::R8G8B8A8_Snorm:        return MTLPixelFormatRGBA8Snorm;
        case EFormat::R8G8B8A8_Sint:         return MTLPixelFormatRGBA8Sint;
            
        case EFormat::B8G8R8A8_Typeless:     return MTLPixelFormatBGRA8Unorm;
        case EFormat::B8G8R8A8_Unorm:        return MTLPixelFormatBGRA8Unorm;
        case EFormat::B8G8R8A8_Unorm_SRGB:   return MTLPixelFormatBGRA8Unorm_sRGB;
    
        case EFormat::R16G16_Typeless:       return MTLPixelFormatRG16Unorm;
        case EFormat::R16G16_Float:          return MTLPixelFormatRG16Float;
        case EFormat::R16G16_Unorm:          return MTLPixelFormatRG16Unorm;
        case EFormat::R16G16_Uint:           return MTLPixelFormatRG16Uint;
        case EFormat::R16G16_Snorm:          return MTLPixelFormatRG16Snorm;
        case EFormat::R16G16_Sint:           return MTLPixelFormatRG16Sint;
    
        case EFormat::R32_Typeless:          return MTLPixelFormatR32Float;
        case EFormat::D32_Float:             return MTLPixelFormatDepth32Float;
        case EFormat::R32_Float:             return MTLPixelFormatR32Float;
        case EFormat::R32_Uint:              return MTLPixelFormatR32Uint;
        case EFormat::R32_Sint:              return MTLPixelFormatR32Sint;
    
        case EFormat::R24G8_Typeless:        return MTLPixelFormatDepth24Unorm_Stencil8;
    
        case EFormat::D24_Unorm_S8_Uint:     return MTLPixelFormatDepth24Unorm_Stencil8;
        case EFormat::R24_Unorm_X8_Typeless: return MTLPixelFormatDepth24Unorm_Stencil8;
        case EFormat::X24_Typeless_G8_Uint:  return MTLPixelFormatX24_Stencil8;
    
        case EFormat::R8G8_Typeless:         return MTLPixelFormatRG8Unorm;
        case EFormat::R8G8_Unorm:            return MTLPixelFormatRG8Unorm;
        case EFormat::R8G8_Uint:             return MTLPixelFormatRG8Uint;
        case EFormat::R8G8_Snorm:            return MTLPixelFormatRG8Snorm;
        case EFormat::R8G8_Sint:             return MTLPixelFormatRG8Sint;
    
        case EFormat::R16_Typeless:          return MTLPixelFormatR16Unorm;
        case EFormat::R16_Float:             return MTLPixelFormatR16Float;
        case EFormat::D16_Unorm:             return MTLPixelFormatDepth16Unorm;
        case EFormat::R16_Unorm:             return MTLPixelFormatR16Unorm;
        case EFormat::R16_Uint:              return MTLPixelFormatR16Uint;
        case EFormat::R16_Snorm:             return MTLPixelFormatR16Snorm;
        case EFormat::R16_Sint:              return MTLPixelFormatR16Sint;

        case EFormat::R8_Typeless:           return MTLPixelFormatR8Unorm;
        case EFormat::R8_Unorm:              return MTLPixelFormatR8Unorm;
        case EFormat::R8_Uint:               return MTLPixelFormatR8Uint;
        case EFormat::R8_Snorm:              return MTLPixelFormatR8Snorm;
        case EFormat::R8_Sint:               return MTLPixelFormatR8Sint;
            
        case EFormat::BC1_Typeless:          return MTLPixelFormatBC1_RGBA;
        case EFormat::BC1_UNorm:             return MTLPixelFormatBC1_RGBA;
        case EFormat::BC1_UNorm_SRGB:        return MTLPixelFormatBC1_RGBA_sRGB;
        case EFormat::BC2_Typeless:          return MTLPixelFormatBC2_RGBA;
        case EFormat::BC2_UNorm:             return MTLPixelFormatBC2_RGBA;
        case EFormat::BC2_UNorm_SRGB:        return MTLPixelFormatBC2_RGBA_sRGB;
        case EFormat::BC3_Typeless:          return MTLPixelFormatBC3_RGBA;
        case EFormat::BC3_UNorm:             return MTLPixelFormatBC3_RGBA;
        case EFormat::BC3_UNorm_SRGB:        return MTLPixelFormatBC3_RGBA_sRGB;
        case EFormat::BC4_Typeless:          return MTLPixelFormatBC4_RUnorm;
        case EFormat::BC4_UNorm:             return MTLPixelFormatBC4_RUnorm;
        case EFormat::BC4_SNorm:             return MTLPixelFormatBC4_RSnorm;
        case EFormat::BC5_Typeless:          return MTLPixelFormatBC5_RGUnorm;
        case EFormat::BC5_UNorm:             return MTLPixelFormatBC5_RGUnorm;
        case EFormat::BC5_SNorm:             return MTLPixelFormatBC5_RGSnorm;
        case EFormat::BC6H_Typeless:         return MTLPixelFormatBC6H_RGBFloat;
        case EFormat::BC6H_UF16:             return MTLPixelFormatBC6H_RGBUfloat;
        case EFormat::BC6H_SF16:             return MTLPixelFormatBC6H_RGBFloat;
        case EFormat::BC7_Typeless:          return MTLPixelFormatBC7_RGBAUnorm;
        case EFormat::BC7_UNorm:             return MTLPixelFormatBC7_RGBAUnorm;
        case EFormat::BC7_UNorm_SRGB:        return MTLPixelFormatBC7_RGBAUnorm_sRGB;
            
        default:                             return MTLPixelFormatInvalid;
    }
}

constexpr MTLVertexFormat ConvertVertexFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Float:    return MTLVertexFormatFloat4;
        case EFormat::R32G32B32A32_Uint:     return MTLVertexFormatUInt4;
        case EFormat::R32G32B32A32_Sint:     return MTLVertexFormatInt4;
    
        case EFormat::R32G32B32_Float:       return MTLVertexFormatFloat3;
        case EFormat::R32G32B32_Uint:        return MTLVertexFormatUInt3;
        case EFormat::R32G32B32_Sint:        return MTLVertexFormatInt3;
            
        case EFormat::R16G16B16A16_Float:    return MTLVertexFormatHalf4;
        case EFormat::R16G16B16A16_Unorm:    return MTLVertexFormatUShort4Normalized;
        case EFormat::R16G16B16A16_Uint:     return MTLVertexFormatUShort4;
        case EFormat::R16G16B16A16_Snorm:    return MTLVertexFormatShort4Normalized;
        case EFormat::R16G16B16A16_Sint:     return MTLVertexFormatShort4;
    
        case EFormat::R32G32_Float:          return MTLVertexFormatFloat2;
        case EFormat::R32G32_Uint:           return MTLVertexFormatUInt2;
        case EFormat::R32G32_Sint:           return MTLVertexFormatInt2;
    
        case EFormat::R10G10B10A2_Unorm:     return MTLVertexFormatUInt1010102Normalized;
    
        case EFormat::R8G8B8A8_Unorm:        return MTLVertexFormatUChar4Normalized;
        case EFormat::R8G8B8A8_Uint:         return MTLVertexFormatUChar4;
        case EFormat::R8G8B8A8_Snorm:        return MTLVertexFormatChar4Normalized;
        case EFormat::R8G8B8A8_Sint:         return MTLVertexFormatChar4;
    
        case EFormat::R16G16_Float:          return MTLVertexFormatHalf2;
        case EFormat::R16G16_Unorm:          return MTLVertexFormatUShort2Normalized;
        case EFormat::R16G16_Uint:           return MTLVertexFormatUShort2;
        case EFormat::R16G16_Snorm:          return MTLVertexFormatShort2Normalized;
        case EFormat::R16G16_Sint:           return MTLVertexFormatShort2;
    
        case EFormat::R32_Float:             return MTLVertexFormatFloat;
        case EFormat::R32_Uint:              return MTLVertexFormatUInt;
        case EFormat::R32_Sint:              return MTLVertexFormatInt;
    
        case EFormat::R8G8_Unorm:            return MTLVertexFormatUChar2Normalized;
        case EFormat::R8G8_Uint:             return MTLVertexFormatUChar2;
        case EFormat::R8G8_Snorm:            return MTLVertexFormatChar2Normalized;
        case EFormat::R8G8_Sint:             return MTLVertexFormatChar2;
    
        case EFormat::R16_Float:             return MTLVertexFormatHalf;
        case EFormat::R16_Unorm:             return MTLVertexFormatUShortNormalized;
        case EFormat::R16_Uint:              return MTLVertexFormatUShort;
        case EFormat::R16_Snorm:             return MTLVertexFormatShortNormalized;
        case EFormat::R16_Sint:              return MTLVertexFormatShort;

        case EFormat::R8_Unorm:              return MTLVertexFormatUCharNormalized;
        case EFormat::R8_Uint:               return MTLVertexFormatUChar;
        case EFormat::R8_Snorm:              return MTLVertexFormatCharNormalized;
        case EFormat::R8_Sint:               return MTLVertexFormatChar;
            
        default:                             return MTLVertexFormatInvalid;
    }
}

constexpr MTLVertexStepFunction ConvertVertexInputClass(EVertexInputClass InputClass)
{
    switch (InputClass)
    {
        case EVertexInputClass::Vertex:   return MTLVertexStepFunctionPerVertex;
        case EVertexInputClass::Instance: return MTLVertexStepFunctionPerInstance;
        default:                          return MTLVertexStepFunction(-1);
    }
}

constexpr MTLCompareFunction ConvertCompareFunction(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
        case EComparisonFunc::Never:        return MTLCompareFunctionNever;
        case EComparisonFunc::Less:         return MTLCompareFunctionLess;
        case EComparisonFunc::Equal:        return MTLCompareFunctionEqual;
        case EComparisonFunc::LessEqual:    return MTLCompareFunctionLessEqual;
        case EComparisonFunc::Greater:      return MTLCompareFunctionGreater;
        case EComparisonFunc::NotEqual:     return MTLCompareFunctionNotEqual;
        case EComparisonFunc::GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case EComparisonFunc::Always:       return MTLCompareFunctionAlways;
        default:                            return MTLCompareFunction(-1);
    }
}

constexpr MTLStencilOperation ConvertStencilOp(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
        case EStencilOp::Keep:    return MTLStencilOperationKeep;
        case EStencilOp::Zero:    return MTLStencilOperationZero;
        case EStencilOp::Replace: return MTLStencilOperationReplace;
        case EStencilOp::IncrSat: return MTLStencilOperationIncrementClamp;
        case EStencilOp::DecrSat: return MTLStencilOperationDecrementClamp;
        case EStencilOp::Invert:  return MTLStencilOperationInvert;
        case EStencilOp::Incr:    return MTLStencilOperationIncrementWrap;
        case EStencilOp::Decr:    return MTLStencilOperationDecrementWrap;
        default:                  return MTLStencilOperation(-1);
    }
}

constexpr MTLPrimitiveType ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
        case EPrimitiveTopology::PointList:     return MTLPrimitiveTypePoint;
        case EPrimitiveTopology::LineList:      return MTLPrimitiveTypeLine;
        case EPrimitiveTopology::LineStrip:     return MTLPrimitiveTypeLineStrip;
        case EPrimitiveTopology::TriangleList:  return MTLPrimitiveTypeTriangle;
        case EPrimitiveTopology::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        default:                                return MTLPrimitiveType(-1);
    }
}

constexpr MTLTriangleFillMode ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::WireFrame: return MTLTriangleFillModeLines;
        case EFillMode::Solid:     return MTLTriangleFillModeFill;
        default:                   return MTLTriangleFillMode(-1);
    }
}
