#pragma once
#include "Core/Misc/Debug.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/ComPtr.h"
#include "RHI/RHIIndirect.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"
#include "D3D12RHI/D3D12Constants.h"

#include <dxgi.h>

// -------------------------------------------
// Windows SDK feature availability
// -------------------------------------------

// DXGI 1.6 (IDXGIFactory6, DXGI_GPU_PREFERENCE) - requires Windows 10 1803 (RS4) SDK
#if (NTDDI_WIN10_RS4 && (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS4))
    #define DXGI_1_6 (1)
#endif

// D3D_FEATURE_LEVEL_12_2, ID3D12Debug5 - requires Windows 10 21H1 (FE) SDK
#if (NTDDI_WIN10_FE && (WDK_NTDDI_VERSION >= NTDDI_WIN10_FE))
    #define WIN10_BUILD_20348 (1)
#endif

static_assert(sizeof(FRHIDrawIndirectParameters) == sizeof(D3D12_DRAW_ARGUMENTS));
static_assert(sizeof(FRHIDrawIndexedIndirectParameters) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
static_assert(sizeof(FRHIDispatchIndirectParameters) == sizeof(D3D12_DISPATCH_ARGUMENTS));

#if D3D12_USE_ID3D12COMMANDLIST_4
static_assert(sizeof(FRHIShaderRecordAddressRange) == sizeof(D3D12_GPU_VIRTUAL_ADDRESS_RANGE));
static_assert(OFFSETOF(FRHIShaderRecordAddressRange, StartAddress) == OFFSETOF(D3D12_GPU_VIRTUAL_ADDRESS_RANGE, StartAddress));
static_assert(OFFSETOF(FRHIShaderRecordAddressRange, SizeInBytes) == OFFSETOF(D3D12_GPU_VIRTUAL_ADDRESS_RANGE, SizeInBytes));
static_assert(sizeof(FRHIShaderTableAddressRange) == sizeof(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE));
static_assert(OFFSETOF(FRHIShaderTableAddressRange, StartAddress) == OFFSETOF(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE, StartAddress));
static_assert(OFFSETOF(FRHIShaderTableAddressRange, SizeInBytes) == OFFSETOF(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE, SizeInBytes));
static_assert(OFFSETOF(FRHIShaderTableAddressRange, StrideInBytes) == OFFSETOF(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE, StrideInBytes));
static_assert(sizeof(FRHIDispatchRaysIndirectParameters) == sizeof(D3D12_DISPATCH_RAYS_DESC));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, RayGenerationShaderRecord) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, RayGenerationShaderRecord));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, MissShaderTable) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, MissShaderTable));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, HitGroupTable) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, HitGroupTable));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, CallableShaderTable) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, CallableShaderTable));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Width) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, Width));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Height) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, Height));
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Depth) == OFFSETOF(D3D12_DISPATCH_RAYS_DESC, Depth));
#endif

#if D3D12_USE_ID3D12COMMANDLIST_6
static_assert(sizeof(FRHIDispatchMeshIndirectParameters) == sizeof(D3D12_DISPATCH_MESH_ARGUMENTS));
#endif

#if D3D12_ENABLE_LOGGING
    #define D3D12_ERROR_CRITICAL(...) \
        do \
        { \
            LOG_ERROR_CRITICAL("[D3D12RHI] " __VA_ARGS__); \
        } while (false)

    #define D3D12_ERROR(...) \
        do \
        { \
            LOG_ERROR("[D3D12RHI] " __VA_ARGS__); \
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
            LOG_WARNING("[D3D12RHI] " __VA_ARGS__); \
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
            LOG_INFO("[D3D12RHI] " __VA_ARGS__); \
        } while (false)
#else
    #define D3D12_ERROR_CRITICAL(...) \
        do { } while(false)

    #define D3D12_ERROR_COND(bCondition, ...) \
        do { UNREFERENCED_VARIABLE(bCondition); } while(false)

    #define D3D12_ERROR(...) \
        do { } while(false)

    #define D3D12_WARNING_COND(bCondition, ...) \
        do { UNREFERENCED_VARIABLE(bCondition); } while(false)

    #define D3D12_WARNING(...) \
        do { } while(false)

    #define D3D12_INFO(...) \
        do { } while(false)
#endif

#if D3D12_ENABLE_RESOURCE_STATE_LOGGING
    #define D3D12_LOG_TRANSITION_MISMATCH(InResource, InContext, InD3D12BeforeState, InD3D12AfterState, InCurrentState) \
        do \
        { \
            String TransitionDebugName; \
            (InResource)->GetDebugName(TransitionDebugName); \
            D3D12_ERROR( \
                "TransitionBarrier mismatch on '%s' [%s]: " \
                "BeforeState=%s, AfterState=%s, CurrentState=%s", \
                *TransitionDebugName, \
                (InContext), \
                ToString(InD3D12BeforeState), \
                ToString(InD3D12AfterState), \
                ToString(InCurrentState)); \
        } while (false)
#else
    #define D3D12_LOG_TRANSITION_MISMATCH(InResource, InContext, InD3D12BeforeState, InD3D12AfterState, InCurrentState) ((void)0)
#endif

NODISCARD inline D3D12_HEAP_PROPERTIES GetUploadHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties = { };
    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = 1;
    HeapProperties.CreationNodeMask     = 1;

    return HeapProperties;
}

NODISCARD inline D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()
{
    D3D12_HEAP_PROPERTIES HeapProperties = { };
    HeapProperties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = 1;
    HeapProperties.CreationNodeMask     = 1;

    return HeapProperties;
}

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

enum class ED3D12CommandQueueType : uint8
{
    Direct  = 0,
    Compute = 1,
    Copy    = 2,

    Count
};

NODISCARD constexpr const CHAR* ToString(ED3D12CommandQueueType QueueType)
{
    switch (QueueType)
    {
        case ED3D12CommandQueueType::Direct:  return "Default";
        case ED3D12CommandQueueType::Compute: return "Compute";
        case ED3D12CommandQueueType::Copy:    return "Copy";
    }

    return "CommandQueueType::Unknown";
}

