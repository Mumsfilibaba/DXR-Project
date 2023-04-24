#pragma once
#include "Core/CoreDefines.h"
#include "Core/Containers/String.h"

#include <utility>
#include <functional>

template<typename T>
using THash = std::hash<T>;

template<typename T, typename THashType = size_t>
constexpr void HashCombine(THashType& OutHash, const T& Value)
{
    THash<T> Hasher;
    OutHash ^= (THashType)Hasher(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
}


namespace std
{
    template<>
    struct hash<FString>
    {
        std::size_t operator()(const FString& String) const noexcept
        {
            FStringHasher Hasher;
            return Hasher(String);
        }
    };

    template<>
    struct hash<FStringWide>
    {
        std::size_t operator()(const FStringWide& String) const noexcept
        {
            FStringHasherWide Hasher;
            return Hasher(String);
        }
    };
}