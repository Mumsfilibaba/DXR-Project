#pragma once
#include "Core/Misc/Debug.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/ComPtr.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12Constants.h"

#define D3D12_DESCRIPTOR_HANDLE_INCREMENT(DescriptorHandle, Value) { (DescriptorHandle.ptr + Value) }

// Windows 10 1507 
#if (NTDDI_WIN10 && (WDK_NTDDI_VERSION >= NTDDI_WIN10))
    #define WIN10_BUILD_10240 (1)
#endif
// Windows 10 1511 (November Update)
#if (NTDDI_WIN10_TH2 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_TH2))
    #define WIN10_BUILD_10586 (1)
#endif
// Windows 10 1607 (Anniversary Update)
#if (NTDDI_WIN10_RS1 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS1))
    #define WIN10_BUILD_14393 (1)
#endif
// Windows 10 1703 (Creators Update)
#if (NTDDI_WIN10_RS2 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS2))
    #define WIN10_BUILD_15063 (1)
#endif
// Windows 10 1709 (Fall Creators Update)
#if (NTDDI_WIN10_RS3 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS3))
    #define WIN10_BUILD_16299 (1)
#endif
// Windows 10 1803 (April 2018 Update)
#if (NTDDI_WIN10_RS4 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS4))
    #define WIN10_BUILD_17134 (1)
#endif
// Windows 10 1809 (October 2018 Update)
#if (NTDDI_WIN10_RS5 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS5))
    #define WIN10_BUILD_17763 (1)
#endif
// Windows 10 1903 (May 2019 Update)
#if (NTDDI_WIN10_19H1 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_19H1))
    #define WIN10_BUILD_18362 (1)
#endif
// Windows 10 2004 (May 2020 Update)
#if (NTDDI_WIN10_VB && (WDK_NTDDI_VERSION >= NTDDI_WIN10_VB))
    #define WIN10_BUILD_19041 (1)
#endif
// Windows 10 2104
#if (NTDDI_WIN10_FE && (WDK_NTDDI_VERSION >= NTDDI_WIN10_FE))
    #define WIN10_BUILD_20348 (1)
#endif
// Windows 11 21H2
#if (NTDDI_WIN10_CO && (WDK_NTDDI_VERSION >= NTDDI_WIN10_CO))
    #define WIN11_BUILD_22000 (1)
#endif
// Windows 11 22H2
#if (NTDDI_WIN10_NI && (WDK_NTDDI_VERSION >= NTDDI_WIN10_NI))
    #define WIN11_BUILD_22621 (1)
#endif

#if !PRODUCTION_BUILD
    #define D3D12_ERROR(...) \
        do \
        { \
            LOG_ERROR("[D3D12RHI] "__VA_ARGS__); \
            DEBUG_BREAK(); \
        } while (false)
    
    #define D3D12_ERROR_COND(bCondition, ...) \
        do \
        { \
            if (!(bCondition)) \
            { \
                D3D12_ERROR(__VA_ARGS__); \
            } \
        } while (false)
    
    #define D3D12_WARNING(...) \
        do \
        { \
            LOG_WARNING("[D3D12RHI] "__VA_ARGS__); \
        } while (false)

    #define D3D12_WARNING_COND(bCondition, ...) \
        do \
        { \
            if (!(bCondition)) \
            { \
                D3D12_WARNING(__VA_ARGS__); \
            } \
        } while (false)

    #define D3D12_INFO(...) \
        do \
        { \
            LOG_INFO("[D3D12RHI] "__VA_ARGS__); \
        } while (false)
#else
    #define D3D12_ERROR_COND(bCondition, ...) do { (void)(bCondition); } while(false)
    #define D3D12_ERROR(...)  do { (void)(0); } while(false)

    #define D3D12_WARNING_COND(bCondition, ...) do { (void)(bCondition); } while(false)
    #define D3D12_WARNING(...) do { (void)(0); } while(false)

    #define D3D12_INFO(...) do { (void)(0); } while(false)
#endif

void D3D12DeviceRemovedHandlerRHI(class FD3D12Device* Device);

inline D3D12_HEAP_PROPERTIES GetUploadHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    FMemory::Memzero(&HeapProperties);

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = 1;
    HeapProperties.CreationNodeMask     = 1;

    return HeapProperties;
}

inline D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    FMemory::Memzero(&HeapProperties);

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = 1;
    HeapProperties.CreationNodeMask     = 1;

    return HeapProperties;
}

enum class ED3D12CommandQueueType
{
    Direct  = 0,
    Compute = 1,
    Copy    = 2,
    Count
};

