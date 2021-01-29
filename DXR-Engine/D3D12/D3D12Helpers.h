#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/PipelineState.h"
#include "RenderLayer/Shader.h"
#include "RenderLayer/SamplerState.h"

#include <d3d12.h>
#include <dxcapi.h>

#include <wrl/client.h>

#define D3D12_DESCRIPTOR_HANDLE_INCREMENT(DescriptorHandle, Value) { (DescriptorHandle.ptr + Value) }

template<typename T>
using TComPtr = Microsoft::WRL::ComPtr<T>;

//Converts EBufferUsage- flags to D3D12_RESOURCE_FLAGS
inline D3D12_RESOURCE_FLAGS ConvertBufferUsage(UInt32 Usage)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (Usage & EBufferUsage::BufferUsage_UAV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return Result;
}

//Converts ETextureUsage- flags to D3D12_RESOURCE_FLAGS
inline D3D12_RESOURCE_FLAGS ConvertTextureUsage(UInt32 Usage)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (Usage & ETextureUsage::TextureUsage_UAV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (Usage & ETextureUsage::TextureUsage_RTV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (Usage & ETextureUsage::TextureUsage_DSV)
    {
        Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (!(Usage & ETextureUsage::TextureUsage_SRV))
        {
            Result |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

    return Result;
}

//Converts EFormat to DXGI_FORMAT
inline DXGI_FORMAT ConvertFormat(EFormat Format)
{
    switch (Format)
    {
    case EFormat::Format_R32G32B32A32_Typeless:    return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case EFormat::Format_R32G32B32A32_Float:       return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case EFormat::Format_R32G32B32A32_Uint:        return DXGI_FORMAT_R32G32B32A32_UINT;
    case EFormat::Format_R32G32B32A32_Sint:        return DXGI_FORMAT_R32G32B32A32_SINT;
    case EFormat::Format_R32G32B32_Typeless:       return DXGI_FORMAT_R32G32B32_TYPELESS;
    case EFormat::Format_R32G32B32_Float:          return DXGI_FORMAT_R32G32B32_FLOAT;
    case EFormat::Format_R32G32B32_Uint:           return DXGI_FORMAT_R32G32B32_UINT;
    case EFormat::Format_R32G32B32_Sint:           return DXGI_FORMAT_R32G32B32_SINT;
    case EFormat::Format_R16G16B16A16_Typeless:    return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case EFormat::Format_R16G16B16A16_Float:       return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case EFormat::Format_R16G16B16A16_Unorm:       return DXGI_FORMAT_R16G16B16A16_UNORM;
    case EFormat::Format_R16G16B16A16_Uint:        return DXGI_FORMAT_R16G16B16A16_UINT;
    case EFormat::Format_R16G16B16A16_Snorm:       return DXGI_FORMAT_R16G16B16A16_SNORM;
    case EFormat::Format_R16G16B16A16_Sint:        return DXGI_FORMAT_R16G16B16A16_SINT;
    case EFormat::Format_R32G32_Typeless:          return DXGI_FORMAT_R32G32_TYPELESS;
    case EFormat::Format_R32G32_Float:             return DXGI_FORMAT_R32G32_FLOAT;
    case EFormat::Format_R32G32_Uint:              return DXGI_FORMAT_R32G32_UINT;
    case EFormat::Format_R32G32_Sint:              return DXGI_FORMAT_R32G32_SINT;
    case EFormat::Format_R32G8X24_Typeless:        return DXGI_FORMAT_R32G8X24_TYPELESS;
    case EFormat::Format_D32_Float_S8X24_Uint:     return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case EFormat::Format_R32_Float_X8X24_Typeless: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case EFormat::Format_X32_Typeless_G8X24_Uint:  return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case EFormat::Format_R10G10B10A2_Typeless:     return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case EFormat::Format_R10G10B10A2_Unorm:        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case EFormat::Format_R10G10B10A2_Uint:         return DXGI_FORMAT_R10G10B10A2_UINT;
    case EFormat::Format_R11G11B10_Float:          return DXGI_FORMAT_R11G11B10_FLOAT;
    case EFormat::Format_R8G8B8A8_Typeless:        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case EFormat::Format_R8G8B8A8_Unorm:           return DXGI_FORMAT_R8G8B8A8_UNORM;
    case EFormat::Format_R8G8B8A8_Unorm_SRGB:      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case EFormat::Format_R8G8B8A8_Uint:            return DXGI_FORMAT_R8G8B8A8_UINT;
    case EFormat::Format_R8G8B8A8_Snorm:           return DXGI_FORMAT_R8G8B8A8_SNORM;
    case EFormat::Format_R8G8B8A8_Sint:            return DXGI_FORMAT_R8G8B8A8_SINT;
    case EFormat::Format_R16G16_Typeless:          return DXGI_FORMAT_R16G16_TYPELESS;
    case EFormat::Format_R16G16_Float:             return DXGI_FORMAT_R16G16_FLOAT;
    case EFormat::Format_R16G16_Unorm:             return DXGI_FORMAT_R16G16_UNORM;
    case EFormat::Format_R16G16_Uint:              return DXGI_FORMAT_R16G16_UINT;
    case EFormat::Format_R16G16_Snorm:             return DXGI_FORMAT_R16G16_SNORM;
    case EFormat::Format_R16G16_Sint:              return DXGI_FORMAT_R16G16_SINT;
    case EFormat::Format_R32_Typeless:             return DXGI_FORMAT_R32_TYPELESS;
    case EFormat::Format_D32_Float:                return DXGI_FORMAT_D32_FLOAT;
    case EFormat::Format_R32_Float:                return DXGI_FORMAT_R32_FLOAT;
    case EFormat::Format_R32_Uint:                 return DXGI_FORMAT_R32_UINT;
    case EFormat::Format_R32_Sint:                 return DXGI_FORMAT_R32_SINT;
    case EFormat::Format_R24G8_Typeless:           return DXGI_FORMAT_R24G8_TYPELESS;
    case EFormat::Format_D24_Unorm_S8_Uint:        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case EFormat::Format_R24_Unorm_X8_Typeless:    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case EFormat::Format_X24_Typeless_G8_Uint:     return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    case EFormat::Format_R8G8_Typeless:            return DXGI_FORMAT_R8G8_TYPELESS;
    case EFormat::Format_R8G8_Unorm:               return DXGI_FORMAT_R8G8_UNORM;
    case EFormat::Format_R8G8_Uint:                return DXGI_FORMAT_R8G8_UINT;
    case EFormat::Format_R8G8_Snorm:               return DXGI_FORMAT_R8G8_SNORM;
    case EFormat::Format_R8G8_Sint:                return DXGI_FORMAT_R8G8_SINT;
    case EFormat::Format_R16_Typeless:             return DXGI_FORMAT_R16_TYPELESS;
    case EFormat::Format_R16_Float:                return DXGI_FORMAT_R16_FLOAT;
    case EFormat::Format_D16_Unorm:                return DXGI_FORMAT_D16_UNORM;
    case EFormat::Format_R16_Unorm:                return DXGI_FORMAT_R16_UNORM;
    case EFormat::Format_R16_Uint:                 return DXGI_FORMAT_R16_UINT;
    case EFormat::Format_R16_Snorm:                return DXGI_FORMAT_R16_SNORM;
    case EFormat::Format_R16_Sint:                 return DXGI_FORMAT_R16_SINT;
    case EFormat::Format_R8_Typeless:              return DXGI_FORMAT_R8_TYPELESS;
    case EFormat::Format_R8_Unorm:                 return DXGI_FORMAT_R8_UNORM;
    case EFormat::Format_R8_Uint:                  return DXGI_FORMAT_R8_UINT;
    case EFormat::Format_R8_Snorm:                 return DXGI_FORMAT_R8_SNORM;
    case EFormat::Format_R8_Sint:                  return DXGI_FORMAT_R8_SINT;
    case EFormat::Format_A8_Unorm:                 return DXGI_FORMAT_A8_UNORM;
    case EFormat::Format_R1_Unorm:                 return DXGI_FORMAT_R1_UNORM;
    case EFormat::Format_B5G6R5_Unorm:             return DXGI_FORMAT_B5G6R5_UNORM;
    case EFormat::Format_B5G5R5A1_Unorm:           return DXGI_FORMAT_B5G5R5A1_UNORM;
    case EFormat::Format_B8G8R8A8_Unorm:           return DXGI_FORMAT_B8G8R8A8_UNORM;
    case EFormat::Format_B8G8R8X8_Unorm:           return DXGI_FORMAT_B8G8R8X8_UNORM;
    case EFormat::Format_B8G8R8A8_Typeless:        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case EFormat::Format_B8G8R8A8_Unorm_SRGB:      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case EFormat::Format_B8G8R8X8_Typeless:        return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case EFormat::Format_B8G8R8X8_Unorm_SRGB:      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    default:                                       return DXGI_FORMAT_UNKNOWN;
    }
}

//Converts EInputClassification to D3D12_INPUT_CLASSIFICATION
inline D3D12_INPUT_CLASSIFICATION ConvertInputClassification(EInputClassification InputClassification)
{
    switch (InputClassification)
    {
    case EInputClassification::InputClassification_Instance: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    case EInputClassification::InputClassification_Vertex:   return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }

    return D3D12_INPUT_CLASSIFICATION();
}

//Converts EDepthWriteMask to DXGI_FORMAT
inline D3D12_DEPTH_WRITE_MASK ConvertDepthWriteMask(EDepthWriteMask DepthWriteMask)
{
    switch (DepthWriteMask)
    {
    case EDepthWriteMask::DepthWriteMask_Zero: return D3D12_DEPTH_WRITE_MASK_ZERO;
    case EDepthWriteMask::DepthWriteMask_All:  return D3D12_DEPTH_WRITE_MASK_ALL;
    }

    return D3D12_DEPTH_WRITE_MASK();
}

//Converts EComparisonFunc to D3D12_COMPARISON_FUNC
inline D3D12_COMPARISON_FUNC ConvertComparisonFunc(EComparisonFunc ComparisonFunc)
{
    switch (ComparisonFunc)
    {
    case EComparisonFunc::ComparisonFunc_Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case EComparisonFunc::ComparisonFunc_Less:         return D3D12_COMPARISON_FUNC_LESS;
    case EComparisonFunc::ComparisonFunc_Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case EComparisonFunc::ComparisonFunc_LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case EComparisonFunc::ComparisonFunc_Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case EComparisonFunc::ComparisonFunc_NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case EComparisonFunc::ComparisonFunc_GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case EComparisonFunc::ComparisonFunc_Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    return D3D12_COMPARISON_FUNC();
}

//Converts EStencilOp to D3D12_STENCIL_OP
inline D3D12_STENCIL_OP ConvertStencilOp(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
    case EStencilOp::StencilOp_Keep:    return D3D12_STENCIL_OP_KEEP;
    case EStencilOp::StencilOp_Zero:    return D3D12_STENCIL_OP_ZERO;
    case EStencilOp::StencilOp_Replace: return D3D12_STENCIL_OP_REPLACE;
    case EStencilOp::StencilOp_IncrSat: return D3D12_STENCIL_OP_INCR_SAT;
    case EStencilOp::StencilOp_DecrSat: return D3D12_STENCIL_OP_DECR_SAT;
    case EStencilOp::StencilOp_Invert:  return D3D12_STENCIL_OP_INVERT;
    case EStencilOp::StencilOp_Incr:    return D3D12_STENCIL_OP_INCR;
    case EStencilOp::StencilOp_Decr:    return D3D12_STENCIL_OP_DECR;
    }

    return D3D12_STENCIL_OP();
}

//Converts DepthStencilOp to D3D12_DEPTH_STENCILOP_DESC
inline D3D12_DEPTH_STENCILOP_DESC ConvertDepthStencilOp(const DepthStencilOp& DepthStencilOp)
{
    return
    {
        ConvertStencilOp(DepthStencilOp.StencilFailOp),
        ConvertStencilOp(DepthStencilOp.StencilDepthFailOp),
        ConvertStencilOp(DepthStencilOp.StencilPassOp),
        ConvertComparisonFunc(DepthStencilOp.StencilFunc)
    };
}

//Converts ECullMode to D3D12_CULL_MODE
inline D3D12_CULL_MODE ConvertCullMode(ECullMode CullMode)
{
    switch (CullMode)
    {
    case ECullMode::CullMode_Back:  return D3D12_CULL_MODE_BACK;
    case ECullMode::CullMode_Front: return D3D12_CULL_MODE_FRONT;
    default:                        return D3D12_CULL_MODE_NONE;
    }
}

//Converts EFillMode to D3D12_FILL_MODE
inline D3D12_FILL_MODE ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
    case EFillMode::FillMode_Solid:     return D3D12_FILL_MODE_SOLID;
    case EFillMode::FillMode_WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    }

    return D3D12_FILL_MODE();
}

//Converts EBlendOp to D3D12_FILL_MODE
inline D3D12_BLEND_OP ConvertBlendOp(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
    case EBlendOp::BlendOp_Add:         return D3D12_BLEND_OP_ADD;
    case EBlendOp::BlendOp_Max:         return D3D12_BLEND_OP_MAX;
    case EBlendOp::BlendOp_Min:         return D3D12_BLEND_OP_MIN;
    case EBlendOp::BlendOp_RevSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case EBlendOp::BlendOp_Subtract:    return D3D12_BLEND_OP_SUBTRACT;
    }

    return D3D12_BLEND_OP();
}

