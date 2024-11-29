#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/TypeHash.h"

// TODO: Custom map implementation
#include <unordered_map>

template<typename InKeyType, typename InValueType>
class TMap
{
    template<typename MapType, typename KeyType, typename ValueType>
    friend class TMapIterator;

public:
    typedef InKeyType   KeyType;
    typedef InValueType ValueType;

private:
    struct FHasher
    {
        size_t operator()(const KeyType& Value) const
        {
            return static_cast<size_t>(GetHashForType(Value));
        }
    };
    
public:
    typedef std::unordered_map<KeyType, ValueType, FHasher>          BaseMapType;
    typedef TMapIterator<TMap, KeyType, ValueType>                   IteratorType;
    typedef TMapIterator<const TMap, const KeyType, const ValueType> ConstIteratorType;

    typedef int32 SIZETYPE;

public:

    /** @brief Default constructor */
    TMap() = default;

    /** @brief Copy constructor */
    TMap(const TMap& Other)
        : BaseMap(Other.BaseMap)
    {
    }
    
    /** @brief Move constructor */
    TMap(TMap&& Other)
        : BaseMap(Move(Other.BaseMap))
    {
    }

    void Append(const TMap& Other)
    {
        BaseMap.insert(Other.BaseMap.begin(), Other.BaseMap.end());
    }

    ValueType& Add(const KeyType& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = ValueType();
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.insert(std::make_pair(Key, ValueType()));
            return InsertedElement.first->second;
        }
    }

    ValueType& Add(KeyType&& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = ValueType();
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Forward<KeyType>(Key), ValueType());
            return InsertedElement.first->second;
        }
    }

    ValueType& Add(const KeyType& Key, const ValueType& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Value;
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.insert(std::make_pair(Key, Value));
            return InsertedElement.first->second;
        }
    }

    ValueType& Add(KeyType&& Key, const ValueType& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Value;
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Forward<KeyType>(Key), Value);
            return InsertedElement.first->second;
        }
    }
    
    ValueType& Add(const KeyType& Key, ValueType&& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Forward<ValueType>(Value);
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Key, Forward<ValueType>(Value));
            return InsertedElement.first->second;
        }
    }
    
    ValueType& Add(KeyType&& Key, ValueType&& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Forward<ValueType>(Value);
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Forward<KeyType>(Key), Forward<ValueType>(Value));
            return InsertedElement.first->second;
        }
    }

    ValueType& Emplace(KeyType&& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = ValueType();
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Forward<KeyType>(Key), ValueType());
            return InsertedElement.first->second;
        }
    }

    ValueType& Emplace(KeyType&& Key, ValueType&& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Forward<ValueType>(Value);
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.emplace(Forward<KeyType>(Key), Forward<ValueType>(Value));
            return InsertedElement.first->second;
        }
    }

    NODISCARD ValueType* Find(const KeyType& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            return AddressOf(Element->second);
        }
        else
        {
            return nullptr;
        }
    }

    NODISCARD const ValueType* Find(const KeyType& Key) const
    {
        typename BaseMapType::const_iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            return AddressOf(Element->second);
        }
        else
        {
            return nullptr;
        }
    }

    NODISCARD ValueType& FindOrAdd(const KeyType& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.insert(std::make_pair(Key, ValueType()));
            return InsertedElement.first->second;
        }
    }
    
    NODISCARD ValueType& FindOrAdd(const KeyType& Key, const ValueType& Value)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            Element->second = Value;
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.insert(std::make_pair(Key, Value));
            return InsertedElement.first->second;
        }
    }
    
    void Remove(const KeyType& InElement)
    {
        BaseMap.erase(InElement);
    }

    NODISCARD bool Contains(const KeyType& Key) const
    {
        typename BaseMapType::const_iterator Element = BaseMap.find(Key);
        return Element != BaseMap.end();
    }

    NODISCARD SIZETYPE Count(const KeyType& Key)
    {
        return static_cast<SIZETYPE>(BaseMap.count(Key));
    }
    
    void Reserve(SIZETYPE InCapacity)
    {
        BaseMap.reserve(InCapacity);
    }
    
    void Clear()
    {
        BaseMap.clear();
    }

    NODISCARD bool IsEmpty() const
    {
        return BaseMap.empty();
    }

    NODISCARD SIZETYPE Size() const
    {
        return static_cast<SIZETYPE>(BaseMap.size());
    }
    
    NODISCARD SIZETYPE Capacity() const
    {
        return static_cast<SIZETYPE>(BaseMap.max_size());
    }

    NODISCARD TArray<KeyType> GetKeys() const
    {
        TArray<KeyType> Keys;
        Keys.Reserve(static_cast<int32>(BaseMap.size()));

        for (const auto& Pair : BaseMap)
        {
            Keys.Emplace(Pair.first);
        }

        return Keys;
    }

    NODISCARD TArray<ValueType> GetValues() const
    {
        TArray<ValueType> Values;
        Values.Reserve(static_cast<int32>(BaseMap.size()));

        for (const auto& Pair : BaseMap)
        {
            Values.Emplace(Pair.second);
        }

        return Values;
    }

public:

    // Iterators
    NODISCARD IteratorType CreateIterator()
    {
        return TMapIterator<TMap, KeyType, ValueType>(*this, BaseMap.begin());
    }

    NODISCARD ConstIteratorType CreateIterator() const
    {
        return TMapIterator<TMap, const KeyType, const ValueType>(*this, BaseMap.begin());
    }

    NODISCARD ConstIteratorType CreateConstIterator() const
    {
        return TMapIterator<TMap, const KeyType, const ValueType>(*this, BaseMap.begin());
    }

public:

    // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       { return TMapIterator<TMap, KeyType, ValueType>(*this, BaseMap.begin()); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return TMapIterator<const TMap, const KeyType, const ValueType>(*this, BaseMap.begin()); }
    
    NODISCARD FORCEINLINE IteratorType      end()       { return TMapIterator<TMap, KeyType, ValueType>(*this, BaseMap.end()); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return TMapIterator<const TMap, const KeyType, const ValueType>(*this, BaseMap.end()); }

public:
    NODISCARD ValueType& operator[](const KeyType& Key)
    {
        typename BaseMapType::iterator Element = BaseMap.find(Key);
        if (Element != BaseMap.end())
        {
            return Element->second;
        }
        else
        {
            auto InsertedElement = BaseMap.insert(std::make_pair(Key, ValueType()));
            return InsertedElement.first->second;
        }
    }
    
    NODISCARD const ValueType& operator[](const KeyType& Key) const
    {
        typename BaseMapType::const_iterator Element = BaseMap.find(Key);
        CHECK(Element != BaseMap.end());
        return Element->second;
    }

    TMap& operator=(const TMap& Other)
    {
        BaseMap = Other.BaseMap;
        return *this;
    }

    TMap& operator=(TMap&& Other)
    {
        BaseMap = Move(Other.BaseMap);
        return *this;
    }

    NODISCARD bool operator==(const TMap& Other) const
    {
        return BaseMap == Other.BaseMap;
    }

    NODISCARD bool operator!=(const TMap& Other) const
    {
        return BaseMap != Other.BaseMap;
    }

private:
    BaseMapType BaseMap;
};