constexpr const CHAR* ToString(ED3D12CommandQueueType QueueType)
{
    switch (QueueType)
    {
        case ED3D12CommandQueueType::Direct:  return "Default";
        case ED3D12CommandQueueType::Compute: return "Compute";
        case ED3D12CommandQueueType::Copy:    return "Copy";
    }

    return "CommandQueueType::Unknown";
}

constexpr D3D12_COMMAND_LIST_TYPE ToCommandListType(ED3D12CommandQueueType QueueType)
{
    switch (QueueType)
    {
        case ED3D12CommandQueueType::Direct:  return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case ED3D12CommandQueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case ED3D12CommandQueueType::Copy:    return D3D12_COMMAND_LIST_TYPE_COPY;
    }

    return D3D12_COMMAND_LIST_TYPE(-1);
}

constexpr D3D12_QUERY_HEAP_TYPE ToQueryHeapType(EQueryType QueryType)
{
    switch (QueryType)
    {
        case EQueryType::Timestamp: return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        case EQueryType::Occlusion: return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
    }

    return D3D12_QUERY_HEAP_TYPE(-1);
}

constexpr uint32 GetBufferAlignment(EBufferUsageFlags BufferFlags)
{
    // Constant buffers require special alignment
    if (IsEnumFlagSet(BufferFlags, EBufferUsageFlags::ConstantBuffer))
    {
        return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }
    else
    {
        // Otherwise, return a default of 16
        return 16;
    }
}

constexpr D3D12_RESOURCE_FLAGS ConvertBufferFlags(EBufferUsageFlags Flags)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (IsEnumFlagSet(Flags, EBufferUsageFlags::UnorderedAccess))
    {
        Result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return Result;
}

constexpr D3D12_RESOURCE_FLAGS ConvertTextureFlags(ETextureUsageFlags Flag)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::UnorderedAccess))
    {
        Result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::RenderTarget))
    {
        Result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }

    const bool bAllowDSV = IsEnumFlagSet(Flag, ETextureUsageFlags::DepthStencil);
    if (bAllowDSV)
    {
        Result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    const bool bAllowSRV = IsEnumFlagSet(Flag, ETextureUsageFlags::ShaderResource);
    if (bAllowDSV && !bAllowSRV)
    {
        Result |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return Result;
}

constexpr D3D12_RESOURCE_DIMENSION ConvertTextureDimension(ETextureDimension TextureDimension)
{
    switch (TextureDimension)
    {
        case ETextureDimension::Texture2D:
        case ETextureDimension::Texture2DArray:
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case ETextureDimension::Texture3D:        return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        default:                                  return D3D12_RESOURCE_DIMENSION_UNKNOWN;
    }
}

constexpr DXGI_FORMAT ConvertFormat(EFormat Format)
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

        default:                             return DXGI_FORMAT_UNKNOWN;
    }
}

constexpr DXGI_FORMAT ConvertIndexFormat(EIndexFormat IndexFormat)
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

constexpr D3D12_INPUT_CLASSIFICATION ConvertVertexInputClass(EVertexInputClass InputClassification)
{
    switch (InputClassification)
    {
        case EVertexInputClass::Instance: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        case EVertexInputClass::Vertex:   return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }

    return D3D12_INPUT_CLASSIFICATION(-1);
}

constexpr D3D12_COMPARISON_FUNC ConvertComparisonFunc(EComparisonFunc ComparisonFunc)
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

    return D3D12_COMPARISON_FUNC_NONE;
}

constexpr D3D12_STENCIL_OP ConvertStencilOp(EStencilOp StencilOp)
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

    return D3D12_STENCIL_OP(-1);
}

inline D3D12_DEPTH_STENCILOP_DESC ConvertStencilState(const FStencilState& StencilState)
{
    return
    {
        ConvertStencilOp(StencilState.StencilFailOp),
        ConvertStencilOp(StencilState.StencilDepthFailOp),
        ConvertStencilOp(StencilState.StencilDepthPassOp),
        ConvertComparisonFunc(StencilState.StencilFunc)
    };
}

constexpr D3D12_CULL_MODE ConvertCullMode(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
        case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
        default:               return D3D12_CULL_MODE_NONE;
    }
}

constexpr D3D12_FILL_MODE ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
        case EFillMode::WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    }

    return D3D12_FILL_MODE();
}

constexpr D3D12_BLEND_OP ConvertBlendOp(EBlendOp BlendOp)
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

