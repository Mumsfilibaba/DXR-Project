#pragma once
#include "D3D12Constants.h"

#include "RHI/RHIResources.h"

#include "Core/Debug/Debug.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/ComPtr.h"

#include <d3d12.h>
#include <dxcapi.h>

#define D3D12_DESCRIPTOR_HANDLE_INCREMENT(DescriptorHandle, Value) { (DescriptorHandle.ptr + Value) }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

#if !PRODUCTION_BUILD
#define D3D12_ERROR(Condition, ErrorMessage) \
    do                                       \
    {                                        \
        if (!(Condition))                    \
        {                                    \
            LOG_ERROR(ErrorMessage);         \
            CDebug::DebugBreak();            \
        }                                    \
    } while (0)

#define D3D12_ERROR_ALWAYS(ErrorMessage) \
    do                                   \
    {                                    \
            LOG_ERROR(ErrorMessage);     \
            CDebug::DebugBreak();        \
    } while (0)

#else
#define D3D12_ERROR(Condtion, ErrorString) do {} while(0)
#define D3D12_ERROR_ALWAYS(ErrorString)    do {} while(0)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// Returns upload heap properties
inline D3D12_HEAP_PROPERTIES GetUploadHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    CMemory::Memzero(&HeapProperties);

    HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask = 1;
    HeapProperties.CreationNodeMask = 1;

    return HeapProperties;
}

// Returns default heap properties
inline D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    CMemory::Memzero(&HeapProperties);

    HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask = 1;
    HeapProperties.CreationNodeMask = 1;

    return HeapProperties;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// Converts EBufferFlag- flags to D3D12_RESOURCE_FLAGS
inline D3D12_RESOURCE_FLAGS ConvertBufferFlags(uint32 Flag)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (Flag & EBufferFlags::BufferFlag_UAV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return Result;
}

// Converts ETextureFlag- flags to D3D12_RESOURCE_FLAGS
inline D3D12_RESOURCE_FLAGS ConvertTextureFlags(uint32 Flag)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (Flag & ETextureFlags::TextureFlag_UAV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (Flag & ETextureFlags::TextureFlag_RTV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (Flag & ETextureFlags::TextureFlag_DSV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (!(Flag & ETextureFlags::TextureFlag_SRV))
        {
            Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

    return Result;
}

// Converts EFormat to DXGI_FORMAT
inline DXGI_FORMAT ConvertFormat(EFormat Format)
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
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

// Converts EInputClassification to D3D12_INPUT_CLASSIFICATION
inline D3D12_INPUT_CLASSIFICATION ConvertInputClassification(EInputClassification InputClassification)
{
    switch (InputClassification)
    {
    case EInputClassification::Instance: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    case EInputClassification::Vertex:   return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }

    return D3D12_INPUT_CLASSIFICATION();
}

// Converts EDepthWriteMask to DXGI_FORMAT
inline D3D12_DEPTH_WRITE_MASK ConvertDepthWriteMask(EDepthWriteMask DepthWriteMask)
{
    switch (DepthWriteMask)
    {
    case EDepthWriteMask::Zero: return D3D12_DEPTH_WRITE_MASK_ZERO;
    case EDepthWriteMask::All:  return D3D12_DEPTH_WRITE_MASK_ALL;
    }

    return D3D12_DEPTH_WRITE_MASK();
}

// Converts EComparisonFunc to D3D12_COMPARISON_FUNC
inline D3D12_COMPARISON_FUNC ConvertComparisonFunc(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
    case EComparisonFunc::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case EComparisonFunc::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case EComparisonFunc::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case EComparisonFunc::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case EComparisonFunc::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case EComparisonFunc::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case EComparisonFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case EComparisonFunc::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    return D3D12_COMPARISON_FUNC();
}

// Converts EStencilOp to D3D12_STENCIL_OP
inline D3D12_STENCIL_OP ConvertStencilOp(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
    case EStencilOp::Keep:    return D3D12_STENCIL_OP_KEEP;
    case EStencilOp::Zero:    return D3D12_STENCIL_OP_ZERO;
    case EStencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
    case EStencilOp::IncrSat: return D3D12_STENCIL_OP_INCR_SAT;
    case EStencilOp::DecrSat: return D3D12_STENCIL_OP_DECR_SAT;
    case EStencilOp::Invert:  return D3D12_STENCIL_OP_INVERT;
    case EStencilOp::Incr:    return D3D12_STENCIL_OP_INCR;
    case EStencilOp::Decr:    return D3D12_STENCIL_OP_DECR;
    }

    return D3D12_STENCIL_OP();
}

// Converts DepthStencilOp to D3D12_DEPTH_STENCILOP_DESC
inline D3D12_DEPTH_STENCILOP_DESC ConvertDepthStencilOp(const SDepthStencilOp& DepthStencilOp)
{
    return
    {
        ConvertStencilOp(DepthStencilOp.StencilFailOp),
        ConvertStencilOp(DepthStencilOp.StencilDepthFailOp),
        ConvertStencilOp(DepthStencilOp.StencilPassOp),
        ConvertComparisonFunc(DepthStencilOp.StencilFunc)
    };
}

// Converts ECullMode to D3D12_CULL_MODE
inline D3D12_CULL_MODE ConvertCullMode(ECullMode CullMode)
{
    switch (CullMode)
    {
    case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
    case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
    default: return D3D12_CULL_MODE_NONE;
    }
}

// Converts EFillMode to D3D12_FILL_MODE
inline D3D12_FILL_MODE ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
    case EFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    case EFillMode::WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    }

    return D3D12_FILL_MODE();
}

