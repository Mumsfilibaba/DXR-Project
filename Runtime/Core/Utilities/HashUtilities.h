#pragma once
#include "Core/CoreDefines.h"
#include "Core/Containers/String.h"
#include "Core/Templates/TypeHash.h"

template<typename T>
constexpr void HashCombine(uint64& OutHash, const T& Value)
{
    OutHash ^= TTypeHash<T>::Hash(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
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


namespace std
{
    template<>
    struct hash<FString>
    {
        size_t operator()(const FString& String) const noexcept
        {
            FStringHasher Hasher;
            return Hasher(String);
        }
    };

    template<>
    struct hash<FStringWide>
    {
        size_t operator()(const FStringWide& String) const noexcept
        {
            FStringHasherWide Hasher;
            return Hasher(String);
        }
    };
}