NODISCARD constexpr D3D12_COMMAND_LIST_TYPE ToCommandListType(ED3D12CommandQueueType QueueType)
{
    switch (QueueType)
    {
        case ED3D12CommandQueueType::Direct:  return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case ED3D12CommandQueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case ED3D12CommandQueueType::Copy:    return D3D12_COMMAND_LIST_TYPE_COPY;
    }

    return D3D12_COMMAND_LIST_TYPE(-1);
}

NODISCARD constexpr const CHAR* ToString(D3D12_COMMAND_LIST_TYPE CommandListType)
{
    switch (CommandListType)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:        return "Direct";
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:        return "Bundle";
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:       return "Compute";
        case D3D12_COMMAND_LIST_TYPE_COPY:          return "Copy";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:  return "VideoDecode";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS: return "VideoProcess";
        case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:  return "VideoEncode";

        default: return "Unknown";
    }
}

enum class ED3D12GlobalDescriptorHeapType : uint8
{
    /** CBV/SRV/UAV global online heap (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) */
    Resource = 0,

    /** Sampler global online heap (D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) */
    Sampler = 1,

    Count
};

NODISCARD constexpr const CHAR* ToString(ED3D12GlobalDescriptorHeapType HeapType)
{
    switch (HeapType)
    {
        case ED3D12GlobalDescriptorHeapType::Resource: return "Resource";
        case ED3D12GlobalDescriptorHeapType::Sampler:  return "Sampler";
    }

    return "Unknown";
}

enum class ED3D12ResourceStateMode : uint8
{
    /** Resource permanently occupies one state. Transitions targeting it are dropped */
    SingleState,

    /** Backend tracks state per subresource and infers the before-state */
    MultipleStates,

    /** Backend never tracks. The caller owns every transition and supplies it verbatim */
    ManualState,
};

NODISCARD constexpr ED3D12ResourceStateMode ConvertResourceStateMode(ERHIResourceStateTrackingMode TrackingMode)
{
    switch (TrackingMode)
    {
        case ERHIResourceStateTrackingMode::Static:  return ED3D12ResourceStateMode::SingleState;
        case ERHIResourceStateTrackingMode::Tracked: return ED3D12ResourceStateMode::MultipleStates;
        case ERHIResourceStateTrackingMode::Manual:  return ED3D12ResourceStateMode::ManualState;
        default:                                     return ED3D12ResourceStateMode::MultipleStates;
    }
}

NODISCARD constexpr ERHIResourceStateTrackingMode ConvertResourceStateMode(ED3D12ResourceStateMode StateMode)
{
    switch (StateMode)
    {
        case ED3D12ResourceStateMode::SingleState:    return ERHIResourceStateTrackingMode::Static;
        case ED3D12ResourceStateMode::MultipleStates: return ERHIResourceStateTrackingMode::Tracked;
        case ED3D12ResourceStateMode::ManualState:    return ERHIResourceStateTrackingMode::Manual;
        default:                                      return ERHIResourceStateTrackingMode::Tracked;
    }
}

NODISCARD constexpr uint32 GetBufferAlignment(EBufferFlags BufferFlags)
{
    if (IsEnumFlagSet(BufferFlags, EBufferFlags::ConstantBuffer))
    {
        return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }
    else
    {
        return 16;
    }
}

NODISCARD constexpr D3D12_RESOURCE_FLAGS ConvertBufferFlags(EBufferFlags Flags)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (IsEnumFlagSet(Flags, EBufferFlags::UnorderedAccessBuffer))
    {
        Result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return Result;
}

