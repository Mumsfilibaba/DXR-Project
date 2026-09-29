#pragma once
#include "Core/Misc/Debug.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/ComPtr.h"
#include "RHI/RHIIndirect.h"
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11Constants.h"

#include <dxgi1_6.h>

static_assert(sizeof(FRHIDrawIndirectParameters)        == sizeof(D3D11_DRAW_INSTANCED_INDIRECT_ARGS));
static_assert(sizeof(FRHIDrawIndexedIndirectParameters) == sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS));

#if D3D11_ENABLE_LOGGING
    #define D3D11_ERROR_CRITICAL(...) \
        do \
        { \
            LOG_ERROR_CRITICAL("[D3D11RHI] " __VA_ARGS__); \
        } while (false)

    #define D3D11_ERROR(...) \
        do \
        { \
            LOG_ERROR("[D3D11RHI] " __VA_ARGS__); \
        } while (false)

    #define D3D11_ERROR_COND(bCondition, ...) \
        do \
        { \
            if (!(bCondition)) \
            { \
                D3D11_ERROR(__VA_ARGS__); \
            } \
        } while (false)

    #define D3D11_WARNING(...) \
        do \
        { \
            LOG_WARNING("[D3D11RHI] " __VA_ARGS__); \
        } while (false)

    #define D3D11_WARNING_COND(bCondition, ...) \
        do \
        { \
            if (!(bCondition)) \
            { \
                D3D11_WARNING(__VA_ARGS__); \
            } \
        } while (false)

    #define D3D11_INFO(...) \
        do \
        { \
            LOG_INFO("[D3D11RHI] " __VA_ARGS__); \
        } while (false)
#else
    #define D3D11_ERROR_CRITICAL(...) \
        do { } while(false)

    #define D3D11_ERROR_COND(bCondition, ...) \
        do { UNREFERENCED_VARIABLE(bCondition); } while(false)

    #define D3D11_ERROR(...) \
        do { } while(false)

    #define D3D11_WARNING_COND(bCondition, ...) \
        do { UNREFERENCED_VARIABLE(bCondition); } while(false)

    #define D3D11_WARNING(...) \
        do { } while(false)

    #define D3D11_INFO(...) \
        do { } while(false)
#endif

NODISCARD inline FRHIShaderResourceViewDesc GetDefaultShaderResourceViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint8   NumMips   = static_cast<uint8>(TextureDesc.NumMipLevels);
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture1DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2D(Format, 0, NumMips);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIShaderResourceViewDesc::CreateTexture2DArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCube(Format, 0, NumMips);
    }

    if (TextureDesc.IsTextureCubeArray())
    {
        return FRHIShaderResourceViewDesc::CreateTextureCubeArray(Format, 0, NumMips, 0, NumSlices);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIShaderResourceViewDesc::CreateTexture3D(Format, 0, NumMips);
    }

    return FRHIShaderResourceViewDesc{};
}

NODISCARD inline FRHIUnorderedAccessViewDesc GetDefaultUnorderedAccessViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIUnorderedAccessViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIUnorderedAccessViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIUnorderedAccessViewDesc{};
}

NODISCARD inline FRHIRenderTargetViewDesc GetDefaultRenderTargetViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIRenderTargetViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIRenderTargetViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    if (TextureDesc.IsTexture3D())
    {
        return FRHIRenderTargetViewDesc::CreateTexture3D(Format, 0, 0, static_cast<uint16>(TextureDesc.Extent.Z));
    }

    return FRHIRenderTargetViewDesc{};
}

NODISCARD inline FRHIDepthStencilViewDesc GetDefaultDepthStencilViewDescForTexture(const FRHITextureDesc& TextureDesc)
{
    const EFormat Format    = TextureDesc.ClearValue.Format != EFormat::Unknown ? TextureDesc.ClearValue.Format : TextureDesc.Format;
    const uint16  NumSlices = static_cast<uint16>(TextureDesc.NumArraySlices);

    if (TextureDesc.IsTexture1D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1D(Format, 0);
    }

    if (TextureDesc.IsTexture1DArray())
    {
        return FRHIDepthStencilViewDesc::CreateTexture1DArray(Format, 0, 0, NumSlices);
    }

    if (TextureDesc.IsTexture2D())
    {
        return FRHIDepthStencilViewDesc::CreateTexture2D(Format, 0);
    }

    if (TextureDesc.IsTexture2DArray() || TextureDesc.IsTextureCube() || TextureDesc.IsTextureCubeArray())
    {
        const uint16 ArraySize = static_cast<uint16>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices));
        return FRHIDepthStencilViewDesc::CreateTexture2DArray(Format, 0, 0, ArraySize);
    }

    return FRHIDepthStencilViewDesc{};
}