// Converts EBlendOp to D3D12_FILL_MODE
inline D3D12_BLEND_OP ConvertBlendOp(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
    case EBlendOp::Add:         return D3D12_BLEND_OP_ADD;
    case EBlendOp::Max:         return D3D12_BLEND_OP_MAX;
    case EBlendOp::Min:         return D3D12_BLEND_OP_MIN;
    case EBlendOp::RevSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case EBlendOp::Subtract:    return D3D12_BLEND_OP_SUBTRACT;
    }

    return D3D12_BLEND_OP();
}

// Converts EBlend to D3D12_BLEND
inline D3D12_BLEND ConvertBlend(EBlend Blend)
{
    switch (Blend)
    {
    case EBlend::Zero:           return D3D12_BLEND_ZERO;
    case EBlend::One:            return D3D12_BLEND_ONE;
    case EBlend::SrcColor:       return D3D12_BLEND_SRC_COLOR;
    case EBlend::InvSrcColor:    return D3D12_BLEND_INV_SRC_COLOR;
    case EBlend::SrcAlpha:       return D3D12_BLEND_SRC_ALPHA;
    case EBlend::InvSrcAlpha:    return D3D12_BLEND_INV_SRC_ALPHA;
    case EBlend::DestAlpha:      return D3D12_BLEND_DEST_ALPHA;
    case EBlend::InvDestAlpha:   return D3D12_BLEND_INV_DEST_ALPHA;
    case EBlend::DestColor:      return D3D12_BLEND_DEST_COLOR;
    case EBlend::InvDestColor:   return D3D12_BLEND_INV_DEST_COLOR;
    case EBlend::SrcAlphaSat:    return D3D12_BLEND_SRC_ALPHA_SAT;
    case EBlend::Src1Color:      return D3D12_BLEND_SRC1_COLOR;
    case EBlend::InvSrc1Color:   return D3D12_BLEND_INV_SRC1_COLOR;
    case EBlend::Src1Alpha:      return D3D12_BLEND_SRC1_ALPHA;
    case EBlend::InvSrc1Alpha:   return D3D12_BLEND_INV_SRC1_ALPHA;
    case EBlend::BlendFactor:    return D3D12_BLEND_BLEND_FACTOR;
    case EBlend::InvBlendFactor: return D3D12_BLEND_INV_BLEND_FACTOR;
    }

    return D3D12_BLEND();
}