//Converts EBlend to D3D12_BLEND
inline D3D12_BLEND ConvertBlend(EBlend Blend)
{
    switch (Blend)
    {
    case EBlend::Blend_Zero:           return D3D12_BLEND_ZERO;
    case EBlend::Blend_One:            return D3D12_BLEND_ONE;
    case EBlend::Blend_SrcColor:       return D3D12_BLEND_SRC_COLOR;
    case EBlend::Blend_InvSrcColor:    return D3D12_BLEND_INV_SRC_COLOR;
    case EBlend::Blend_SrcAlpha:       return D3D12_BLEND_SRC_ALPHA;
    case EBlend::Blend_InvSrcAlpha:    return D3D12_BLEND_INV_SRC_ALPHA;
    case EBlend::Blend_DestAlpha:      return D3D12_BLEND_DEST_ALPHA;
    case EBlend::Blend_InvDestAlpha:   return D3D12_BLEND_INV_DEST_ALPHA;
    case EBlend::Blend_DestColor:      return D3D12_BLEND_DEST_COLOR;
    case EBlend::Blend_InvDestColor:   return D3D12_BLEND_INV_DEST_COLOR;
    case EBlend::Blend_SrcAlphaSat:    return D3D12_BLEND_SRC_ALPHA_SAT;
    case EBlend::Blend_Src1Color:      return D3D12_BLEND_SRC1_COLOR;
    case EBlend::Blend_InvSrc1Color:   return D3D12_BLEND_INV_SRC1_COLOR;
    case EBlend::Blend_Src1Alpha:      return D3D12_BLEND_SRC1_ALPHA;
    case EBlend::Blend_InvSrc1Alpha:   return D3D12_BLEND_INV_SRC1_ALPHA;
    case EBlend::Blend_BlendFactor:    return D3D12_BLEND_BLEND_FACTOR;
    case EBlend::Blend_InvBlendFactor: return D3D12_BLEND_INV_BLEND_FACTOR;
    }

    return D3D12_BLEND();
}

