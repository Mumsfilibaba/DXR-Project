#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "ShaderCore/ShaderTypes.h"

enum class EShaderResourceType : uint8
{
    ConstantBuffer = 0,
    Sampler,

    // Shader resource views
    Texture,
    TypedBuffer,
    StructuredBuffer,
    ByteAddressBuffer,
    AccelerationStructure,

    // Unordered access views
    RWTexture,
    RWTypedBuffer,
    RWStructuredBuffer,
    RWByteAddressBuffer,

    Count
};

NODISCARD constexpr const CHAR* ToString(EShaderResourceType Type)
{
    switch (Type)
    {
        case EShaderResourceType::ConstantBuffer:        return "ConstantBuffer";
        case EShaderResourceType::Sampler:               return "Sampler";
        case EShaderResourceType::Texture:               return "Texture";
        case EShaderResourceType::TypedBuffer:           return "TypedBuffer";
        case EShaderResourceType::StructuredBuffer:      return "StructuredBuffer";
        case EShaderResourceType::ByteAddressBuffer:     return "ByteAddressBuffer";
        case EShaderResourceType::AccelerationStructure: return "AccelerationStructure";
        case EShaderResourceType::RWTexture:             return "RWTexture";
        case EShaderResourceType::RWTypedBuffer:         return "RWTypedBuffer";
        case EShaderResourceType::RWStructuredBuffer:    return "RWStructuredBuffer";
        case EShaderResourceType::RWByteAddressBuffer:   return "RWByteAddressBuffer";
        default:                                         return "Unknown";
    }
}

enum class EShaderResourceClass : uint8
{
    ConstantBuffer = 0,
    Sampler,
    SRV,
    UAV,
};

NODISCARD constexpr EShaderResourceClass GetShaderResourceClass(EShaderResourceType Type)
{
    switch (Type)
    {
        case EShaderResourceType::ConstantBuffer:      return EShaderResourceClass::ConstantBuffer;
        case EShaderResourceType::Sampler:             return EShaderResourceClass::Sampler;
        case EShaderResourceType::RWTexture:
        case EShaderResourceType::RWTypedBuffer:
        case EShaderResourceType::RWStructuredBuffer:
        case EShaderResourceType::RWByteAddressBuffer: return EShaderResourceClass::UAV;
        default:                                       return EShaderResourceClass::SRV;
    }
}

enum class EShaderResourceDimension : uint8
{
    Unknown = 0,
    Buffer,
    BufferEx,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    Count
};

enum class EShaderBindingSpace : uint8
{
    Global = 0,
    RayTracingLocal,

    /**
     * SPIR-V and MSL: ResourceDescriptorHeap / SamplerDescriptorHeap. In SPIR-V these are the marker bindings that VulkanRHI
     * redirects to its bindless set. MSL has one binding per heap, with Type Sampler for the sampler heap.
     */
    BindlessHeap,

    /**
     * DXBC and MSL, because both bind the constants at a slot the compiler picks: the Constants_CB cbuffer FFXCShaderTranslator
     * creates, or the MSL buffer the push constants became. DXIL and SPIR-V report them only through ShaderConstantsSize.
     */
    ShaderConstants,

    Count
};

enum class EShaderComponentType : uint8
{
    Unknown = 0,
    Float32,
    Int32,
    Uint32,
    Float16,
    Int16,
    Uint16,
    Count
};

/** HLSL semantics are case-insensitive, so the name is upper-cased before it is hashed (CRC32) */
NODISCARD SHADERCORE_API uint32 HashShaderSemantic(StringView SemanticName);

enum class EShaderFeatureFlags : uint32
{
    None                                        = 0,
    RequiresResourceDescriptorHeapIndexing      = FLAG(1),
    RequiresSamplerDescriptorHeapIndexing       = FLAG(2),
    RequiresEarlyDepthStencil                   = FLAG(3),
    RequiresStencilRef                          = FLAG(4),
    RequiresInnerCoverage                       = FLAG(5),
    RequiresROVs                                = FLAG(6),
    RequiresWaveOps                             = FLAG(7),
    RequiresInt64Ops                            = FLAG(8),
    RequiresNative16BitOps                      = FLAG(9),
    RequiresBarycentrics                        = FLAG(10),
    RequiresViewID                              = FLAG(11),
    RequiresShadingRate                         = FLAG(12),
    RequiresRaytracingTier1_1                   = FLAG(13),
    RequiresSamplerFeedback                     = FLAG(14),
    RequiresTiledResources                      = FLAG(15),
    RequiresTypedUAVLoadAdditionalFormats       = FLAG(16),
    RequiresVPAndRTArrayIndexFromAnyShader      = FLAG(17),
    RequiresAtomicInt64OnTypedResource          = FLAG(18),
    RequiresAtomicInt64OnGroupShared            = FLAG(19),
    RequiresAtomicInt64OnDescriptorHeapResource = FLAG(20),
    RequiresWaveMMA                             = FLAG(21),
    RequiresDerivativesInMeshAndAmpShaders      = FLAG(22),
};

