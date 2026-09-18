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

    Count = 8,
};

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
    static constexpr uint32 ExpectedVersion = 1;

    uint32 Magic;
    uint32 Version;
    uint32 NumBindings;
    uint32 SourceSize;
};

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