// Converts ELogicOp to D3D12_LOGIC_OP
inline D3D12_LOGIC_OP ConvertLogicOp(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
    case ELogicOp::Clear:        return D3D12_LOGIC_OP_CLEAR;
    case ELogicOp::Set:          return D3D12_LOGIC_OP_SET;
    case ELogicOp::Copy:         return D3D12_LOGIC_OP_COPY;
    case ELogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
    case ELogicOp::Noop:         return D3D12_LOGIC_OP_NOOP;
    case ELogicOp::Invert:       return D3D12_LOGIC_OP_INVERT;
    case ELogicOp::And:          return D3D12_LOGIC_OP_AND;
    case ELogicOp::Nand:         return D3D12_LOGIC_OP_NAND;
    case ELogicOp::Or:           return D3D12_LOGIC_OP_OR;
    case ELogicOp::Nor:          return D3D12_LOGIC_OP_NOR;
    case ELogicOp::Xor:          return D3D12_LOGIC_OP_XOR;
    case ELogicOp::Equiv:        return D3D12_LOGIC_OP_EQUIV;
    case ELogicOp::AndReverse:   return D3D12_LOGIC_OP_AND_REVERSE;
    case ELogicOp::AndInverted:  return D3D12_LOGIC_OP_AND_INVERTED;
    case ELogicOp::OrReverse:    return D3D12_LOGIC_OP_OR_REVERSE;
    case ELogicOp::OrInverted:   return D3D12_LOGIC_OP_OR_INVERTED;
    }

    return D3D12_LOGIC_OP();
}

// Converts RenderTargetWriteState to D3D12 RenderTargetWriteMask
inline uint8 ConvertRenderTargetWriteState(const SRenderTargetWriteState& RenderTargetWriteState)
{
    uint8 RenderTargetWriteMask = 0;
    if (RenderTargetWriteState.WriteAll())
    {
        RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    else
    {
        if (RenderTargetWriteState.WriteRed())
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
        }
        if (RenderTargetWriteState.WriteGreen())
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
        }
        if (RenderTargetWriteState.WriteBlue())
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
        }
        if (RenderTargetWriteState.WriteAlpha())
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    return RenderTargetWriteMask;
}

// Converts EPrimitiveTopologyType to D3D12_PRIMITIVE_TOPOLOGY_TYPE
inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(EPrimitiveTopologyType PrimitiveTopologyType)
{
    switch (PrimitiveTopologyType)
    {
    case EPrimitiveTopologyType::Line:      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case EPrimitiveTopologyType::Patch:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    case EPrimitiveTopologyType::Point:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case EPrimitiveTopologyType::Triangle:  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case EPrimitiveTopologyType::Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }

    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

// Converts EPrimitiveTopology to D3D12_PRIMITIVE_TOPOLOGY
inline D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
    case EPrimitiveTopology::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case EPrimitiveTopology::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case EPrimitiveTopology::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case EPrimitiveTopology::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case EPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case EPrimitiveTopology::Undefined:     return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

// Converts EResourceState to D3D12_RESOURCE_STATES
inline D3D12_RESOURCE_STATES ConvertResourceState(EResourceState ResourceState)
{
    switch (ResourceState)
    {
    case EResourceState::Common:                  return D3D12_RESOURCE_STATE_COMMON;
    case EResourceState::CopyDest:                return D3D12_RESOURCE_STATE_COPY_DEST;
    case EResourceState::CopySource:              return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case EResourceState::DepthRead:               return D3D12_RESOURCE_STATE_DEPTH_READ;
    case EResourceState::DepthWrite:              return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case EResourceState::IndexBuffer:             return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case EResourceState::NonPixelShaderResource:  return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case EResourceState::PixelShaderResource:     return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case EResourceState::Present:                 return D3D12_RESOURCE_STATE_PRESENT;
    case EResourceState::RenderTarget:            return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case EResourceState::ResolveDest:             return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case EResourceState::ResolveSource:           return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case EResourceState::ShadingRateSource:       return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    case EResourceState::UnorderedAccess:         return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case EResourceState::VertexAndConstantBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case EResourceState::GenericRead:             return D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    return D3D12_RESOURCE_STATES();
}

// Converts ESamplerMode to D3D12_TEXTURE_ADDRESS_MODE
inline D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerMode(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::Wrap:       return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case ESamplerMode::Clamp:      return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case ESamplerMode::Mirror:     return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case ESamplerMode::Border:     return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case ESamplerMode::MirrorOnce: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    }

    return D3D12_TEXTURE_ADDRESS_MODE();
}

// Converts ESamplerFilter to D3D12_FILTER
inline D3D12_FILTER ConvertSamplerFilter(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ESamplerFilter::MinMagMipPoint:                          return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case ESamplerFilter::MinMagPoint_MipLinear:                   return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::MinPoint_MagMipLinear:                   return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    case ESamplerFilter::MinLinear_MagMipPoint:                   return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::MinMagLinear_MipPoint:                   return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::MinMagMipLinear:                         return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    case ESamplerFilter::Anistrotopic:                            return D3D12_FILTER_ANISOTROPIC;
    case ESamplerFilter::Comparison_MinMagMipPoint:               return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
    case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
    case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::Comparison_MinMagMipLinear:              return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    case ESamplerFilter::Comparison_Anistrotopic:                 return D3D12_FILTER_COMPARISON_ANISOTROPIC;
    }

    return D3D12_FILTER();
}