NODISCARD constexpr D3D11_USAGE ConvertBufferUsage(EBufferFlags Flags)
{
    if (IsEnumFlagSet(Flags, EBufferFlags::ReadBack))
    {
        return D3D11_USAGE_STAGING;
    }
    else if (IsEnumFlagSet(Flags, EBufferFlags::Dynamic))
    {
        return D3D11_USAGE_DYNAMIC;
    }

    return D3D11_USAGE_DEFAULT;
}

NODISCARD constexpr UINT ConvertBufferBindFlags(EBufferFlags Flags)
{
    if (IsEnumFlagSet(Flags, EBufferFlags::ReadBack))
    {
        return 0;
    }

    UINT Result = 0;
    if (IsEnumFlagSet(Flags, EBufferFlags::VertexBuffer))
    {
        Result |= D3D11_BIND_VERTEX_BUFFER;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::IndexBuffer))
    {
        Result |= D3D11_BIND_INDEX_BUFFER;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::ConstantBuffer))
    {
        Result |= D3D11_BIND_CONSTANT_BUFFER;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::ShaderResourceBuffer))
    {
        Result |= D3D11_BIND_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::UnorderedAccessBuffer))
    {
        Result |= D3D11_BIND_UNORDERED_ACCESS;
    }

    return Result;
}

NODISCARD constexpr UINT ConvertTextureBindFlags(ETextureUsageFlags Flags)
{
    UINT Result = 0;
    if (IsEnumFlagSet(Flags, ETextureUsageFlags::ShaderResourceTexture))
    {
        Result |= D3D11_BIND_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(Flags, ETextureUsageFlags::UnorderedAccessTexture))
    {
        Result |= D3D11_BIND_UNORDERED_ACCESS;
    }

    if (IsEnumFlagSet(Flags, ETextureUsageFlags::RenderTarget))
    {
        Result |= D3D11_BIND_RENDER_TARGET;
    }

    if (IsEnumFlagSet(Flags, ETextureUsageFlags::DepthStencil))
    {
        Result |= D3D11_BIND_DEPTH_STENCIL;
    }

    return Result;
}