NODISCARD constexpr D3D12_RESOURCE_FLAGS ConvertTextureFlags(ETextureUsageFlags Flag)
{
    D3D12_RESOURCE_FLAGS Result = D3D12_RESOURCE_FLAG_NONE;
    if (IsEnumFlagSet(Flag, ETextureUsageFlags::UnorderedAccessTexture))
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

    const bool bAllowSRV = IsEnumFlagSet(Flag, ETextureUsageFlags::ShaderResourceTexture);
    if (bAllowDSV && !bAllowSRV)
    {
        Result |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return Result;
}

NODISCARD constexpr D3D12_RESOURCE_DIMENSION ConvertTextureDimension(ETextureDimension TextureDimension)
{
    switch (TextureDimension)
    {
        case ETextureDimension::Texture1D:
        case ETextureDimension::Texture1DArray:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

        case ETextureDimension::Texture2D:
        case ETextureDimension::Texture2DArray:
        case ETextureDimension::TextureCube:
        case ETextureDimension::TextureCubeArray:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        case ETextureDimension::Texture3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

        default: return D3D12_RESOURCE_DIMENSION_UNKNOWN;
    }
}

NODISCARD constexpr DXGI_FORMAT ConvertFormat(EFormat Format)
{
    switch (Format)
    {
        case EFormat::R32G32B32A32_Typeless:              return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case EFormat::R32G32B32A32_Float:                 return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case EFormat::R32G32B32A32_Uint:                  return DXGI_FORMAT_R32G32B32A32_UINT;
        case EFormat::R32G32B32A32_Sint:                  return DXGI_FORMAT_R32G32B32A32_SINT;
		case EFormat::R32G32B32_Typeless:                 return DXGI_FORMAT_R32G32B32_TYPELESS;
		case EFormat::R32G32B32_Float:                    return DXGI_FORMAT_R32G32B32_FLOAT;
		case EFormat::R32G32B32_Uint:                     return DXGI_FORMAT_R32G32B32_UINT;
		case EFormat::R32G32B32_Sint:                     return DXGI_FORMAT_R32G32B32_SINT;
        case EFormat::R16G16B16A16_Typeless:              return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case EFormat::R16G16B16A16_Float:                 return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case EFormat::R16G16B16A16_Unorm:                 return DXGI_FORMAT_R16G16B16A16_UNORM;
        case EFormat::R16G16B16A16_Uint:                  return DXGI_FORMAT_R16G16B16A16_UINT;
        case EFormat::R16G16B16A16_Snorm:                 return DXGI_FORMAT_R16G16B16A16_SNORM;
        case EFormat::R16G16B16A16_Sint:                  return DXGI_FORMAT_R16G16B16A16_SINT;
        case EFormat::R32G32_Typeless:                    return DXGI_FORMAT_R32G32_TYPELESS;
        case EFormat::R32G32_Float:                       return DXGI_FORMAT_R32G32_FLOAT;
        case EFormat::R32G32_Uint:                        return DXGI_FORMAT_R32G32_UINT;
        case EFormat::R32G32_Sint:                        return DXGI_FORMAT_R32G32_SINT;
        case EFormat::R10G10B10A2_Typeless:               return DXGI_FORMAT_R10G10B10A2_TYPELESS;
        case EFormat::R10G10B10A2_Unorm:                  return DXGI_FORMAT_R10G10B10A2_UNORM;
        case EFormat::R10G10B10A2_Uint:                   return DXGI_FORMAT_R10G10B10A2_UINT;
        case EFormat::R11G11B10_Float:                    return DXGI_FORMAT_R11G11B10_FLOAT;
        case EFormat::R8G8B8A8_Typeless:                  return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case EFormat::R8G8B8A8_Unorm:                     return DXGI_FORMAT_R8G8B8A8_UNORM;
        case EFormat::R8G8B8A8_Unorm_SRGB:                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case EFormat::R8G8B8A8_Uint:                      return DXGI_FORMAT_R8G8B8A8_UINT;
        case EFormat::R8G8B8A8_Snorm:                     return DXGI_FORMAT_R8G8B8A8_SNORM;
        case EFormat::R8G8B8A8_Sint:                      return DXGI_FORMAT_R8G8B8A8_SINT;
        case EFormat::B8G8R8A8_Typeless:                  return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case EFormat::B8G8R8A8_Unorm:                     return DXGI_FORMAT_B8G8R8A8_UNORM;
        case EFormat::B8G8R8A8_Unorm_SRGB:                return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case EFormat::R16G16_Typeless:                    return DXGI_FORMAT_R16G16_TYPELESS;
		case EFormat::R16G16_Float:                       return DXGI_FORMAT_R16G16_FLOAT;
		case EFormat::R16G16_Unorm:                       return DXGI_FORMAT_R16G16_UNORM;
		case EFormat::R16G16_Uint:                        return DXGI_FORMAT_R16G16_UINT;
		case EFormat::R16G16_Snorm:                       return DXGI_FORMAT_R16G16_SNORM;
		case EFormat::R16G16_Sint:                        return DXGI_FORMAT_R16G16_SINT;
		case EFormat::R32_Typeless:                       return DXGI_FORMAT_R32_TYPELESS;
		case EFormat::D32_Float:                          return DXGI_FORMAT_D32_FLOAT;
		case EFormat::R32_Float:                          return DXGI_FORMAT_R32_FLOAT;
		case EFormat::R32_Uint:                           return DXGI_FORMAT_R32_UINT;
		case EFormat::R32_Sint:                           return DXGI_FORMAT_R32_SINT;
        case EFormat::R24G8_Typeless:                     return DXGI_FORMAT_R24G8_TYPELESS;
        case EFormat::D24_Unorm_S8_Uint:                  return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case EFormat::R24_Unorm_X8_Typeless:              return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case EFormat::X24_Typeless_G8_Uint:               return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case EFormat::R8G8_Typeless:                      return DXGI_FORMAT_R8G8_TYPELESS;
		case EFormat::R8G8_Unorm:                         return DXGI_FORMAT_R8G8_UNORM;
		case EFormat::R8G8_Uint:                          return DXGI_FORMAT_R8G8_UINT;
		case EFormat::R8G8_Snorm:                         return DXGI_FORMAT_R8G8_SNORM;
		case EFormat::R8G8_Sint:                          return DXGI_FORMAT_R8G8_SINT;
        case EFormat::R16_Typeless:                       return DXGI_FORMAT_R16_TYPELESS;
        case EFormat::R16_Float:                          return DXGI_FORMAT_R16_FLOAT;
        case EFormat::D16_Unorm:                          return DXGI_FORMAT_D16_UNORM;
        case EFormat::R16_Unorm:                          return DXGI_FORMAT_R16_UNORM;
        case EFormat::R16_Uint:                           return DXGI_FORMAT_R16_UINT;
        case EFormat::R16_Snorm:                          return DXGI_FORMAT_R16_SNORM;
        case EFormat::R16_Sint:                           return DXGI_FORMAT_R16_SINT;
        case EFormat::R8_Typeless:                        return DXGI_FORMAT_R8_TYPELESS;
        case EFormat::R8_Unorm:                           return DXGI_FORMAT_R8_UNORM;
        case EFormat::R8_Uint:                            return DXGI_FORMAT_R8_UINT;
        case EFormat::R8_Snorm:                           return DXGI_FORMAT_R8_SNORM;
        case EFormat::R8_Sint:                            return DXGI_FORMAT_R8_SINT;
        case EFormat::BC1_Typeless:                       return DXGI_FORMAT_BC1_TYPELESS;
        case EFormat::BC1_UNorm:                          return DXGI_FORMAT_BC1_UNORM;
        case EFormat::BC1_UNorm_SRGB:                     return DXGI_FORMAT_BC1_UNORM_SRGB;
        case EFormat::BC2_Typeless:                       return DXGI_FORMAT_BC2_TYPELESS;
        case EFormat::BC2_UNorm:                          return DXGI_FORMAT_BC2_UNORM;
        case EFormat::BC2_UNorm_SRGB:                     return DXGI_FORMAT_BC2_UNORM_SRGB;
        case EFormat::BC3_Typeless:                       return DXGI_FORMAT_BC3_TYPELESS;
        case EFormat::BC3_UNorm:                          return DXGI_FORMAT_BC3_UNORM;
        case EFormat::BC3_UNorm_SRGB:                     return DXGI_FORMAT_BC3_UNORM_SRGB;
        case EFormat::BC4_Typeless:                       return DXGI_FORMAT_BC4_TYPELESS;
        case EFormat::BC4_UNorm:                          return DXGI_FORMAT_BC4_UNORM;
        case EFormat::BC4_SNorm:                          return DXGI_FORMAT_BC4_SNORM;
        case EFormat::BC5_Typeless:                       return DXGI_FORMAT_BC5_TYPELESS;
        case EFormat::BC5_UNorm:                          return DXGI_FORMAT_BC5_UNORM;
        case EFormat::BC5_SNorm:                          return DXGI_FORMAT_BC5_SNORM;
        case EFormat::BC6H_Typeless:                      return DXGI_FORMAT_BC6H_TYPELESS;
        case EFormat::BC6H_UF16:                          return DXGI_FORMAT_BC6H_UF16;
        case EFormat::BC6H_SF16:                          return DXGI_FORMAT_BC6H_SF16;
        case EFormat::BC7_Typeless:                       return DXGI_FORMAT_BC7_TYPELESS;
        case EFormat::BC7_UNorm:                          return DXGI_FORMAT_BC7_UNORM;
        case EFormat::BC7_UNorm_SRGB:                     return DXGI_FORMAT_BC7_UNORM_SRGB;
        case EFormat::SamplerFeedbackMinMipOpaque:        return DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
        case EFormat::SamplerFeedbackMipRegionUsedOpaque: return DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE;
        default:                                          return DXGI_FORMAT_UNKNOWN;
    }
}

constexpr const CHAR* ToString(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:               return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:                  return "DXGI_FORMAT_R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_UINT:                   return "DXGI_FORMAT_R32G32B32A32_UINT";
    case DXGI_FORMAT_R32G32B32A32_SINT:                   return "DXGI_FORMAT_R32G32B32A32_SINT";
    case DXGI_FORMAT_R32G32B32_TYPELESS:                  return "DXGI_FORMAT_R32G32B32_TYPELESS";
    case DXGI_FORMAT_R32G32B32_FLOAT:                     return "DXGI_FORMAT_R32G32B32_FLOAT";
    case DXGI_FORMAT_R32G32B32_UINT:                      return "DXGI_FORMAT_R32G32B32_UINT";
    case DXGI_FORMAT_R32G32B32_SINT:                      return "DXGI_FORMAT_R32G32B32_SINT";
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:               return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:                  return "DXGI_FORMAT_R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R16G16B16A16_UNORM:                  return "DXGI_FORMAT_R16G16B16A16_UNORM";
    case DXGI_FORMAT_R16G16B16A16_UINT:                   return "DXGI_FORMAT_R16G16B16A16_UINT";
    case DXGI_FORMAT_R16G16B16A16_SNORM:                  return "DXGI_FORMAT_R16G16B16A16_SNORM";
    case DXGI_FORMAT_R16G16B16A16_SINT:                   return "DXGI_FORMAT_R16G16B16A16_SINT";
    case DXGI_FORMAT_R32G32_TYPELESS:                     return "DXGI_FORMAT_R32G32_TYPELESS";
    case DXGI_FORMAT_R32G32_FLOAT:                        return "DXGI_FORMAT_R32G32_FLOAT";
    case DXGI_FORMAT_R32G32_UINT:                         return "DXGI_FORMAT_R32G32_UINT";
    case DXGI_FORMAT_R32G32_SINT:                         return "DXGI_FORMAT_R32G32_SINT";
    case DXGI_FORMAT_R32G8X24_TYPELESS:                   return "DXGI_FORMAT_R32G8X24_TYPELESS";
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:                return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:            return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:             return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:                return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
    case DXGI_FORMAT_R10G10B10A2_UNORM:                   return "DXGI_FORMAT_R10G10B10A2_UNORM";
    case DXGI_FORMAT_R10G10B10A2_UINT:                    return "DXGI_FORMAT_R10G10B10A2_UINT";
    case DXGI_FORMAT_R11G11B10_FLOAT:                     return "DXGI_FORMAT_R11G11B10_FLOAT";
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:                   return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
    case DXGI_FORMAT_R8G8B8A8_UNORM:                      return "DXGI_FORMAT_R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:                 return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
    case DXGI_FORMAT_R8G8B8A8_UINT:                       return "DXGI_FORMAT_R8G8B8A8_UINT";
    case DXGI_FORMAT_R8G8B8A8_SNORM:                      return "DXGI_FORMAT_R8G8B8A8_SNORM";
    case DXGI_FORMAT_R8G8B8A8_SINT:                       return "DXGI_FORMAT_R8G8B8A8_SINT";
    case DXGI_FORMAT_R16G16_TYPELESS:                     return "DXGI_FORMAT_R16G16_TYPELESS";
    case DXGI_FORMAT_R16G16_FLOAT:                        return "DXGI_FORMAT_R16G16_FLOAT";
    case DXGI_FORMAT_R16G16_UNORM:                        return "DXGI_FORMAT_R16G16_UNORM";
    case DXGI_FORMAT_R16G16_UINT:                         return "DXGI_FORMAT_R16G16_UINT";
    case DXGI_FORMAT_R16G16_SNORM:                        return "DXGI_FORMAT_R16G16_SNORM";
    case DXGI_FORMAT_R16G16_SINT:                         return "DXGI_FORMAT_R16G16_SINT";
    case DXGI_FORMAT_R32_TYPELESS:                        return "DXGI_FORMAT_R32_TYPELESS";
    case DXGI_FORMAT_D32_FLOAT:                           return "DXGI_FORMAT_D32_FLOAT";
    case DXGI_FORMAT_R32_FLOAT:                           return "DXGI_FORMAT_R32_FLOAT";
    case DXGI_FORMAT_R32_UINT:                            return "DXGI_FORMAT_R32_UINT";
    case DXGI_FORMAT_R32_SINT:                            return "DXGI_FORMAT_R32_SINT";
    case DXGI_FORMAT_R24G8_TYPELESS:                      return "DXGI_FORMAT_R24G8_TYPELESS";
    case DXGI_FORMAT_D24_UNORM_S8_UINT:                   return "DXGI_FORMAT_D24_UNORM_S8_UINT";
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:               return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:                return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
    case DXGI_FORMAT_R8G8_TYPELESS:                       return "DXGI_FORMAT_R8G8_TYPELESS";
    case DXGI_FORMAT_R8G8_UNORM:                          return "DXGI_FORMAT_R8G8_UNORM";
    case DXGI_FORMAT_R8G8_UINT:                           return "DXGI_FORMAT_R8G8_UINT";
    case DXGI_FORMAT_R8G8_SNORM:                          return "DXGI_FORMAT_R8G8_SNORM";
    case DXGI_FORMAT_R8G8_SINT:                           return "DXGI_FORMAT_R8G8_SINT";
    case DXGI_FORMAT_R16_TYPELESS:                        return "DXGI_FORMAT_R16_TYPELESS";
    case DXGI_FORMAT_R16_FLOAT:                           return "DXGI_FORMAT_R16_FLOAT";
    case DXGI_FORMAT_D16_UNORM:                           return "DXGI_FORMAT_D16_UNORM";
    case DXGI_FORMAT_R16_UNORM:                           return "DXGI_FORMAT_R16_UNORM";
    case DXGI_FORMAT_R16_UINT:                            return "DXGI_FORMAT_R16_UINT";
    case DXGI_FORMAT_R16_SNORM:                           return "DXGI_FORMAT_R16_SNORM";
    case DXGI_FORMAT_R16_SINT:                            return "DXGI_FORMAT_R16_SINT";
    case DXGI_FORMAT_R8_TYPELESS:                         return "DXGI_FORMAT_R8_TYPELESS";
    case DXGI_FORMAT_R8_UNORM:                            return "DXGI_FORMAT_R8_UNORM";
    case DXGI_FORMAT_R8_UINT:                             return "DXGI_FORMAT_R8_UINT";
    case DXGI_FORMAT_R8_SNORM:                            return "DXGI_FORMAT_R8_SNORM";
    case DXGI_FORMAT_R8_SINT:                             return "DXGI_FORMAT_R8_SINT";
    case DXGI_FORMAT_A8_UNORM:                            return "DXGI_FORMAT_A8_UNORM";
    case DXGI_FORMAT_R1_UNORM:                            return "DXGI_FORMAT_R1_UNORM";
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:                  return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
    case DXGI_FORMAT_R8G8_B8G8_UNORM:                     return "DXGI_FORMAT_R8G8_B8G8_UNORM";
    case DXGI_FORMAT_G8R8_G8B8_UNORM:                     return "DXGI_FORMAT_G8R8_G8B8_UNORM";
    case DXGI_FORMAT_BC1_TYPELESS:                        return "DXGI_FORMAT_BC1_TYPELESS";
    case DXGI_FORMAT_BC1_UNORM:                           return "DXGI_FORMAT_BC1_UNORM";
    case DXGI_FORMAT_BC1_UNORM_SRGB:                      return "DXGI_FORMAT_BC1_UNORM_SRGB";
    case DXGI_FORMAT_BC2_TYPELESS:                        return "DXGI_FORMAT_BC2_TYPELESS";
    case DXGI_FORMAT_BC2_UNORM:                           return "DXGI_FORMAT_BC2_UNORM";
    case DXGI_FORMAT_BC2_UNORM_SRGB:                      return "DXGI_FORMAT_BC2_UNORM_SRGB";
    case DXGI_FORMAT_BC3_TYPELESS:                        return "DXGI_FORMAT_BC3_TYPELESS";
    case DXGI_FORMAT_BC3_UNORM:                           return "DXGI_FORMAT_BC3_UNORM";
    case DXGI_FORMAT_BC3_UNORM_SRGB:                      return "DXGI_FORMAT_BC3_UNORM_SRGB";
    case DXGI_FORMAT_BC4_TYPELESS:                        return "DXGI_FORMAT_BC4_TYPELESS";
    case DXGI_FORMAT_BC4_UNORM:                           return "DXGI_FORMAT_BC4_UNORM";
    case DXGI_FORMAT_BC4_SNORM:                           return "DXGI_FORMAT_BC4_SNORM";
    case DXGI_FORMAT_BC5_TYPELESS:                        return "DXGI_FORMAT_BC5_TYPELESS";
    case DXGI_FORMAT_BC5_UNORM:                           return "DXGI_FORMAT_BC5_UNORM";
    case DXGI_FORMAT_BC5_SNORM:                           return "DXGI_FORMAT_BC5_SNORM";
    case DXGI_FORMAT_B5G6R5_UNORM:                        return "DXGI_FORMAT_B5G6R5_UNORM";
    case DXGI_FORMAT_B5G5R5A1_UNORM:                      return "DXGI_FORMAT_B5G5R5A1_UNORM";
    case DXGI_FORMAT_B8G8R8A8_UNORM:                      return "DXGI_FORMAT_B8G8R8A8_UNORM";
    case DXGI_FORMAT_B8G8R8X8_UNORM:                      return "DXGI_FORMAT_B8G8R8X8_UNORM";
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:          return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:                   return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:                 return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:                   return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:                 return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
    case DXGI_FORMAT_BC6H_TYPELESS:                       return "DXGI_FORMAT_BC6H_TYPELESS";
    case DXGI_FORMAT_BC6H_UF16:                           return "DXGI_FORMAT_BC6H_UF16";
    case DXGI_FORMAT_BC6H_SF16:                           return "DXGI_FORMAT_BC6H_SF16";
    case DXGI_FORMAT_BC7_TYPELESS:                        return "DXGI_FORMAT_BC7_TYPELESS";
    case DXGI_FORMAT_BC7_UNORM:                           return "DXGI_FORMAT_BC7_UNORM";
    case DXGI_FORMAT_BC7_UNORM_SRGB:                      return "DXGI_FORMAT_BC7_UNORM_SRGB";
    case DXGI_FORMAT_AYUV:                                return "DXGI_FORMAT_AYUV";
    case DXGI_FORMAT_Y410:                                return "DXGI_FORMAT_Y410";
    case DXGI_FORMAT_Y416:                                return "DXGI_FORMAT_Y416";
    case DXGI_FORMAT_NV12:                                return "DXGI_FORMAT_NV12";
    case DXGI_FORMAT_P010:                                return "DXGI_FORMAT_P010";
    case DXGI_FORMAT_P016:                                return "DXGI_FORMAT_P016";
    case DXGI_FORMAT_420_OPAQUE:                          return "DXGI_FORMAT_420_OPAQUE";
    case DXGI_FORMAT_YUY2:                                return "DXGI_FORMAT_YUY2";
    case DXGI_FORMAT_Y210:                                return "DXGI_FORMAT_Y210";
    case DXGI_FORMAT_Y216:                                return "DXGI_FORMAT_Y216";
    case DXGI_FORMAT_NV11:                                return "DXGI_FORMAT_NV11";
    case DXGI_FORMAT_AI44:                                return "DXGI_FORMAT_AI44";
    case DXGI_FORMAT_IA44:                                return "DXGI_FORMAT_IA44";
    case DXGI_FORMAT_P8:                                  return "DXGI_FORMAT_P8";
    case DXGI_FORMAT_A8P8:                                return "DXGI_FORMAT_A8P8";
    case DXGI_FORMAT_B4G4R4A4_UNORM:                      return "DXGI_FORMAT_B4G4R4A4_UNORM";
    case DXGI_FORMAT_P208:                                return "DXGI_FORMAT_P208";
    case DXGI_FORMAT_V208:                                return "DXGI_FORMAT_V208";
    case DXGI_FORMAT_V408:                                return "DXGI_FORMAT_V408";
    case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:     return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
    case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";

    case DXGI_FORMAT_UNKNOWN:
    default:
        return "DXGI_FORMAT_UNKNOWN";
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

NODISCARD constexpr DXGI_USAGE ConvertSwapChainUsage(ESwapChainUsageFlags Usage)
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

constexpr const CHAR* ToString(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
    switch (ColorSpace)
    {
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:           return "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709";
        case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:           return "DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709";
        case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:         return "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709";
        case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:        return "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020";
        case DXGI_COLOR_SPACE_RESERVED:                         return "DXGI_COLOR_SPACE_RESERVED";
        case DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601:    return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601:       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601";
        case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601:         return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709:       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709";
        case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709:         return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020:      return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020";
        case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020:        return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020";
        case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:        return "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:    return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020";
        case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:      return "DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020:   return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020: return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020";
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:          return "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:  return "DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020";
        case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:    return "DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020";
        case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709:         return "DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709";
        case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020:        return "DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709:       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020:      return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020";
        case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020:   return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020";
        case DXGI_COLOR_SPACE_CUSTOM:                           return "DXGI_COLOR_SPACE_CUSTOM";

        default:
            return "Unknown DXGI_COLOR_SPACE_TYPE";
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

NODISCARD constexpr D3D12_INPUT_CLASSIFICATION ConvertVertexInputClass(EVertexInputClass InputClassification)
{
    switch (InputClassification)
    {
        case EVertexInputClass::Instance: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        case EVertexInputClass::Vertex:   return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }

    return D3D12_INPUT_CLASSIFICATION(-1);
}

NODISCARD constexpr D3D12_COMPARISON_FUNC ConvertComparisonFunc(EComparisonFunc ComparisonFunc)
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

NODISCARD constexpr D3D12_STENCIL_OP ConvertStencilOp(EStencilOp StencilOp)
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

NODISCARD inline D3D12_DEPTH_STENCILOP_DESC ConvertStencilState(const FRHIDepthStencilStateDesc::FStencilState& StencilState)
{
    return
    {
        ConvertStencilOp(StencilState.StencilFailOp),
        ConvertStencilOp(StencilState.StencilDepthFailOp),
        ConvertStencilOp(StencilState.StencilDepthPassOp),
        ConvertComparisonFunc(StencilState.StencilFunc)
    };
}

NODISCARD constexpr D3D12_CULL_MODE ConvertCullMode(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
        case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
        default:               return D3D12_CULL_MODE_NONE;
    }
}

NODISCARD constexpr D3D12_FILL_MODE ConvertFillMode(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
        case EFillMode::WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    }

    return D3D12_FILL_MODE();
}

NODISCARD constexpr D3D12_BLEND_OP ConvertBlendOp(EBlendOp BlendOp)
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

NODISCARD constexpr D3D12_BLEND ConvertBlend(EBlendType  Blend)
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

NODISCARD constexpr D3D12_LOGIC_OP ConvertLogicOp(ELogicOp LogicOp)
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

NODISCARD constexpr uint8 ConvertColorWriteFlags(EColorWriteFlags ColorWriteFlags)
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

NODISCARD constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(EPrimitiveTopology PrimitiveTopology)
{
    switch (PrimitiveTopology)
    {
        case EPrimitiveTopology::LineList:
        case EPrimitiveTopology::LineStrip:
        case EPrimitiveTopology::LineListAdjacency:
        case EPrimitiveTopology::LineStripAdjacency:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        case EPrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        case EPrimitiveTopology::TriangleList:
        case EPrimitiveTopology::TriangleStrip:
        case EPrimitiveTopology::TriangleListAdjacency:
        case EPrimitiveTopology::TriangleStripAdjacency:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        default:
            return IsPatchTopology(PrimitiveTopology) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

NODISCARD constexpr D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology PrimitiveTopology)
{
    // Both patch ranges are contiguous and one-based, so the whole range maps by a single offset
    static_assert(D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST - D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST == (RHI_MAX_PATCH_CONTROL_POINTS - 1),
        "D3D_PRIMITIVE_TOPOLOGY_N_CONTROL_POINT_PATCHLIST enumerators must be contiguous");

    if (IsPatchTopology(PrimitiveTopology))
    {
        return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (GetNumPatchControlPoints(PrimitiveTopology) - 1));
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

NODISCARD constexpr D3D12_RESOURCE_STATES ConvertResourceState(ERHIResourceState ResourceState)
{
    if (ResourceState == ERHIResourceState::Common)
    {
        return D3D12_RESOURCE_STATE_COMMON;
    }

    D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON;
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ConstantBuffer))
    {
        State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndexBuffer))
    {
        State |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::VertexBuffer))
    {
        State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::RenderTarget))
    {
        State |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::UnorderedAccess))
    {
        State |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthWrite))
    {
        State |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthRead))
    {
        State |= D3D12_RESOURCE_STATE_DEPTH_READ;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::NonPixelShaderResource))
    {
        State |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::PixelShaderResource))
    {
        State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopyDest))
    {
        State |= D3D12_RESOURCE_STATE_COPY_DEST;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopySource))
    {
        State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveDest))
    {
        State |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveSource))
    {
        State |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ShadingRateSource))
    {
        State |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::Present))
    {
        State |= D3D12_RESOURCE_STATE_PRESENT;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::GenericRead))
    {
        State |= D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::StreamOutput))
    {
        State |= D3D12_RESOURCE_STATE_STREAM_OUT;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndirectArgument))
    {
        State |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }

    return State;
}

