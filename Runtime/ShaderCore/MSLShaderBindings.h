#pragma once
#include "Core/Containers/Array.h"
#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCore/ShaderReflection.h"

enum class EMSLBindingType : uint8
{
    Unknown = 0,

    /** @brief ConstantBuffer (b#), bound to the MSL buffer table. */
    ConstantBuffer = 1,

    /** @brief StructuredBuffer or ByteAddressBuffer (t#), bound to the MSL buffer table. */
    ShaderResourceBuffer = 2,

    /** @brief Texture or texel Buffer (t#), bound to the MSL texture table. */
    ShaderResourceTexture = 3,

    /** @brief RWStructuredBuffer or RWByteAddressBuffer (u#), bound to the MSL buffer table. */
    UnorderedAccessBuffer = 4,

    /** @brief RWTexture or RWBuffer (u#), bound to the MSL texture table. */
    UnorderedAccessTexture = 5,

    /** @brief SamplerState (s#), bound to the MSL sampler table. */
    Sampler = 6,

    /** @brief Root constants, bound to the MSL buffer table. Carries no register index. */
    ShaderConstants = 7,

    /** @brief SM 6.6 ResourceDescriptorHeap, bound as a descriptor table buffer. */
    BindlessResourceHeap = 8,

    /** @brief SM 6.6 SamplerDescriptorHeap, bound as a descriptor table buffer. */
    BindlessSamplerHeap = 9,

    /** @brief RaytracingAccelerationStructure (t#), bound to the MSL buffer table. */
    AccelerationStructure = 10,

    Count = 11,
};

enum class EMSLBindingTable : uint8
{
    /** @brief `[[buffer(n)]]` table. */
    Buffer = 0,

    /** @brief `[[texture(n)]]` table. */
    Texture = 1,

    /** @brief `[[sampler(n)]]` table. */
    Sampler = 2,
};

/** @brief Conservative Metal buffer-table size (`maxBuffers` / `maxVertexBuffers`). */
static constexpr uint8 MSL_MAX_BUFFER_SLOTS = 31;

/** @brief Conservative Mac texture-table size. Apple silicon can be 128. */
static constexpr uint8 MSL_MAX_TEXTURE_SLOTS = 31;

/** @brief Sampler-table size, matching `MAX_SAMPLER_STATES`. */
static constexpr uint8 MSL_MAX_SAMPLER_SLOTS = 16;

/** @brief DXC `-fvk-bind-*-heap` marker set, shared with the Vulkan cook. */
static constexpr uint32 MSL_BINDLESS_HEAP_MARKER_SET = ShaderBindings::SpirvHeapMarkerSet;

static constexpr uint32 MSL_BINDLESS_RESOURCE_BINDING = ShaderBindings::SpirvHeapResourceBinding;
static constexpr uint32 MSL_BINDLESS_SAMPLER_BINDING  = ShaderBindings::SpirvHeapSamplerBinding;
static constexpr uint32 MSL_BINDLESS_COUNTER_BINDING  = ShaderBindings::SpirvHeapCounterBinding;

/** @brief Fixed MSL buffer index for the resource descriptor table. */
static constexpr uint8 MSL_BINDLESS_RESOURCE_HEAP_BUFFER_INDEX = 29;

/** @brief Fixed MSL buffer index for the sampler descriptor table. */
static constexpr uint8 MSL_BINDLESS_SAMPLER_HEAP_BUFFER_INDEX = 30;

/** @brief MSL buffer index of vertex stream 0. Later streams take the indices below it. */
static constexpr uint8 MSL_VERTEX_STREAM_BUFFER_INDEX = 28;

/** @brief Number of vertex streams the run ending at MSL_VERTEX_STREAM_BUFFER_INDEX holds. */
static constexpr uint8 MSL_MAX_VERTEX_STREAMS = 8;

inline uint8 GetMSLVertexStreamBufferIndex(uint32 InputSlot)
{
    return static_cast<uint8>(MSL_VERTEX_STREAM_BUFFER_INDEX - InputSlot);
}

inline EMSLBindingTable GetMSLBindingTable(EMSLBindingType BindingType)
{
    switch (BindingType)
    {
        case EMSLBindingType::ShaderResourceTexture:
        case EMSLBindingType::UnorderedAccessTexture:
            return EMSLBindingTable::Texture;

        case EMSLBindingType::Sampler:
            return EMSLBindingTable::Sampler;

        default:
            return EMSLBindingTable::Buffer;
    }
}