constexpr D3D12_BLEND ConvertBlend(EBlendType  Blend)
{
    switch (Blend)
    {
        case EBlendType ::Zero:           return D3D12_BLEND_ZERO;
        case EBlendType ::One:            return D3D12_BLEND_ONE;
        case EBlendType ::SrcColor:       return D3D12_BLEND_SRC_COLOR;
        case EBlendType ::InvSrcColor:    return D3D12_BLEND_INV_SRC_COLOR;
        case EBlendType ::SrcAlpha:       return D3D12_BLEND_SRC_ALPHA;
        case EBlendType ::InvSrcAlpha:    return D3D12_BLEND_INV_SRC_ALPHA;
        case EBlendType ::DstAlpha:       return D3D12_BLEND_DEST_ALPHA;
        case EBlendType ::InvDstAlpha:    return D3D12_BLEND_INV_DEST_ALPHA;
        case EBlendType ::DstColor:       return D3D12_BLEND_DEST_COLOR;
        case EBlendType ::InvDstColor:    return D3D12_BLEND_INV_DEST_COLOR;
        case EBlendType ::SrcAlphaSat:    return D3D12_BLEND_SRC_ALPHA_SAT;
        case EBlendType ::Src1Color:      return D3D12_BLEND_SRC1_COLOR;
        case EBlendType ::InvSrc1Color:   return D3D12_BLEND_INV_SRC1_COLOR;
        case EBlendType ::Src1Alpha:      return D3D12_BLEND_SRC1_ALPHA;
        case EBlendType ::InvSrc1Alpha:   return D3D12_BLEND_INV_SRC1_ALPHA;
        case EBlendType ::BlendFactor:    return D3D12_BLEND_BLEND_FACTOR;
        case EBlendType ::InvBlendFactor: return D3D12_BLEND_INV_BLEND_FACTOR;
    }

    return D3D12_BLEND();
}

constexpr D3D12_LOGIC_OP ConvertLogicOp(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
        case ELogicOp::Clear:        return D3D12_LOGIC_OP_CLEAR;
        case ELogicOp::Set:          return D3D12_LOGIC_OP_SET;
        case ELogicOp::Copy:         return D3D12_LOGIC_OP_COPY;
        case ELogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
        case ELogicOp::NoOp:         return D3D12_LOGIC_OP_NOOP;
        case ELogicOp::Invert:       return D3D12_LOGIC_OP_INVERT;
        case ELogicOp::And:          return D3D12_LOGIC_OP_AND;
        case ELogicOp::Nand:         return D3D12_LOGIC_OP_NAND;
        case ELogicOp::Or:           return D3D12_LOGIC_OP_OR;
        case ELogicOp::Nor:          return D3D12_LOGIC_OP_NOR;
        case ELogicOp::Xor:          return D3D12_LOGIC_OP_XOR;
        case ELogicOp::Equivalent:   return D3D12_LOGIC_OP_EQUIV;
        case ELogicOp::AndReverse:   return D3D12_LOGIC_OP_AND_REVERSE;
        case ELogicOp::AndInverted:  return D3D12_LOGIC_OP_AND_INVERTED;
        case ELogicOp::OrReverse:    return D3D12_LOGIC_OP_OR_REVERSE;
        case ELogicOp::OrInverted:   return D3D12_LOGIC_OP_OR_INVERTED;
    }

    return D3D12_LOGIC_OP();
}

constexpr uint8 ConvertColorWriteFlags(EColorWriteFlags ColorWriteFlags)
{
    uint8 RenderTargetWriteMask = 0;
    if (ColorWriteFlags == EColorWriteFlags::All)
    {
        RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    else
    {
        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Red))
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
        }
        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Green))
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
        }
        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Blue))
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
        }
        if (IsEnumFlagSet(ColorWriteFlags, EColorWriteFlags::Alpha))
        {
            RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    return RenderTargetWriteMask;
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
        case EPrimitiveTopology::LineList:
        case EPrimitiveTopology::LineStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        case EPrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        case EPrimitiveTopology::TriangleList:
        case EPrimitiveTopology::TriangleStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

constexpr D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
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

constexpr D3D12_RESOURCE_STATES ConvertResourceState(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
        case EResourceAccess::Common:                 return D3D12_RESOURCE_STATE_COMMON;
        case EResourceAccess::CopyDest:               return D3D12_RESOURCE_STATE_COPY_DEST;
        case EResourceAccess::CopySource:             return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case EResourceAccess::DepthRead:              return D3D12_RESOURCE_STATE_DEPTH_READ;
        case EResourceAccess::DepthWrite:             return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case EResourceAccess::IndexBuffer:            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case EResourceAccess::VertexBuffer:           return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case EResourceAccess::NonPixelShaderResource: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case EResourceAccess::PixelShaderResource:    return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case EResourceAccess::Present:                return D3D12_RESOURCE_STATE_PRESENT;
        case EResourceAccess::RenderTarget:           return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case EResourceAccess::ResolveDest:            return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case EResourceAccess::ResolveSource:          return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        case EResourceAccess::ShadingRateSource:      return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        case EResourceAccess::UnorderedAccess:        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case EResourceAccess::ConstantBuffer:         return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case EResourceAccess::GenericRead:            return D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    return D3D12_RESOURCE_STATES();
}

constexpr D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerMode(ESamplerMode SamplerMode)
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

constexpr D3D12_FILTER ConvertSamplerFilter(ESamplerFilter SamplerFilter)
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

constexpr D3D12_SHADING_RATE ConvertShadingRate(EShadingRate ShadingRate)
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

constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertAccelerationStructureBuildFlags(EAccelerationStructureBuildFlags InFlags)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    if ((InFlags & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    if ((InFlags & EAccelerationStructureBuildFlags::PreferFastTrace) != EAccelerationStructureBuildFlags::None)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    }
    if ((InFlags & EAccelerationStructureBuildFlags::PreferFastBuild) != EAccelerationStructureBuildFlags::None)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    }

    return Flags;
}

