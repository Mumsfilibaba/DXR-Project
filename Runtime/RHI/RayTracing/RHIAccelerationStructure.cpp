#include "RHI/RayTracing/RHIAccelerationStructure.h"

static constexpr uint64 SerializedBlockFormatOffset = 48;
static constexpr uint32 SerializedBlockFormatMarker = 0xFFFFFFFFu;
static constexpr uint32 OpacityMicromapBlockType    = 0;

static bool FindOpacityMicromapAddresses(const uint8* SerializedData, uint64 SerializedSizeInBytes, TArray<uint64>& OutAddressOffsets)
{
    OutAddressOffsets.Clear();
    if (!SerializedData || SerializedSizeInBytes < SerializedBlockFormatOffset + sizeof(uint64))
    {
        return false;
    }

    uint64 BlockFormat = 0;
    Memory::Memcpy(&BlockFormat, SerializedData + SerializedBlockFormatOffset, sizeof(uint64));
    if (static_cast<uint32>(BlockFormat >> 32) != SerializedBlockFormatMarker)
    {
        return true;
    }

    const uint32 NumBlocks = static_cast<uint32>(BlockFormat & 0xFFFFFFFFull);

    uint64 Offset = SerializedBlockFormatOffset + sizeof(uint64);
    for (uint32 BlockIndex = 0; BlockIndex < NumBlocks; ++BlockIndex)
    {
        if (Offset + 2 * sizeof(uint64) > SerializedSizeInBytes)
        {
            return false;
        }

        uint32 BlockType    = 0;
        uint64 NumAddresses = 0;
        Memory::Memcpy(&BlockType, SerializedData + Offset, sizeof(uint32));
        Memory::Memcpy(&NumAddresses, SerializedData + Offset + sizeof(uint64), sizeof(uint64));
        Offset += 2 * sizeof(uint64);

        if (NumAddresses > (SerializedSizeInBytes - Offset) / sizeof(uint64))
        {
            return false;
        }

        for (uint64 AddressIndex = 0; AddressIndex < NumAddresses && BlockType == OpacityMicromapBlockType; ++AddressIndex)
        {
            OutAddressOffsets.Add(Offset + AddressIndex * sizeof(uint64));
        }

        Offset += NumAddresses * sizeof(uint64);
    }

    return true;
}

bool FRHIGeometryAccelerationStructure::FindSerializedOpacityMicromaps(
    const void*                                   SerializedData,
    uint64                                        SerializedSizeInBytes,
    const TArrayView<FRHIOpacityMicromap* const>& LinkedMicromaps,
    TFunctionRef<uint64(FRHIOpacityMicromap*)>    GetAddress,
    TArray<FRHIOpacityMicromap*>&                 OutOpacityMicromaps)
{
    OutOpacityMicromaps.Clear();

    const uint8* SerializedBytes = static_cast<const uint8*>(SerializedData);

    TArray<uint64> AddressOffsets;
    if (!FindOpacityMicromapAddresses(SerializedBytes, SerializedSizeInBytes, AddressOffsets))
    {
        return false;
    }

    for (const uint64 AddressOffset : AddressOffsets)
    {
        uint64 Address = 0;
        Memory::Memcpy(&Address, SerializedBytes + AddressOffset, sizeof(uint64));

        FRHIOpacityMicromap* OpacityMicromap = nullptr;
        for (int32 Index = 0; Index < LinkedMicromaps.Size() && Address != 0 && !OpacityMicromap; ++Index)
        {
            if (GetAddress(LinkedMicromaps[Index]) == Address)
            {
                OpacityMicromap = LinkedMicromaps[Index];
            }
        }

        if (Address != 0 && !OpacityMicromap)
        {
            return false;
        }

        OutOpacityMicromaps.Add(OpacityMicromap);
    }

    return true;
}

bool FRHIGeometryAccelerationStructure::LinkSerializedOpacityMicromaps(
    const void*                                   SerializedData,
    uint64                                        SerializedSizeInBytes,
    const TArrayView<FRHIOpacityMicromap* const>& OpacityMicromaps,
    TFunctionRef<uint64(FRHIOpacityMicromap*)>    GetAddress,
    TArray<uint8>&                                OutSerializedData)
{
    const uint8* SerializedBytes = static_cast<const uint8*>(SerializedData);

    TArray<uint64> AddressOffsets;
    if (!FindOpacityMicromapAddresses(SerializedBytes, SerializedSizeInBytes, AddressOffsets) || AddressOffsets.Size() != OpacityMicromaps.Size())
    {
        return false;
    }

    OutSerializedData = TArray<uint8>(SerializedBytes, static_cast<int32>(SerializedSizeInBytes));
    for (int32 Index = 0; Index < AddressOffsets.Size(); ++Index)
    {
        const uint64 Address = OpacityMicromaps[Index] ? GetAddress(OpacityMicromaps[Index]) : 0;
        Memory::Memcpy(OutSerializedData.Data() + AddressOffsets[Index], &Address, sizeof(uint64));
    }

    return true;
}