NODISCARD constexpr D3D12_RESOURCE_BARRIER_FLAGS ConvertBarrierFlags(ERHIBarrierFlags Flags)
{
    D3D12_RESOURCE_BARRIER_FLAGS Result = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    if ((Flags & ERHIBarrierFlags::BeginOnly) != ERHIBarrierFlags::None)
    {
        Result |= D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
    }

    if ((Flags & ERHIBarrierFlags::EndOnly) != ERHIBarrierFlags::None)
    {
        Result |= D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
    }

    return Result;
}

NODISCARD inline D3D12_RESOURCE_STATES DetermineDefaultBufferState(EBufferFlags Flags)
{
    constexpr EBufferFlags TrackedMask =
        EBufferFlags::UnorderedAccessBuffer |
        EBufferFlags::StreamOutputBuffer |
        EBufferFlags::CopyDest |
        EBufferFlags::AccelerationStructure;

    if ((Flags & TrackedMask) != EBufferFlags::None)
    {
        return D3D12_RESOURCE_STATES(0);
    }

    D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATES(0);
    if (IsEnumFlagSet(Flags, EBufferFlags::IndexBuffer))
    {
        State |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::VertexBuffer) || IsEnumFlagSet(Flags, EBufferFlags::ConstantBuffer))
    {
        State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::ShaderResourceBuffer))
    {
        State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::CopySource))
    {
        State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }

    if (IsEnumFlagSet(Flags, EBufferFlags::IndirectArguments))
    {
        State |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }

    return State;
}