constexpr D3D12_RAYTRACING_INSTANCE_FLAGS ConvertRayTracingInstanceFlags(ERayTracingInstanceFlags InFlags)
{
    D3D12_RAYTRACING_INSTANCE_FLAGS Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    if ((InFlags & ERayTracingInstanceFlags::CullDisable) != ERayTracingInstanceFlags::None)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    }
    if ((InFlags & ERayTracingInstanceFlags::FrontCounterClockwise) != ERayTracingInstanceFlags::None)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    }
    if ((InFlags & ERayTracingInstanceFlags::ForceOpaque) != ERayTracingInstanceFlags::None)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
    }
    if ((InFlags & ERayTracingInstanceFlags::ForceNonOpaque) != ERayTracingInstanceFlags::None)
    {
        Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
    }

    return Flags;
}


constexpr uint32 GetFormatStride(DXGI_FORMAT Format)
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

constexpr DXGI_FORMAT D3D12CastShaderResourceFormat(DXGI_FORMAT Format)
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
        default:                                return Format;
    }
}

struct FD3D12HashableTextureView
{
    FD3D12HashableTextureView()
        : Hash(0)
    {
    }

    bool operator==(const FD3D12HashableTextureView& Other) const
    {
        return Hash == Other.Hash;
    }

    bool operator!=(const FD3D12HashableTextureView& Other) const
    {
        return Hash != Other.Hash;
    }

    friend uint64 GetHashForType(const FD3D12HashableTextureView& Value)
    {
        return Value.Hash;
    }

    union
    {
        struct
        {
            uint16  ArrayIndex;
            uint16  NumArraySlices;
            EFormat Format;
            uint8   MipLevel;
        };
        
        uint64 Hash;
    };
};

static_assert(sizeof(FD3D12HashableTextureView) == sizeof(uint64), "FD3D12HashableTextureView should be the same size as uint64");

