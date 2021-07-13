#pragma once
#include <unordered_map>

// TODO: Custom hash map implementation
template<typename TKeyType, typename T, typename THashType = std::hash<TKeyType>>
using THashTable = std::unordered_map<TKeyType, T, THashType>;