NODISCARD inline D3D12_RESOURCE_STATES DetermineDefaultTextureState(ETextureUsageFlags Flags)
{
    constexpr ETextureUsageFlags WriteMask = 
        ETextureUsageFlags::RenderTarget | 
        ETextureUsageFlags::DepthStencil | 
        ETextureUsageFlags::UnorderedAccessTexture | 
        ETextureUsageFlags::Presentable |
        ETextureUsageFlags::CopyDest;

    if ((Flags & WriteMask) != ETextureUsageFlags::None)
    {
        return D3D12_RESOURCE_STATES(0);
    }

    D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATES(0);
    if (IsEnumFlagSet(Flags, ETextureUsageFlags::ShaderResourceTexture))
    {
        State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (IsEnumFlagSet(Flags, ETextureUsageFlags::ShadingRateTexture))
    {
        State |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    }

    if (IsEnumFlagSet(Flags, ETextureUsageFlags::CopySource))
    {
        State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }

    return State;
}

NODISCARD constexpr D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerMode(ESamplerMode SamplerMode)
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

NODISCARD constexpr D3D12_FILTER ConvertSamplerFilter(ESamplerFilter SamplerFilter)
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
        case ESamplerFilter::Comparison_Anisotropic:                  return D3D12_FILTER_COMPARISON_ANISOTROPIC;
    }

    return D3D12_FILTER();
}

