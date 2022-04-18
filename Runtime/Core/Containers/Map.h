#pragma once
#include <map>

template<typename KeyType, typename T, typename CompareType = std::less<KeyType>>
using TMap = std::map<KeyType, T, CompareType>;
