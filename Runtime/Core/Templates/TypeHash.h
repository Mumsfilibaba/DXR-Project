#pragma once
#include "Core/Templates/Utility.h"

template<typename T, typename = void>
struct THash;

template<>
struct THash<bool>
{
    static constexpr uint64 GetHash(bool bValue)
    {
        return static_cast<uint64>(bValue);
    }
};

template<>
struct THash<int8>
{
    static constexpr uint64 GetHash(int8 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<uint8>
{
    static constexpr uint64 GetHash(uint8 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<int16>
{
    static constexpr uint64 GetHash(int16 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<uint16>
{
    static constexpr uint64 GetHash(uint16 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<int32>
{
    static constexpr uint64 GetHash(int32 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<uint32>
{
    static constexpr uint64 GetHash(uint32 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<int64>
{
    static constexpr uint64 GetHash(int64 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<uint64>
{
    static constexpr uint64 GetHash(uint64 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct THash<float>
{
    static constexpr uint64 GetHash(float Value)
    {
        return static_cast<uint64>(BitCast<uint32>(Value));
    }
};

template<>
struct THash<double>
{
    static constexpr uint64 GetHash(double Value)
    {
        return BitCast<uint64>(Value);
    }
};

template<typename PointerType>
struct THash<PointerType*>
{
    static uint64 GetHash(PointerType* Value)
    {
        return reinterpret_cast<uint64>(Value);
    }
};

template<typename EnumType>
struct THash<EnumType, typename TEnableIf<TIsEnum<EnumType>::Value>::Type>
{
    static constexpr uint64 GetHash(EnumType Value)
    {
        return static_cast<uint64>(UnderlyingTypeValue<EnumType>(Value));
    }
};

template<typename T>
constexpr void HashCombine(uint64& OutHash, const T& Value)
{
    OutHash ^= THash<T>::GetHash(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
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