NODISCARD constexpr D3D12_SHADING_RATE ConvertShadingRate(EShadingRate ShadingRate)
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

NODISCARD constexpr EShaderModel ConvertShaderModel(D3D_SHADER_MODEL ShaderModel)
{
    switch (ShaderModel)
    {
        case D3D_SHADER_MODEL_6_0: return EShaderModel::SM_6_0;
        case D3D_SHADER_MODEL_6_1: return EShaderModel::SM_6_1;
        case D3D_SHADER_MODEL_6_2: return EShaderModel::SM_6_2;
        case D3D_SHADER_MODEL_6_3: return EShaderModel::SM_6_3;
        case D3D_SHADER_MODEL_6_4: return EShaderModel::SM_6_4;
        case D3D_SHADER_MODEL_6_5: return EShaderModel::SM_6_5;
        case D3D_SHADER_MODEL_6_6: return EShaderModel::SM_6_6;
        case D3D_SHADER_MODEL_6_7: return EShaderModel::SM_6_7;
        case D3D_SHADER_MODEL_6_8: return EShaderModel::SM_6_8;
        case D3D_SHADER_MODEL_6_9: return EShaderModel::SM_6_9;
        default:                   return EShaderModel::Unknown;
    }
}

NODISCARD constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertAccelerationStructureBuildFlags(EAccelerationStructureBuildFlags InFlags)
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

    if ((InFlags & EAccelerationStructureBuildFlags::AllowCompaction) != EAccelerationStructureBuildFlags::None)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    }

    if ((InFlags & EAccelerationStructureBuildFlags::MinimizeMemory) != EAccelerationStructureBuildFlags::None)
    {
        Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
    }

    return Flags;
}