ENUM_CLASS_OPERATORS(EShaderFeatureFlags);

enum class EShaderReflectionFlags : uint8
{
    None = 0,

    /** DXIL only: the container has an RTS0 part */
    HasEmbeddedRootSignature = FLAG(0),
};

ENUM_CLASS_OPERATORS(EShaderReflectionFlags);

struct FShaderReflectionInfo
{
    NODISCARD bool HasFlag(EShaderReflectionFlags InFlag) const
    {
        return IsEnumFlagSet(Flags, InFlag);
    }

    EShaderFeatureFlags    RequiredFeatures    = EShaderFeatureFlags::None;
    EShaderReflectionFlags Flags               = EShaderReflectionFlags::None;
    uint8                  NumVertexInputs     = 0;
    uint8                  NumBindings         = 0;

    /** Declared size in bytes, 0 when the shader has no constants. Each RHI rounds it the way it did before. */
    uint8                  ShaderConstantsSize = 0;
};

static_assert(sizeof(FShaderReflectionInfo) == 8, "FShaderReflectionInfo is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FShaderVertexInput
{
    /** HashShaderSemantic of the semantic name, so release containers still carry no names */
    uint32               SemanticHash  = 0;
    uint8                SemanticIndex = 0;

    /** SPIR-V Location decoration, or the D3D input register */
    uint8                Location      = 0;
    EShaderComponentType ComponentType = EShaderComponentType::Unknown;
    uint8                NumComponents = 0;
};

static_assert(sizeof(FShaderVertexInput) == 8, "FShaderVertexInput is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FShaderResourceBinding
{
    EShaderResourceType      Type      = EShaderResourceType::ConstantBuffer;
    EShaderResourceDimension Dimension = EShaderResourceDimension::Unknown;
    EShaderBindingSpace      Space     = EShaderBindingSpace::Global;
    uint8                    Register  = 0;
    uint8                    Count     = 1;
};

static_assert(sizeof(FShaderResourceBinding) == 5 && alignof(FShaderResourceBinding) == 1, "FShaderResourceBinding is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FSpirvBindingOffsets
{
    uint16 SetWordOffset     = 0;
    uint16 BindingWordOffset = 0;
};

static_assert(sizeof(FSpirvBindingOffsets) == 4, "FSpirvBindingOffsets is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FMSLShaderInfo
{
    uint16 ThreadGroupSize[3] = { 0, 0, 0 };
    uint16 Reserved           = 0;
};

static_assert(sizeof(FMSLShaderInfo) == 8, "FMSLShaderInfo is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FMSLBindingSlot
{
    uint8 Slot            = 0;

    /** MakeMSLNullTextureType for texture bindings, 0 otherwise */
    uint8 NullTextureType = 0;
};

static_assert(sizeof(FMSLBindingSlot) == 2, "FMSLBindingSlot is serialized as-is, bump FShaderCodeHeader::CurrentVersion when it changes");

struct FShaderReflection
{
    FShaderReflectionInfo          Info;

    /** Only set where an RHI needs it: ray-tracing export names, SPIR-V and MSL entry points */
    String                         EntryPoint;

    /** Vertex shaders only, sorted by Location */
    TArray<FShaderVertexInput>     VertexInputs;

    /** SPIR-V: in the order VulkanRHI assigns descriptor slots: sampled images, samplers, storage images, uniform buffers, storage buffers, acceleration structures */
    TArray<FShaderResourceBinding> Bindings;

    /** Empty, or one per binding for SPIR-V */
    TArray<FSpirvBindingOffsets>   SpirvOffsets;

    /** MSL only */
    FMSLShaderInfo                 MSLInfo;

    /** Empty, or one per binding for MSL */
    TArray<FMSLBindingSlot>        MSLSlots;

    /** Empty, or one per binding when compiled with debug info */
    TArray<String>                 BindingNames;
};