// Converts EShadingRate to D3D12_SHADING_RATE
inline D3D12_SHADING_RATE ConvertShadingRate(EShadingRate ShadingRate)
{
    switch (ShadingRate)
    {
    case EShadingRate::VRS_1x1: return D3D12_SHADING_RATE_1X1;
    case EShadingRate::VRS_1x2: return D3D12_SHADING_RATE_1X2;
    case EShadingRate::VRS_2x1: return D3D12_SHADING_RATE_2X1;
    case EShadingRate::VRS_2x2: return D3D12_SHADING_RATE_2X2;
    case EShadingRate::VRS_2x4: return D3D12_SHADING_RATE_2X4;
    case EShadingRate::VRS_4x2: return D3D12_SHADING_RATE_4X2;
    case EShadingRate::VRS_4x4: return D3D12_SHADING_RATE_4X4;
    }

    return D3D12_SHADING_RATE();
}

inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertAccelerationStructureBuildFlags(uint32 InFlags)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    if (InFlags & RayTracingStructureBuildFlag_AllowUpdate)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    if (InFlags & RayTracingStructureBuildFlag_PreferFastTrace)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    }
    if (InFlags & RayTracingStructureBuildFlag_PreferFastBuild)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    }

    return Flags;
}

inline D3D12_RAYTRACING_INSTANCE_FLAGS ConvertRayTracingInstanceFlags(uint32 InFlags)
{
    D3D12_RAYTRACING_INSTANCE_FLAGS Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    if (InFlags & RayTracingInstanceFlags_CullDisable)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    }
    if (InFlags & RayTracingInstanceFlags_FrontCounterClockwise)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    }
    if (InFlags & RayTracingInstanceFlags_ForceOpaque)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
    }
    if (InFlags & RayTracingInstanceFlags_ForceNonOpaque)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
    }

    return Flags;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// Operators for D3D12_GPU_DESCRIPTOR_HANDLE
inline bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle, uint64 Value)
{
    return DescriptorHandle.ptr == Value;
}

inline bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle, uint64 Value)
{
    return !(DescriptorHandle == Value);
}

inline bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE Left, D3D12_GPU_DESCRIPTOR_HANDLE Right)
{
    return Left.ptr == Right.ptr;
}

inline bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE Left, D3D12_GPU_DESCRIPTOR_HANDLE Right)
{
    return !(Left == Right);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// Operators for D3D12_CPU_DESCRIPTOR_HANDLE
inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle, uint64 Value)
{
    return DescriptorHandle.ptr == Value;
}

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle, uint64 Value)
{
    return !(DescriptorHandle == Value);
}

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE Left, D3D12_CPU_DESCRIPTOR_HANDLE Right)
{
    return Left.ptr == Right.ptr;
}

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE Left, D3D12_CPU_DESCRIPTOR_HANDLE Right)
{
    return !(Left == Right);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

inline uint32 GetFormatStride(DXGI_FORMAT Format)
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

inline DXGI_FORMAT CastShaderResourceFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        // TODO: Fix formats better
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:  return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:             return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R24G8_TYPELESS:        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;
    case DXGI_FORMAT_D16_UNORM:             return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
    default: return Format;
    }
}