//Converts ELogicOp to D3D12_LOGIC_OP
inline D3D12_LOGIC_OP ConvertLogicOp(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
    case ELogicOp::LogicOp_Clear:        return D3D12_LOGIC_OP_CLEAR;
    case ELogicOp::LogicOp_Set:          return D3D12_LOGIC_OP_SET;
    case ELogicOp::LogicOp_Copy:         return D3D12_LOGIC_OP_COPY;
    case ELogicOp::LogicOp_CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
    case ELogicOp::LogicOp_Noop:         return D3D12_LOGIC_OP_NOOP;
    case ELogicOp::LogicOp_Invert:       return D3D12_LOGIC_OP_INVERT;
    case ELogicOp::LogicOp_And:          return D3D12_LOGIC_OP_AND;
    case ELogicOp::LogicOp_Nand:         return D3D12_LOGIC_OP_NAND;
    case ELogicOp::LogicOp_Or:           return D3D12_LOGIC_OP_OR;
    case ELogicOp::LogicOp_Nor:          return D3D12_LOGIC_OP_NOR;
    case ELogicOp::LogicOp_Xor:          return D3D12_LOGIC_OP_XOR;
    case ELogicOp::LogicOp_Equiv:        return D3D12_LOGIC_OP_EQUIV;
    case ELogicOp::LogicOp_AndReverse:   return D3D12_LOGIC_OP_AND_REVERSE;
    case ELogicOp::LogicOp_AndInverted:  return D3D12_LOGIC_OP_AND_INVERTED;
    case ELogicOp::LogicOp_OrReverse:    return D3D12_LOGIC_OP_OR_REVERSE;
    case ELogicOp::LogicOp_OrInverted:   return D3D12_LOGIC_OP_OR_INVERTED;
    }

    return D3D12_LOGIC_OP();
}

