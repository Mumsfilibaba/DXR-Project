#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHICore.h"

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

    Count = 10,
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
static constexpr uint32 MSL_BINDLESS_HEAP_MARKER_SET = 31;

static constexpr uint32 MSL_BINDLESS_RESOURCE_BINDING = 0;
static constexpr uint32 MSL_BINDLESS_SAMPLER_BINDING  = 1;
static constexpr uint32 MSL_BINDLESS_COUNTER_BINDING  = 16;

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
    };

    static_assert(ARRAY_COUNT(BindingTypeStrings) == static_cast<int32>(EMSLBindingType::Count), "BindingTypeStrings is out of date");
    return BindingTypeStrings[static_cast<int32>(BindingType)];
}

struct FMSLShaderBinding
{
    /** @brief Namespace the slot was allocated from. */
    EMSLBindingType BindingType;

    /** @brief HLSL register index within its namespace, unused for ShaderConstants. */
    uint8 RegisterIndex;

    /** @brief Index into the MSL buffer, texture or sampler table, whichever the type selects. */
    uint8 SlotIndex;

    uint8 Padding0;
};

static_assert(sizeof(FMSLShaderBinding) == 4, "FMSLShaderBinding is serialized verbatim and must not carry padding");

struct FMSLShaderHeader
{
    /** @brief Value Magic must hold for a blob to be an MSL blob rather than raw source. */
    static constexpr uint32 ExpectedMagic = 0x4D534C42;

    /** @brief Layout revision, bumped whenever the header or the binding array changes shape. */
    static constexpr uint32 ExpectedVersion = 4;

    uint32 Magic;
    uint32 Version;
    uint32 NumBindings;
    uint32 SourceSize;
    uint16 ThreadGroupSizeX;
    uint16 ThreadGroupSizeY;
    uint16 ThreadGroupSizeZ;
    uint16 ShaderConstantsSize;

    /** @brief MSL buffer index of ResourceDescriptorHeap, or UINT8_MAX. */
    uint8 ResourceHeapSlot;

    /** @brief MSL buffer index of SamplerDescriptorHeap, or UINT8_MAX. */
    uint8 SamplerHeapSlot;

    uint16 Padding0;
};

static_assert(sizeof(FMSLShaderHeader) == 28, "FMSLShaderHeader is serialized verbatim and must not carry padding");

inline bool ParseMSLShaderByteCode(const TArray<uint8>& ByteCode, TArray<FMSLShaderBinding>& OutBindings, TArrayView<const uint8>& OutSource)
{
    OutBindings.Clear();

    if (ByteCode.Size() < static_cast<int32>(sizeof(FMSLShaderHeader)))
    {
        OutSource = TArrayView<const uint8>(ByteCode.Data(), ByteCode.Size());
        return true;
    }

    FMSLShaderHeader Header;
    Memory::Memcpy(&Header, ByteCode.Data(), sizeof(FMSLShaderHeader));

    if (Header.Magic != FMSLShaderHeader::ExpectedMagic)
    {
        OutSource = TArrayView<const uint8>(ByteCode.Data(), ByteCode.Size());
        return true;
    }

    if (Header.Version != FMSLShaderHeader::ExpectedVersion)
    {
        return false;
    }

    const uint64 BindingsSize = uint64(Header.NumBindings) * sizeof(FMSLShaderBinding);
    const uint64 ExpectedSize = sizeof(FMSLShaderHeader) + BindingsSize + Header.SourceSize;

    if (ExpectedSize > uint64(ByteCode.Size()))
    {
        return false;
    }

    OutBindings.Resize(static_cast<int32>(Header.NumBindings));
    if (Header.NumBindings > 0)
    {
        Memory::Memcpy(OutBindings.Data(), ByteCode.Data() + sizeof(FMSLShaderHeader), BindingsSize);
    }

    OutSource = TArrayView<const uint8>(ByteCode.Data() + sizeof(FMSLShaderHeader) + BindingsSize, static_cast<int32>(Header.SourceSize));
    return true;
}