NODISCARD constexpr DXGI_FORMAT ConvertFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Typeless: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case EFormat::R32G32B32A32_Float:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case EFormat::R32G32B32A32_Uint:     return DXGI_FORMAT_R32G32B32A32_UINT;
        case EFormat::R32G32B32A32_Sint:     return DXGI_FORMAT_R32G32B32A32_SINT;
        case EFormat::R32G32B32_Typeless:    return DXGI_FORMAT_R32G32B32_TYPELESS;
        case EFormat::R32G32B32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;
        case EFormat::R32G32B32_Uint:        return DXGI_FORMAT_R32G32B32_UINT;
        case EFormat::R32G32B32_Sint:        return DXGI_FORMAT_R32G32B32_SINT;
        case EFormat::R16G16B16A16_Typeless: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case EFormat::R16G16B16A16_Float:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case EFormat::R16G16B16A16_Unorm:    return DXGI_FORMAT_R16G16B16A16_UNORM;
        case EFormat::R16G16B16A16_Uint:     return DXGI_FORMAT_R16G16B16A16_UINT;
        case EFormat::R16G16B16A16_Snorm:    return DXGI_FORMAT_R16G16B16A16_SNORM;
        case EFormat::R16G16B16A16_Sint:     return DXGI_FORMAT_R16G16B16A16_SINT;
        case EFormat::R32G32_Typeless:       return DXGI_FORMAT_R32G32_TYPELESS;
        case EFormat::R32G32_Float:          return DXGI_FORMAT_R32G32_FLOAT;
        case EFormat::R32G32_Uint:           return DXGI_FORMAT_R32G32_UINT;
        case EFormat::R32G32_Sint:           return DXGI_FORMAT_R32G32_SINT;
        case EFormat::R10G10B10A2_Typeless:  return DXGI_FORMAT_R10G10B10A2_TYPELESS;
        case EFormat::R10G10B10A2_Unorm:     return DXGI_FORMAT_R10G10B10A2_UNORM;
        case EFormat::R10G10B10A2_Uint:      return DXGI_FORMAT_R10G10B10A2_UINT;
        case EFormat::R11G11B10_Float:       return DXGI_FORMAT_R11G11B10_FLOAT;
        case EFormat::R8G8B8A8_Typeless:     return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case EFormat::R8G8B8A8_Unorm:        return DXGI_FORMAT_R8G8B8A8_UNORM;
        case EFormat::R8G8B8A8_Unorm_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case EFormat::R8G8B8A8_Uint:         return DXGI_FORMAT_R8G8B8A8_UINT;
        case EFormat::R8G8B8A8_Snorm:        return DXGI_FORMAT_R8G8B8A8_SNORM;
        case EFormat::R8G8B8A8_Sint:         return DXGI_FORMAT_R8G8B8A8_SINT;
        case EFormat::B8G8R8A8_Typeless:     return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case EFormat::B8G8R8A8_Unorm:        return DXGI_FORMAT_B8G8R8A8_UNORM;
        case EFormat::B8G8R8A8_Unorm_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case EFormat::R16G16_Typeless:       return DXGI_FORMAT_R16G16_TYPELESS;
        case EFormat::R16G16_Float:          return DXGI_FORMAT_R16G16_FLOAT;
        case EFormat::R16G16_Unorm:          return DXGI_FORMAT_R16G16_UNORM;
        case EFormat::R16G16_Uint:           return DXGI_FORMAT_R16G16_UINT;
        case EFormat::R16G16_Snorm:          return DXGI_FORMAT_R16G16_SNORM;
        case EFormat::R16G16_Sint:           return DXGI_FORMAT_R16G16_SINT;
        case EFormat::R32_Typeless:          return DXGI_FORMAT_R32_TYPELESS;
        case EFormat::D32_Float:             return DXGI_FORMAT_D32_FLOAT;
        case EFormat::R32_Float:             return DXGI_FORMAT_R32_FLOAT;
        case EFormat::R32_Uint:              return DXGI_FORMAT_R32_UINT;
        case EFormat::R32_Sint:              return DXGI_FORMAT_R32_SINT;
        case EFormat::R24G8_Typeless:        return DXGI_FORMAT_R24G8_TYPELESS;
        case EFormat::D24_Unorm_S8_Uint:     return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case EFormat::R24_Unorm_X8_Typeless: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case EFormat::X24_Typeless_G8_Uint:  return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        case EFormat::R8G8_Typeless:         return DXGI_FORMAT_R8G8_TYPELESS;
        case EFormat::R8G8_Unorm:            return DXGI_FORMAT_R8G8_UNORM;
        case EFormat::R8G8_Uint:             return DXGI_FORMAT_R8G8_UINT;
        case EFormat::R8G8_Snorm:            return DXGI_FORMAT_R8G8_SNORM;
        case EFormat::R8G8_Sint:             return DXGI_FORMAT_R8G8_SINT;
        case EFormat::R16_Typeless:          return DXGI_FORMAT_R16_TYPELESS;
        case EFormat::R16_Float:             return DXGI_FORMAT_R16_FLOAT;
        case EFormat::D16_Unorm:             return DXGI_FORMAT_D16_UNORM;
        case EFormat::R16_Unorm:             return DXGI_FORMAT_R16_UNORM;
        case EFormat::R16_Uint:              return DXGI_FORMAT_R16_UINT;
        case EFormat::R16_Snorm:             return DXGI_FORMAT_R16_SNORM;
        case EFormat::R16_Sint:              return DXGI_FORMAT_R16_SINT;
        case EFormat::R8_Typeless:           return DXGI_FORMAT_R8_TYPELESS;
        case EFormat::R8_Unorm:              return DXGI_FORMAT_R8_UNORM;
        case EFormat::R8_Uint:               return DXGI_FORMAT_R8_UINT;
        case EFormat::R8_Snorm:              return DXGI_FORMAT_R8_SNORM;
        case EFormat::R8_Sint:               return DXGI_FORMAT_R8_SINT;
        case EFormat::BC1_Typeless:          return DXGI_FORMAT_BC1_TYPELESS;
        case EFormat::BC1_UNorm:             return DXGI_FORMAT_BC1_UNORM;
        case EFormat::BC1_UNorm_SRGB:        return DXGI_FORMAT_BC1_UNORM_SRGB;
        case EFormat::BC2_Typeless:          return DXGI_FORMAT_BC2_TYPELESS;
        case EFormat::BC2_UNorm:             return DXGI_FORMAT_BC2_UNORM;
        case EFormat::BC2_UNorm_SRGB:        return DXGI_FORMAT_BC2_UNORM_SRGB;
        case EFormat::BC3_Typeless:          return DXGI_FORMAT_BC3_TYPELESS;
        case EFormat::BC3_UNorm:             return DXGI_FORMAT_BC3_UNORM;
        case EFormat::BC3_UNorm_SRGB:        return DXGI_FORMAT_BC3_UNORM_SRGB;
        case EFormat::BC4_Typeless:          return DXGI_FORMAT_BC4_TYPELESS;
        case EFormat::BC4_UNorm:             return DXGI_FORMAT_BC4_UNORM;
        case EFormat::BC4_SNorm:             return DXGI_FORMAT_BC4_SNORM;
        case EFormat::BC5_Typeless:          return DXGI_FORMAT_BC5_TYPELESS;
        case EFormat::BC5_UNorm:             return DXGI_FORMAT_BC5_UNORM;
        case EFormat::BC5_SNorm:             return DXGI_FORMAT_BC5_SNORM;
        case EFormat::BC6H_Typeless:         return DXGI_FORMAT_BC6H_TYPELESS;
        case EFormat::BC6H_UF16:             return DXGI_FORMAT_BC6H_UF16;
        case EFormat::BC6H_SF16:             return DXGI_FORMAT_BC6H_SF16;
        case EFormat::BC7_Typeless:          return DXGI_FORMAT_BC7_TYPELESS;
        case EFormat::BC7_UNorm:             return DXGI_FORMAT_BC7_UNORM;
        case EFormat::BC7_UNorm_SRGB:        return DXGI_FORMAT_BC7_UNORM_SRGB;

        default: return DXGI_FORMAT_UNKNOWN;
    }
}