NODISCARD constexpr D3D12_RAYTRACING_INSTANCE_FLAGS ConvertRayTracingInstanceFlags(ERayTracingInstanceFlags InFlags)
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

NODISCARD constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE ConvertAccelerationStructureCopyMode(EAccelerationStructureCopyMode CopyMode)
{
    switch (CopyMode)
    {
        case EAccelerationStructureCopyMode::Clone:                    return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
        case EAccelerationStructureCopyMode::Compact:                  return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT;
        case EAccelerationStructureCopyMode::Serialize:                return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE;
        case EAccelerationStructureCopyMode::Deserialize:              return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE;
        case EAccelerationStructureCopyMode::ToolsVisualizationDecode: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_VISUALIZATION_DECODE_FOR_TOOLS;
        default:                                                       return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
    }
}

NODISCARD constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE ConvertAccelerationStructurePostBuildInfoType(EAccelerationStructurePostBuildInfoType InfoType)
{
    switch (InfoType)
    {
        case EAccelerationStructurePostBuildInfoType::CompactedSize:      return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
        case EAccelerationStructurePostBuildInfoType::CurrentSize:        return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
        case EAccelerationStructurePostBuildInfoType::Serialization:      return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION;
        case EAccelerationStructurePostBuildInfoType::ToolsVisualization: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TOOLS_VISUALIZATION;
        default:                                                          return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    }
}

