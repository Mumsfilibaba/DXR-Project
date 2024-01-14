#pragma once
#include "Core/Utilities/HashUtilities.h"

// TODO: Custom map implementation
#if 1
#include <unordered_map>

template<typename KeyType, typename T, typename HashType = THash<KeyType>>
using TMap = std::unordered_map<KeyType, T, HashType>;

#else

#include "Pair.h"
#include "Array.h"

template<typename T>
struct THash;

template<typename ElementType, typename KeyType, typename HasherType = THash<KeyType>>
class TMap
{
public:
    ElementType* Find(const KeyType& Key)
    {
        return nullptr;
    }

    void Add(const KeyType& Key, )

    ElementType& operator[](const KeyType& Key)
    {
        if (ElementType* Element = Find(Key))
        {
            return *Element;
        }
        else
        {

        }
    }

private:
    TArray<ElementType> Elements;
    TArray<KeyType>     Keys;
};

#endif