NODISCARD constexpr bool IsStencilFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;

        default:
            return false;
    }
}

NODISCARD constexpr DXGI_USAGE D3D11ConvertSwapChainUsage(ESwapChainUsageFlags Usage)
{
    DXGI_USAGE Result = 0;
    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::RenderTarget))
    {
        Result |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
    }

    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::UnorderedAccess))
    {
        Result |= DXGI_USAGE_UNORDERED_ACCESS;
    }

    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::ShaderResource))
    {
        Result |= DXGI_USAGE_SHADER_INPUT;
    }

    return Result;
}

NODISCARD constexpr DXGI_COLOR_SPACE_TYPE ConvertColorSpace(EColorSpace ColorSpace)
{
    switch (ColorSpace)
    {
        case EColorSpace::RGB_Full_G22_None_P709:    return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        case EColorSpace::RGB_Full_G10_None_P709:    return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
        case EColorSpace::RGB_Full_G2084_None_P2020: return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
        case EColorSpace::RGB_Full_G22_None_P2020:   return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
        default:                                     return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    }
}

NODISCARD constexpr EColorSpace ConvertColorSpace(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
    switch (ColorSpace)
    {
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:    return EColorSpace::RGB_Full_G22_None_P709;
        case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:    return EColorSpace::RGB_Full_G10_None_P709;
        case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020: return EColorSpace::RGB_Full_G2084_None_P2020;
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:   return EColorSpace::RGB_Full_G22_None_P2020;
        default:                                         return EColorSpace::Unknown;
    }
}

NODISCARD constexpr DXGI_FORMAT ConvertIndexFormat(EIndexFormat IndexFormat)
{
    if (IndexFormat == EIndexFormat::uint32)
    {
        return DXGI_FORMAT_R32_UINT;
    }
    else if (IndexFormat == EIndexFormat::uint16)
    {
        return DXGI_FORMAT_R16_UINT;
    }
    else
    {
        return DXGI_FORMAT_UNKNOWN;
    }
}