#if D3D12_ENABLE_OPACITY_MICROMAPS
NODISCARD constexpr D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT ConvertOpacityMicromapFormat(EOpacityMicromapFormat Format)
{
    return (Format == EOpacityMicromapFormat::OC1_4State)
        ? D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_4_STATE
        : D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE;
}
#endif

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

NODISCARD constexpr DXGI_FORMAT D3D12CastShaderResourceFormat(DXGI_FORMAT Format)
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

NODISCARD constexpr DXGI_FORMAT D3D12CastUnorderedAccessFormat(DXGI_FORMAT Format)
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

NODISCARD constexpr DXGI_FORMAT D3D12CastRenderTargetFormat(DXGI_FORMAT Format)
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

NODISCARD constexpr DXGI_FORMAT D3D12CastDepthStencilFormat(DXGI_FORMAT Format)
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

NODISCARD inline uint64 GetQueryResultStride(D3D12_QUERY_HEAP_TYPE HeapType)
{
    switch (HeapType)
    {
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:            return sizeof(uint64);
        case D3D12_QUERY_HEAP_TYPE_OCCLUSION:            return sizeof(uint64);
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:  return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
    #if D3D12_SUPPORT_PIPELINE_STATISTICS1
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1: return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS1);
    #endif
        default:                                         return sizeof(uint64);
    }
}

NODISCARD inline D3D12_QUERY_TYPE GetResolveQueryType(D3D12_QUERY_HEAP_TYPE HeapType)
{
    switch (HeapType)
    {
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:            return D3D12_QUERY_TYPE_TIMESTAMP;
        case D3D12_QUERY_HEAP_TYPE_OCCLUSION:            return D3D12_QUERY_TYPE_OCCLUSION;
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:  return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
    #if D3D12_SUPPORT_PIPELINE_STATISTICS1
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS1;
    #endif
        default:                                         return D3D12_QUERY_TYPE_TIMESTAMP;
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

template<>
struct THash<FD3D12HashableTextureView>
{
    NODISCARD static uint64 GetHash(const FD3D12HashableTextureView& Value)
    {
        return Value.Hash;
    }
};

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

    bool operator==(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) const noexcept
    {
        return ptr == Other.ptr;
    }

    bool operator!=(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) const noexcept
    {
        return ptr != Other.ptr;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator-=(int64 Other) noexcept
    {
        ptr -= Other;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator-=(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr -= Other.ptr;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator+=(int64 Other) noexcept
    {
        ptr += Other;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator+=(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr += Other.ptr;
        return *this;
    }

    FD3D12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr = Other.ptr;
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

    bool operator==(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) const noexcept
    {
        return ptr == Other.ptr;
    }

    bool operator!=(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) const noexcept
    {
        return ptr != Other.ptr;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator-=(int64 Other) noexcept
    {
        ptr -= Other;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator-=(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr -= Other.ptr;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator+=(int64 Other) noexcept
    {
        ptr += Other;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator+=(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr += Other.ptr;
        return *this;
    }

    FD3D12_GPU_DESCRIPTOR_HANDLE& operator=(const D3D12_GPU_DESCRIPTOR_HANDLE& Other) noexcept
    {
        ptr = Other.ptr;
        return *this;
    }
};

NODISCARD constexpr const CHAR* ToString(D3D12_RESOURCE_DIMENSION Dimension)
{
    switch(Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_BUFFER:    return "RESOURCE_DIMENSION_BUFFER";
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: return "RESOURCE_DIMENSION_TEXTURE1D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: return "RESOURCE_DIMENSION_TEXTURE2D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return "RESOURCE_DIMENSION_TEXTURE3D";

        default: return "RESOURCE_DIMENSION_UNKNOWN";
    }
}

NODISCARD constexpr const CHAR* ToString(D3D12_RESOURCE_STATES ResourceState)
{
	switch (ResourceState)
	{
	case D3D12_RESOURCE_STATE_COMMON:                     return "D3D12_RESOURCE_STATE_COMMON_or_PRESENT";
    case D3D12_RESOURCE_STATE_COPY_DEST:                  return "D3D12_RESOURCE_STATE_COPY_DEST";
    case D3D12_RESOURCE_STATE_COPY_SOURCE:                return "D3D12_RESOURCE_STATE_COPY_SOURCE";
    case D3D12_RESOURCE_STATE_DEPTH_READ:                 return "D3D12_RESOURCE_STATE_DEPTH_READ";
    case D3D12_RESOURCE_STATE_DEPTH_WRITE:                return "D3D12_RESOURCE_STATE_DEPTH_WRITE";
    case D3D12_RESOURCE_STATE_INDEX_BUFFER:               return "D3D12_RESOURCE_STATE_INDEX_BUFFER";
    case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: return "D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER";
    case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:  return "D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE";
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:      return "D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE";
    case D3D12_RESOURCE_STATE_RENDER_TARGET:              return "D3D12_RESOURCE_STATE_RENDER_TARGET";
    case D3D12_RESOURCE_STATE_RESOLVE_DEST:               return "D3D12_RESOURCE_STATE_RESOLVE_DEST";
    case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:             return "D3D12_RESOURCE_STATE_RESOLVE_SOURCE";
    case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:        return "D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE";
    case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:           return "D3D12_RESOURCE_STATE_UNORDERED_ACCESS";
	case D3D12_RESOURCE_STATE_GENERIC_READ:               return "D3D12_RESOURCE_STATE_GENERIC_READ";
    
    default: return "D3D12_RESOURCE_STATE_UNKNOWN";
	}
}

NODISCARD constexpr uint32 D3D12CalculateSubresource(uint32 MipSlice, uint32 ArraySlice, uint32 PlaneSlice, uint32 MipLevels, uint32 ArraySize) noexcept
{
    return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
}

NODISCARD constexpr uint32 D3D12CalculateSubresourceCount(uint32 MipLevels, uint32 ArraySize, uint32 PlaneCount) noexcept
{
    return MipLevels * ArraySize * PlaneCount;
}
