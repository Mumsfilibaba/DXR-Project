#pragma once
#include "Core/CoreDefines.h"

#include <utility>
#include <functional>

template<typename T>
using THash = std::hash<T>;

template<typename T, typename THashType = size_t>
CONSTEXPR void HashCombine(THashType& OutHash, const T& Value)
{
    THash<T> Hasher;
    OutHash ^= (THashType)Hasher(Value) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
}