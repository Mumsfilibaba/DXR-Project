#pragma once
#include <utility>
#include <functional>

template<typename T, typename THashType = size_t>
inline void HashCombine( THashType& OutHash, const T& Value )
{
    std::hash<T> Hasher;
    OutHash ^= (THashType)Hasher( Value ) + 0x9e3779b9 + (OutHash << 6) + (OutHash >> 2);
}