#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Logging/Log.h"
#include "Core/Debug/Debug.h"

#include "RHI/RHITexture.h"
#include "RHI/RHIResourceViews.h"

#include <Metal/Metal.h>

#if MONOLITHIC_BUILD
    #define METAL_RHI_API
#else
    #if METALRHI_IMPL
        #define METAL_RHI_API MODULE_EXPORT
    #else
        #define METAL_RHI_API MODULE_IMPORT
    #endif
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Metal Log Macros

#if !PRODUCTION_BUILD
    #define METAL_ERROR(...)                      \
        do                                        \
        {                                         \
            LOG_ERROR("[MetalRHI] " __VA_ARGS__); \
            CDebug::DebugBreak();                 \
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI conversion functions 

CONSTEXPR MTLLoadAction ConvertAttachmentLoadAction(EAttachmentLoadAction LoadAction)
{
    switch(LoadAction)
    {
        case EAttachmentLoadAction::Clear:    return MTLLoadActionClear;
        case EAttachmentLoadAction::Load:     return MTLLoadActionLoad;
        case EAttachmentLoadAction::DontCare: return MTLLoadActionDontCare;
        default:                              return MTLLoadAction(-1);
    }
}

CONSTEXPR MTLStoreAction ConvertAttachmentStoreAction(EAttachmentStoreAction StoreAction)
{
    switch(StoreAction)
    {
        case EAttachmentStoreAction::Store:    return MTLStoreActionStore;
        case EAttachmentStoreAction::DontCare: return MTLStoreActionDontCare;
        default:                               return MTLStoreActionUnknown;
    }
}

inline MTLTextureType GetMTLTextureType(CRHITexture* Texture)
{
    Check(Texture != nullptr);
    
    if (CRHITexture2D* Texture2D = Texture->GetTexture2D())
    {
        return Texture2D->IsMultiSampled() ? MTLTextureType2DMultisample : MTLTextureType2D;
    }
    else if (CRHITexture2DArray* Texture2DArray = Texture->GetTexture2DArray())
    {
        return Texture2DArray->IsMultiSampled() ? MTLTextureType2DMultisampleArray : MTLTextureType2DArray;
    }
    else if (CRHITextureCube* TextureCube = Texture->GetTextureCube())
    {
        return MTLTextureTypeCube;
    }
    else if (CRHITextureCubeArray* TextureCubeArray = Texture->GetTextureCubeArray())
    {
        return MTLTextureTypeCubeArray;
    }
    else if (CRHITexture3D* Texture3D = Texture->GetTexture3D())
    {
        return MTLTextureType3D;
    }
    else
    {
        Check(false);
        return MTLTextureType(-1);
    }
}

CONSTEXPR MTLTextureUsage ConvertTextureFlags(ETextureUsageFlags Flag)
{
    MTLTextureUsage Result = MTLTextureUsageUnknown;
    if ((Flag & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None)
    {
        Result |= MTLTextureUsageShaderWrite;
    }
    if ((Flag & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None)
    {
        Result |= MTLTextureUsageRenderTarget;
    }
    if ((Flag & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None)
    {
        Result |= MTLTextureUsageRenderTarget;
    }
    if ((Flag & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None)
    {
        Result |= MTLTextureUsageShaderRead;
    }

    return Result;
}

CONSTEXPR MTLPixelFormat ConvertFormat(EFormat Format)
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
            
        default:                             return MTLPixelFormatInvalid;
    }
}