inline uint8 GetMSLMaxSlotCount(EMSLBindingType BindingType)
{
    switch (GetMSLBindingTable(BindingType))
    {
        case EMSLBindingTable::Texture:
            return MSL_MAX_TEXTURE_SLOTS;

        case EMSLBindingTable::Sampler:
            return MSL_MAX_SAMPLER_SLOTS;

        default:
            return MSL_MAX_BUFFER_SLOTS;
    }
}

inline const CHAR* ToString(EMSLBindingType BindingType)
{
    static constexpr const CHAR* const BindingTypeStrings[] =
    {
        "Unknown",
        "ConstantBuffer",
        "ShaderResourceBuffer",
        "ShaderResourceTexture",
        "UnorderedAccessBuffer",
        "UnorderedAccessTexture",
        "Sampler",
        "ShaderConstants",
        "BindlessResourceHeap",
        "BindlessSamplerHeap",
        "AccelerationStructure",
    };

    static_assert(ARRAY_COUNT(BindingTypeStrings) == static_cast<int32>(EMSLBindingType::Count), "BindingTypeStrings is out of date");
    return BindingTypeStrings[static_cast<int32>(BindingType)];
}

enum class EMSLTextureDimension : uint8
{
    Texture1D        = 0,
    Texture1DArray   = 1,
    Texture2D        = 2,
    Texture2DArray   = 3,
    TextureCube      = 4,
    TextureCubeArray = 5,
    Texture3D        = 6,
    Texture2DMS      = 7,
    TextureBuffer    = 8,
    Count            = 9,
};

enum class EMSLTextureComponent : uint8
{
    Float = 0,
    Int   = 1,
    Uint  = 2,
    Depth = 3,
    Count = 4,
};

/** @brief Number of distinct FMSLShaderBinding::NullTextureType values. */
static constexpr uint8 MSL_NUM_NULL_TEXTURE_TYPES = static_cast<uint8>(EMSLTextureDimension::Count) * static_cast<uint8>(EMSLTextureComponent::Count);

/**
 * @param Dimension Texture type the binding declares
 * @param Component Component type the binding declares
 * @return Index of the null texture matching both, in the range [0, MSL_NUM_NULL_TEXTURE_TYPES)
 */
constexpr uint8 MakeMSLNullTextureType(EMSLTextureDimension Dimension, EMSLTextureComponent Component)
{
    return static_cast<uint8>(static_cast<uint8>(Component) * static_cast<uint8>(EMSLTextureDimension::Count) + static_cast<uint8>(Dimension));
}

struct FMSLShaderBinding
{
    /** @brief Namespace the slot was allocated from. */
    EMSLBindingType BindingType;

    /** @brief HLSL register index within its namespace, unused for ShaderConstants. */
    uint8 RegisterIndex;

    /** @brief Index into the MSL buffer, texture or sampler table, whichever the type selects. */
    uint8 SlotIndex;

    /** @brief Null texture bound when the register is empty, from MakeMSLNullTextureType. Zero for every other binding type. */
    uint8 NullTextureType;
};

static_assert(sizeof(FMSLShaderBinding) == 4, "FMSLShaderBinding must not carry padding");

NODISCARD constexpr EMSLBindingType GetMSLBindingType(const FShaderResourceBinding& Binding)
{
    if (Binding.Space == EShaderBindingSpace::ShaderConstants)
    {
        return EMSLBindingType::ShaderConstants;
    }

    if (Binding.Space == EShaderBindingSpace::BindlessHeap)
    {
        return Binding.Type == EShaderResourceType::Sampler ? EMSLBindingType::BindlessSamplerHeap : EMSLBindingType::BindlessResourceHeap;
    }

    switch (Binding.Type)
    {
        case EShaderResourceType::ConstantBuffer:        return EMSLBindingType::ConstantBuffer;
        case EShaderResourceType::Sampler:               return EMSLBindingType::Sampler;
        case EShaderResourceType::Texture:
        case EShaderResourceType::TypedBuffer:           return EMSLBindingType::ShaderResourceTexture;
        case EShaderResourceType::StructuredBuffer:
        case EShaderResourceType::ByteAddressBuffer:     return EMSLBindingType::ShaderResourceBuffer;
        case EShaderResourceType::AccelerationStructure: return EMSLBindingType::AccelerationStructure;
        case EShaderResourceType::RWTexture:
        case EShaderResourceType::RWTypedBuffer:         return EMSLBindingType::UnorderedAccessTexture;
        case EShaderResourceType::RWStructuredBuffer:
        case EShaderResourceType::RWByteAddressBuffer:   return EMSLBindingType::UnorderedAccessBuffer;
        default:                                         return EMSLBindingType::Unknown;
    }
}