NODISCARD constexpr D3D11_INPUT_CLASSIFICATION ConvertVertexInputClass(EVertexInputClass InputClassification)
{
    switch (InputClassification)
    {
        case EVertexInputClass::Instance: return D3D11_INPUT_PER_INSTANCE_DATA;
        case EVertexInputClass::Vertex:   return D3D11_INPUT_PER_VERTEX_DATA;
    }

    return D3D11_INPUT_CLASSIFICATION(-1);
}

NODISCARD constexpr D3D11_COMPARISON_FUNC ConvertComparisonFunc(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
        case EComparisonFunc::Never:        return D3D11_COMPARISON_NEVER;
        case EComparisonFunc::Less:         return D3D11_COMPARISON_LESS;
        case EComparisonFunc::Equal:        return D3D11_COMPARISON_EQUAL;
        case EComparisonFunc::LessEqual:    return D3D11_COMPARISON_LESS_EQUAL;
        case EComparisonFunc::Greater:      return D3D11_COMPARISON_GREATER;
        case EComparisonFunc::NotEqual:     return D3D11_COMPARISON_NOT_EQUAL;
        case EComparisonFunc::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
        case EComparisonFunc::Always:       return D3D11_COMPARISON_ALWAYS;
    }

    return D3D11_COMPARISON_NEVER;
}

NODISCARD constexpr D3D11_STENCIL_OP ConvertStencilOp(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
        case EStencilOp::Keep:    return D3D11_STENCIL_OP_KEEP;
        case EStencilOp::Zero:    return D3D11_STENCIL_OP_ZERO;
        case EStencilOp::Replace: return D3D11_STENCIL_OP_REPLACE;
        case EStencilOp::IncrSat: return D3D11_STENCIL_OP_INCR_SAT;
        case EStencilOp::DecrSat: return D3D11_STENCIL_OP_DECR_SAT;
        case EStencilOp::Invert:  return D3D11_STENCIL_OP_INVERT;
        case EStencilOp::Incr:    return D3D11_STENCIL_OP_INCR;
        case EStencilOp::Decr:    return D3D11_STENCIL_OP_DECR;
    }

    return D3D11_STENCIL_OP(-1);
}

NODISCARD inline D3D11_DEPTH_STENCILOP_DESC ConvertStencilState(const FRHIDepthStencilStateDesc::FStencilState& StencilState)
{
    return
    {
        ConvertStencilOp(StencilState.StencilFailOp),
        ConvertStencilOp(StencilState.StencilDepthFailOp),
        ConvertStencilOp(StencilState.StencilDepthPassOp),
        ConvertComparisonFunc(StencilState.StencilFunc)
    };
}

NODISCARD constexpr D3D11_CULL_MODE ConvertCullMode(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::Back:  return D3D11_CULL_BACK;
        case ECullMode::Front: return D3D11_CULL_FRONT;
        default:               return D3D11_CULL_NONE;
    }
}

NODISCARD constexpr D3D11_FILL_MODE ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::Solid:     return D3D11_FILL_SOLID;
        case EFillMode::WireFrame: return D3D11_FILL_WIREFRAME;
    }

    return D3D11_FILL_MODE();
}

NODISCARD constexpr D3D11_BLEND_OP ConvertBlendOp(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
        case EBlendOp::Add:         return D3D11_BLEND_OP_ADD;
        case EBlendOp::Max:         return D3D11_BLEND_OP_MAX;
        case EBlendOp::Min:         return D3D11_BLEND_OP_MIN;
        case EBlendOp::RevSubtract: return D3D11_BLEND_OP_REV_SUBTRACT;
        case EBlendOp::Subtract:    return D3D11_BLEND_OP_SUBTRACT;
    }

    return D3D11_BLEND_OP();
}

