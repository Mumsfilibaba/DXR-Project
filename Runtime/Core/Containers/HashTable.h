#pragma once
#include <unordered_map>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TODO: Custom hash map implementation

template<typename KeyType, typename T, typename HashType = std::hash<KeyType>>
using THashTable = std::unordered_map<KeyType, T, HashType>;