//Converts RenderTargetWriteState to D3D12 RenderTargetWriteMask
inline UInt8 ConvertRenderTargetWriteState(const RenderTargetWriteState& RenderTargetWriteState)
{
    UInt8 RenderTargetWriteMask = 0;
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

//Converts EPrimitiveTopologyType to D3D12_PRIMITIVE_TOPOLOGY_TYPE
inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(EPrimitiveTopologyType PrimitiveTopologyType)
{
    switch (PrimitiveTopologyType)
    {
    case EPrimitiveTopologyType::PrimitiveTopologyType_Line:      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case EPrimitiveTopologyType::PrimitiveTopologyType_Patch:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    case EPrimitiveTopologyType::PrimitiveTopologyType_Point:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case EPrimitiveTopologyType::PrimitiveTopologyType_Triangle:  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case EPrimitiveTopologyType::PrimitiveTopologyType_Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }

    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

//Converts EPrimitiveTopology to D3D12_PRIMITIVE_TOPOLOGY
inline D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
    case EPrimitiveTopology::PrimitiveTopology_LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case EPrimitiveTopology::PrimitiveTopology_LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case EPrimitiveTopology::PrimitiveTopology_PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case EPrimitiveTopology::PrimitiveTopology_TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case EPrimitiveTopology::PrimitiveTopology_TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case EPrimitiveTopology::PrimitiveTopology_Undefined:     return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

//Converts EResourceState to D3D12_RESOURCE_STATES
inline D3D12_RESOURCE_STATES ConvertResourceState(EResourceState ResourceState)
{
    switch (ResourceState)
    {
    case EResourceState::ResourceState_Common:                  return D3D12_RESOURCE_STATE_COMMON;
    case EResourceState::ResourceState_CopyDest:                return D3D12_RESOURCE_STATE_COPY_DEST;
    case EResourceState::ResourceState_CopySource:              return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case EResourceState::ResourceState_DepthRead:               return D3D12_RESOURCE_STATE_DEPTH_READ;
    case EResourceState::ResourceState_DepthWrite:              return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case EResourceState::ResourceState_IndexBuffer:             return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case EResourceState::ResourceState_NonPixelShaderResource:  return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case EResourceState::ResourceState_PixelShaderResource:     return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case EResourceState::ResourceState_Present:                 return D3D12_RESOURCE_STATE_PRESENT;
    case EResourceState::ResourceState_RenderTarget:            return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case EResourceState::ResourceState_ResolveDest:             return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case EResourceState::ResourceState_ResolveSource:           return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case EResourceState::ResourceState_ShadingRateSource:       return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    case EResourceState::ResourceState_UnorderedAccess:         return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case EResourceState::ResourceState_VertexAndConstantBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case EResourceState::ResourceState_GenericRead:             return D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    return D3D12_RESOURCE_STATES();
}

//Converts ESamplerMode to D3D12_TEXTURE_ADDRESS_MODE
inline D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerMode(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::SamplerMode_Wrap:       return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case ESamplerMode::SamplerMode_Clamp:      return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case ESamplerMode::SamplerMode_Mirror:     return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case ESamplerMode::SamplerMode_Border:     return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case ESamplerMode::SamplerMode_MirrorOnce: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    }

    return D3D12_TEXTURE_ADDRESS_MODE();
}

//Converts ESamplerFilter to D3D12_FILTER
inline D3D12_FILTER ConvertSamplerFilter(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ESamplerFilter::SamplerFilter_MinMagMipPoint:                          return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case ESamplerFilter::SamplerFilter_MinMagPoint_MipLinear:                   return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_MinPoint_MagLinear_MipPoint:             return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::SamplerFilter_MinPoint_MagMipLinear:                   return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_MinLinear_MagMipPoint:                   return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    case ESamplerFilter::SamplerFilter_MinLinear_MagPoint_MipLinear:            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_MinMagLinear_MipPoint:                   return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::SamplerFilter_MinMagMipLinear:                         return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_Anistrotopic:                            return D3D12_FILTER_ANISOTROPIC;
    case ESamplerFilter::SamplerFilter_Comparison_MinMagMipPoint:               return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    case ESamplerFilter::SamplerFilter_Comparison_MinMagPoint_MipLinear:        return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_Comparison_MinPoint_MagLinear_MipPoint:  return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::SamplerFilter_Comparison_MinPoint_MagMipLinear:        return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_Comparison_MinLinear_MagMipPoint:        return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
    case ESamplerFilter::SamplerFilter_Comparison_MinLinear_MagPoint_MipLinear: return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_Comparison_MinMagLinear_MipPoint:        return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::SamplerFilter_Comparison_MinMagMipLinear:              return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    case ESamplerFilter::SamplerFilter_Comparison_Anistrotopic:                 return D3D12_FILTER_COMPARISON_ANISOTROPIC;
    }

    return D3D12_FILTER();
}

//Converts EShadingRate to D3D12_SHADING_RATE
inline D3D12_SHADING_RATE ConvertShadingRate(EShadingRate ShadingRate)
{
    switch (ShadingRate)
    {
    case EShadingRate::ShadingRate_1x1: return D3D12_SHADING_RATE_1X1;
    case EShadingRate::ShadingRate_2x2: return D3D12_SHADING_RATE_2X2;
    case EShadingRate::ShadingRate_4x4: return D3D12_SHADING_RATE_4X4;
    }

    return D3D12_SHADING_RATE();
}

//Operators for D3D12_GPU_DESCRIPTOR_HANDLE
inline Bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle, UInt64 Value)
{
    return DescriptorHandle.ptr == Value;
}