NODISCARD constexpr D3D11_BLEND ConvertBlend(EBlendType Blend)
{
    switch (Blend)
    {
        case EBlendType::Zero:           return D3D11_BLEND_ZERO;
        case EBlendType::One:            return D3D11_BLEND_ONE;
        case EBlendType::SrcColor:       return D3D11_BLEND_SRC_COLOR;
        case EBlendType::InvSrcColor:    return D3D11_BLEND_INV_SRC_COLOR;
        case EBlendType::SrcAlpha:       return D3D11_BLEND_SRC_ALPHA;
        case EBlendType::InvSrcAlpha:    return D3D11_BLEND_INV_SRC_ALPHA;
        case EBlendType::DstAlpha:       return D3D11_BLEND_DEST_ALPHA;
        case EBlendType::InvDstAlpha:    return D3D11_BLEND_INV_DEST_ALPHA;
        case EBlendType::DstColor:       return D3D11_BLEND_DEST_COLOR;
        case EBlendType::InvDstColor:    return D3D11_BLEND_INV_DEST_COLOR;
        case EBlendType::SrcAlphaSat:    return D3D11_BLEND_SRC_ALPHA_SAT;
        case EBlendType::Src1Color:      return D3D11_BLEND_SRC1_COLOR;
        case EBlendType::InvSrc1Color:   return D3D11_BLEND_INV_SRC1_COLOR;
        case EBlendType::Src1Alpha:      return D3D11_BLEND_SRC1_ALPHA;
        case EBlendType::InvSrc1Alpha:   return D3D11_BLEND_INV_SRC1_ALPHA;
        case EBlendType::BlendFactor:    return D3D11_BLEND_BLEND_FACTOR;
        case EBlendType::InvBlendFactor: return D3D11_BLEND_INV_BLEND_FACTOR;
    }

    return D3D11_BLEND();
}

NODISCARD constexpr D3D11_LOGIC_OP ConvertLogicOp(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
        case ELogicOp::Clear:        return D3D11_LOGIC_OP_CLEAR;
        case ELogicOp::Set:          return D3D11_LOGIC_OP_SET;
        case ELogicOp::Copy:         return D3D11_LOGIC_OP_COPY;
        case ELogicOp::CopyInverted: return D3D11_LOGIC_OP_COPY_INVERTED;
        case ELogicOp::NoOp:         return D3D11_LOGIC_OP_NOOP;
        case ELogicOp::Invert:       return D3D11_LOGIC_OP_INVERT;
        case ELogicOp::And:          return D3D11_LOGIC_OP_AND;
        case ELogicOp::Nand:         return D3D11_LOGIC_OP_NAND;
        case ELogicOp::Or:           return D3D11_LOGIC_OP_OR;
        case ELogicOp::Nor:          return D3D11_LOGIC_OP_NOR;
        case ELogicOp::Xor:          return D3D11_LOGIC_OP_XOR;
        case ELogicOp::Equivalent:   return D3D11_LOGIC_OP_EQUIV;
        case ELogicOp::AndReverse:   return D3D11_LOGIC_OP_AND_REVERSE;
        case ELogicOp::AndInverted:  return D3D11_LOGIC_OP_AND_INVERTED;
        case ELogicOp::OrReverse:    return D3D11_LOGIC_OP_OR_REVERSE;
        case ELogicOp::OrInverted:   return D3D11_LOGIC_OP_OR_INVERTED;
    }

    return D3D11_LOGIC_OP();
}

NODISCARD constexpr uint8 ConvertColorWriteFlags(EColorWriteFlags ColorWriteFlags)
{
    uint8 RenderTargetWriteMask = 0;
    if (ColorWriteFlags == EColorWriteFlags::All)
    {
        RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    else
    {
        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Red))
        {
            RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
        }

        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Green))
        {
            RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
        }

        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Blue))
        {
            RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
        }

        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Alpha))
        {
            RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    return RenderTargetWriteMask;
}

