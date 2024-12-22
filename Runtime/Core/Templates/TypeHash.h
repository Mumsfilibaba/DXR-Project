#pragma once
#include "Core/Templates/Utility.h"

constexpr uint64 GetHashForType(bool bValue)
{
    return static_cast<uint64>(bValue);
}

constexpr uint64 GetHashForType(int8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(float Value)
{
    return static_cast<uint64>(BitCast<uint32>(Value));
}

constexpr uint64 GetHashForType(double Value)
{
    return BitCast<uint64>(Value);
}

template<typename PointerType>
constexpr TEnableIf<TIsPointer<PointerType>::Value, uint64>::Type GetHashForType(PointerType* Value) 
{
    return reinterpret_cast<uint64>(Value);
}

template<typename EnumType>
constexpr TEnableIf<TIsEnum<EnumType>::Value, uint64>::Type GetHashForType(EnumType Value) 
{
    return static_cast<uint64>(UnderlyingTypeValue<EnumType>(Value));
}

template<typename T>
constexpr void HashCombine(uint64& OutHash, const T& Value)
{
    OutHash ^= GetHashForType(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
}

template<typename T, const uint64 NumEntries>
constexpr uint64 HashIntegers(const T* Entries)
{
    uint64 Sum = 0;
    for (int32 Index = 0; Index < NumEntries; Index++)
    {
        Sum += Entries[Index];
    }

    uint64 Result = 0x9e3779b9 + (Sum << 6) + (Sum >> 2);;
    for (uint32 Index = 0; Index < NumEntries; Index++)
    {
        HashCombine(Result, Entries[Index]);
    }

    return Result;
}