inline Bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle, UInt64 Value)
{
    return !(DescriptorHandle == Value);
}

inline Bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE Left, D3D12_GPU_DESCRIPTOR_HANDLE Right)
{
    return Left.ptr == Right.ptr;
}

inline Bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE Left, D3D12_GPU_DESCRIPTOR_HANDLE Right)
{
    return !(Left == Right);
}

//Operators for D3D12_CPU_DESCRIPTOR_HANDLE
inline Bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle, UInt64 Value)
{
    return DescriptorHandle.ptr == Value;
}

inline Bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle, UInt64 Value)
{
    return !(DescriptorHandle == Value);
}

inline Bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE Left, D3D12_CPU_DESCRIPTOR_HANDLE Right)
{
    return Left.ptr == Right.ptr;
}

inline Bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE Left, D3D12_CPU_DESCRIPTOR_HANDLE Right)
{
    return !(Left == Right);
}

//Other helpers
inline Bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::ShaderStage_Vertex:
        case EShaderStage::ShaderStage_Hull:
        case EShaderStage::ShaderStage_Domain:
        case EShaderStage::ShaderStage_Geometry:
        case EShaderStage::ShaderStage_Pixel:
        case EShaderStage::ShaderStage_Mesh:
        case EShaderStage::ShaderStage_Amplification:
        {
            return true;
        }

        default: 
        {
            return false;
        }
    }
}

inline Bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::ShaderStage_Compute:
        case EShaderStage::ShaderStage_RayGen:
        case EShaderStage::ShaderStage_RayClosestHit:
        case EShaderStage::ShaderStage_RayAnyHit:
        case EShaderStage::ShaderStage_RayMiss:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

inline UInt32 GetFormatStride(DXGI_FORMAT Format)
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