NODISCARD constexpr D3D11_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
{
    static_assert(D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST - D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST == (RHI_MAX_PATCH_CONTROL_POINTS - 1),
        "D3D_PRIMITIVE_TOPOLOGY_N_CONTROL_POINT_PATCHLIST enumerators must be contiguous");

    if (IsPatchTopology(PrimitiveTopology))
    {
        return static_cast<D3D11_PRIMITIVE_TOPOLOGY>(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (GetNumPatchControlPoints(PrimitiveTopology) - 1));
    }

    switch (PrimitiveTopology)
    {
        case EPrimitiveTopology::LineList:               return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case EPrimitiveTopology::LineStrip:              return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case EPrimitiveTopology::PointList:              return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case EPrimitiveTopology::TriangleList:           return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case EPrimitiveTopology::TriangleStrip:          return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case EPrimitiveTopology::LineListAdjacency:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
        case EPrimitiveTopology::LineStripAdjacency:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
        case EPrimitiveTopology::TriangleListAdjacency:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
        case EPrimitiveTopology::TriangleStripAdjacency: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
        case EPrimitiveTopology::Undefined:              return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        default:                                         return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

NODISCARD constexpr D3D11_TEXTURE_ADDRESS_MODE ConvertSamplerMode(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
        case ESamplerMode::Wrap:       return D3D11_TEXTURE_ADDRESS_WRAP;
        case ESamplerMode::Clamp:      return D3D11_TEXTURE_ADDRESS_CLAMP;
        case ESamplerMode::Mirror:     return D3D11_TEXTURE_ADDRESS_MIRROR;
        case ESamplerMode::Border:     return D3D11_TEXTURE_ADDRESS_BORDER;
        case ESamplerMode::MirrorOnce: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    }

    return D3D11_TEXTURE_ADDRESS_MODE();
}

NODISCARD constexpr D3D11_FILTER ConvertSamplerFilter(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
        case ESamplerFilter::MinMagMipPoint:                          return D3D11_FILTER_MIN_MAG_MIP_POINT;
        case ESamplerFilter::MinMagPoint_MipLinear:                   return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case ESamplerFilter::MinPoint_MagMipLinear:                   return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case ESamplerFilter::MinLinear_MagMipPoint:                   return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case ESamplerFilter::MinMagLinear_MipPoint:                   return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case ESamplerFilter::MinMagMipLinear:                         return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        case ESamplerFilter::Anistrotopic:                            return D3D11_FILTER_ANISOTROPIC;
        case ESamplerFilter::Comparison_MinMagMipPoint:               return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case ESamplerFilter::Comparison_MinMagMipLinear:              return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case ESamplerFilter::Comparison_Anisotropic:                  return D3D11_FILTER_COMPARISON_ANISOTROPIC;
    }

    return D3D11_FILTER();
}

NODISCARD constexpr uint32 GetFormatStride(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            return 16;
        }

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        {
            return 12;
        }

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        {
            return 8;
        }

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        {
            return 4;
        }

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        {
            return 2;
        }

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        {
            return 1;
        }

        default:
        {
            return 0;
        }
    }
}

NODISCARD constexpr bool IsFormatCompressed(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;

        default:
            return false;
    }
}

NODISCARD constexpr uint32 GetBitsPerPixel(DXGI_FORMAT Format)
{
    if (IsFormatCompressed(Format))
    {
        switch (Format)
        {
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return 4;

            default:
                return 8;
        }
    }

    const uint32 BytesPerTexel = GetFormatStride(Format);
    return BytesPerTexel ? (BytesPerTexel * 8u) : 0u;
}

NODISCARD constexpr DXGI_FORMAT D3D11CastShaderResourceFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;

        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:    return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:    return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R16G16_TYPELESS:      return DXGI_FORMAT_R16G16_FLOAT;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

        case DXGI_FORMAT_R8G8_TYPELESS: return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:  return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_D16_UNORM:     return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R8_TYPELESS:   return DXGI_FORMAT_R8_UNORM;
        case DXGI_FORMAT_BC1_TYPELESS:  return DXGI_FORMAT_BC1_UNORM;
        case DXGI_FORMAT_BC2_TYPELESS:  return DXGI_FORMAT_BC2_UNORM;
        case DXGI_FORMAT_BC3_TYPELESS:  return DXGI_FORMAT_BC3_UNORM;
        case DXGI_FORMAT_BC4_TYPELESS:  return DXGI_FORMAT_BC4_UNORM;
        case DXGI_FORMAT_BC5_TYPELESS:  return DXGI_FORMAT_BC5_UNORM;
        case DXGI_FORMAT_BC6H_TYPELESS: return DXGI_FORMAT_BC6H_UF16;
        case DXGI_FORMAT_BC7_TYPELESS:  return DXGI_FORMAT_BC7_UNORM;

        default:
            return Format;
    }
}

NODISCARD constexpr DXGI_FORMAT D3D11CastUnorderedAccessFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
        default:                                return Format;
    }
}

NODISCARD constexpr DXGI_FORMAT D3D11CastRenderTargetFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
        default:                                return Format;
    }
}

NODISCARD constexpr DXGI_FORMAT D3D11CastDepthStencilFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case DXGI_FORMAT_R32_TYPELESS:      return DXGI_FORMAT_D32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_R16_TYPELESS:      return DXGI_FORMAT_D16_UNORM;
        default:                            return Format;
    }
}