struct FD3D12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{
    FD3D12_CPU_DESCRIPTOR_HANDLE() noexcept
        : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }
    {
    }

    explicit FD3D12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) noexcept
        : D3D12_CPU_DESCRIPTOR_HANDLE{ Other }
    {
    }

    explicit FD3D12_CPU_DESCRIPTOR_HANDLE(uint64 Handle) noexcept
        : D3D12_CPU_DESCRIPTOR_HANDLE{ Handle }
    {
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& Other, int64 OffsetScaledByIncrementSize) noexcept
        : D3D12_CPU_DESCRIPTOR_HANDLE{ static_cast<uint64>(static_cast<int64>(Other.ptr) + OffsetScaledByIncrementSize) }
    {
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& Other, int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) noexcept
        : D3D12_CPU_DESCRIPTOR_HANDLE{ static_cast<uint64>(static_cast<int64>(Other.ptr) + (static_cast<int64>(OffsetInDescriptors) * static_cast<int64>(DescriptorIncrementSize))) }
    {
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE Offset(int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) const noexcept
    {
        return FD3D12_CPU_DESCRIPTOR_HANDLE(*this, OffsetInDescriptors, DescriptorIncrementSize);
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& Offset(int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) noexcept
    {
        ptr = static_cast<uint64>(static_cast<int64>(ptr) + static_cast<int64>(OffsetInDescriptors) * static_cast<int64>(DescriptorIncrementSize));
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE Offset(int64 OffsetScaledByIncrementSize) const noexcept
    {
        return FD3D12_CPU_DESCRIPTOR_HANDLE(*this, OffsetScaledByIncrementSize);
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& Offset(int64 OffsetScaledByIncrementSize) noexcept
    {
        ptr = static_cast<uint64>(static_cast<int64>(ptr) + OffsetScaledByIncrementSize);
        return *this;
    }

    bool operator==(const D3D12_CPU_DESCRIPTOR_HANDLE& RHS) const noexcept
    {
        return ptr == RHS.ptr;
    }

    bool operator!=(const D3D12_CPU_DESCRIPTOR_HANDLE& RHS) const noexcept
    {
        return ptr != RHS.ptr;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator-=(int64 RHS) noexcept
    {
        ptr -= RHS;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator-=(const D3D12_CPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr -= RHS.ptr;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator+=(int64 RHS) noexcept
    {
        ptr += RHS;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator+=(const D3D12_CPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr += RHS.ptr;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr = RHS.ptr;
        return *this;
    }
};

struct FD3D12_GPU_DESCRIPTOR_HANDLE : public D3D12_GPU_DESCRIPTOR_HANDLE
{
    FD3D12_GPU_DESCRIPTOR_HANDLE() noexcept
        : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 }
    {
    }

    explicit FD3D12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) noexcept
        : D3D12_GPU_DESCRIPTOR_HANDLE{ Other }
    {
    }

    explicit FD3D12_GPU_DESCRIPTOR_HANDLE(uint64 Handle) noexcept
        : D3D12_GPU_DESCRIPTOR_HANDLE{ Handle }
    {
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& Other, int64 OffsetScaledByIncrementSize) noexcept
        : D3D12_GPU_DESCRIPTOR_HANDLE{ static_cast<uint64>(static_cast<int64>(Other.ptr) + OffsetScaledByIncrementSize) }
    {
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& Other, int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) noexcept
        : D3D12_GPU_DESCRIPTOR_HANDLE{ static_cast<uint64>(static_cast<int64>(Other.ptr) + (static_cast<int64>(OffsetInDescriptors) * static_cast<int64>(DescriptorIncrementSize))) }
    {
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& Offset(int64 OffsetScaledByIncrementSize) noexcept
    {
        ptr = static_cast<uint64>(static_cast<int64>(ptr) + OffsetScaledByIncrementSize);
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE Offset(int64 OffsetScaledByIncrementSize) const noexcept
    {
        return FD3D12_GPU_DESCRIPTOR_HANDLE(*this, OffsetScaledByIncrementSize);
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& Offset(int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) noexcept
    {
        ptr = static_cast<uint64>(static_cast<int64>(ptr) + static_cast<int64>(OffsetInDescriptors) * static_cast<int64>(DescriptorIncrementSize));
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE Offset(int32 OffsetInDescriptors, uint32 DescriptorIncrementSize) const noexcept
    {
        return FD3D12_GPU_DESCRIPTOR_HANDLE(*this, OffsetInDescriptors, DescriptorIncrementSize);
    }

    bool operator==(const D3D12_GPU_DESCRIPTOR_HANDLE& RHS) const noexcept
    {
        return (ptr == RHS.ptr);
    }

    bool operator!=(const D3D12_GPU_DESCRIPTOR_HANDLE& RHS) const noexcept
    {
        return (ptr != RHS.ptr);
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator-=(int64 RHS) noexcept
    {
        ptr -= RHS;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator-=(const D3D12_GPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr -= RHS.ptr;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator+=(int64 RHS) noexcept
    {
        ptr += RHS;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator+=(const D3D12_GPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr += RHS.ptr;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator=(const D3D12_GPU_DESCRIPTOR_HANDLE& RHS) noexcept
    {
        ptr = RHS.ptr;
        return *this;
    }
};

constexpr const CHAR* ToString(D3D12_RESOURCE_DIMENSION Dimension)
{
    switch(Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_BUFFER:    return "RESOURCE_DIMENSION_BUFFER";
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: return "RESOURCE_DIMENSION_TEXTURE1D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: return "RESOURCE_DIMENSION_TEXTURE2D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return "RESOURCE_DIMENSION_TEXTURE3D";
        default:                                 return "RESOURCE_DIMENSION_UNKNOWN";
    }
}

constexpr uint32 D3D12CalculateSubresource(uint32 MipSlice, uint32 ArraySlice, uint32 PlaneSlice, uint32 MipLevels, uint32 ArraySize) noexcept
{
    return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
}

constexpr uint32 D3D12CalculateArraySlices(ETextureDimension Dimension, uint32 NumArraySlices)
{
    if (IsTextureCube(Dimension))
    {
        return NumArraySlices * RHI_NUM_CUBE_FACES;
    }
    else
    {
        return NumArraySlices;
    }
}
