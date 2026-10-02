#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/HashTable.h"

template<typename InKeyType, typename InValueType, typename AllocatorType = TDefaultHashTableAllocator<TPair<InKeyType, InValueType>>>
class TMap : private THashTable<TPair<InKeyType, InValueType>, TPairKeyFuncs<InKeyType, InValueType>, false, AllocatorType>
{
    template<typename MapType, typename KeyType, typename ValueType>
    friend class TMapIterator;

public:
    typedef InKeyType                                                                     KeyType;
    typedef InValueType                                                                   ValueType;
    typedef TPair<KeyType, ValueType>                                                     PairType;
    typedef THashTable<PairType, TPairKeyFuncs<KeyType, ValueType>, false, AllocatorType> HashTableType;
    typedef TMapIterator<TMap, KeyType, ValueType>                                        IteratorType;
    typedef TMapIterator<const TMap, const KeyType, const ValueType>                      ConstIteratorType;
    typedef int32                                                                         SizeType;

    /** @brief Default constructor, creates an empty map */
    TMap() = default;

    /** @brief Copy-constructor */
    TMap(const TMap&) = default;

    /** @brief Move-constructor */
    TMap(TMap&&) = default;

    /** @brief Copy-assignment */
    TMap& operator=(const TMap&) = default;

    /** @brief Move-assignment */
    TMap& operator=(TMap&&) = default;

    /**
     * @brief Add all pairs of another map whose keys are not already in this map
     * @param Other Map to append the pairs from
     */
    void Append(const TMap& Other)
    {
        Other.Foreach([this](const KeyType& Key, const ValueType& Value)
        {
            if (!Contains(Key))
            {
                Add(Key, Value);
            }
        });
    }

    /**
     * @brief Add a key with a default constructed value, overwriting any current value of the key
     * @param Key Key to add
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(const KeyType& Key)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = ValueType();
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Key, ValueType())).Second;
    }

    /**
     * @brief Add a key with a default constructed value, overwriting any current value of the key
     * @param Key Key to add
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(KeyType&& Key)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = ValueType();
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Move(Key), ValueType())).Second;
    }

    /**
     * @brief Add a key-value pair, overwriting any current value of the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(const KeyType& Key, const ValueType& Value)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = Value;
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Key, Value)).Second;
    }

    /**
     * @brief Add a key-value pair, overwriting any current value of the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(KeyType&& Key, const ValueType& Value)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = Value;
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Move(Key), Value)).Second;
    }

    /**
     * @brief Add a key-value pair, overwriting any current value of the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(const KeyType& Key, ValueType&& Value)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = Move(Value);
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Key, Move(Value))).Second;
    }

    /**
     * @brief Add a key-value pair, overwriting any current value of the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the value of the key
     */
    ValueType& Add(KeyType&& Key, ValueType&& Value)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = Move(Value);
            return *Existing;
        }

        return HashTableType::InsertAlways(PairType(Move(Key), Move(Value))).Second;
    }

    /**
     * @brief Add a key with a default constructed value, overwriting any current value of the key
     * @param Key Key to add
     * @return Returns a reference to the value of the key
     */
    ValueType& Emplace(KeyType&& Key)
    {
        return Add(Move(Key));
    }

    /**
     * @brief Add a key-value pair, overwriting any current value of the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the value of the key
     */
    ValueType& Emplace(KeyType&& Key, ValueType&& Value)
    {
        return Add(Move(Key), Move(Value));
    }

    /**
     * @brief Find the value of a key
     * @param Key Key to look for
     * @return Returns a pointer to the value, or nullptr if the key is not in the map
     */
    NODISCARD ValueType* Find(const KeyType& Key)
    {
        PairType* Pair = HashTableType::Find(Key);
        return Pair ? AddressOf(Pair->Second) : nullptr;
    }

    /**
     * @brief Find the value of a key
     * @param Key Key to look for
     * @return Returns a pointer to the value, or nullptr if the key is not in the map
     */
    NODISCARD const ValueType* Find(const KeyType& Key) const
    {
        const PairType* Pair = HashTableType::Find(Key);
        return Pair ? AddressOf(Pair->Second) : nullptr;
    }

    /**
     * @brief Find the value of a key, adding it with a default constructed value if it is missing
     * @param Key Key to look for
     * @return Returns a reference to the value of the key
     */
    NODISCARD ValueType& FindOrAdd(const KeyType& Key)
    {
        if (ValueType* Existing = Find(Key))
        {
            return *Existing;
        }

        return Add(Key, ValueType());
    }

    /**
     * @brief Find the value of a key, and overwrite or add it with the specified value
     * @param Key Key to look for
     * @param Value Value to assign to the key
     * @return Returns a reference to the value of the key
     */
    NODISCARD ValueType& FindOrAdd(const KeyType& Key, const ValueType& Value)
    {
        if (ValueType* Existing = Find(Key))
        {
            *Existing = Value;
            return *Existing;
        }

        return Add(Key, Value);
    }

    /**
     * @brief Find the value of a key, adding it with a default constructed value if it is missing
     * @param Key Key to look for
     * @return Returns a reference to the value of the key
     */
    NODISCARD ValueType& FindOrAdd(KeyType&& Key)
    {
        if (ValueType* Existing = Find(Key))
        {
            return *Existing;
        }

        return Add(Move(Key), ValueType());
    }

    /**
     * @brief Remove the pair with the specified key
     * @param Key Key of the pair to remove
     * @return Returns true if a pair was removed
     */
    NODISCARD bool RemoveKey(const KeyType& Key)
    {
        return HashTableType::RemoveOne(Key);
    }

    /**
     * @brief Remove the pair with the specified key and retrieve the removed value
     * @param Key Key of the pair to remove
     * @param OutRemovedValue Assigned the removed value, may be nullptr
     * @return Returns true if a pair was removed
     */
    NODISCARD bool RemoveKey(const KeyType& Key, ValueType* OutRemovedValue)
    {
        if (ValueType* Found = Find(Key))
        {
            if (OutRemovedValue)
            {
                *OutRemovedValue = *Found;
            }

            return HashTableType::RemoveOne(Key);
        }

        return false;
    }

    /**
     * @brief Remove the pair with the specified key
     * @param InElement Key of the pair to remove
     */
    void Remove(const KeyType& InElement)
    {
        HashTableType::RemoveOne(InElement);
    }

    /**
     * @brief Check if a key is in the map
     * @param Key Key to look for
     * @return Returns true if the key is in the map
     */
    NODISCARD bool Contains(const KeyType& Key) const
    {
        return HashTableType::Find(Key) != nullptr;
    }

    /**
     * @brief Count the pairs with the specified key, which is never more than one
     * @param Key Key to look for
     * @return Returns one if the key is in the map, otherwise zero
     */
    NODISCARD SizeType Count(const KeyType& Key)
    {
        return Contains(Key) ? 1 : 0;
    }

    /**
     * @brief Grow the map so that the specified number of pairs fits without rehashing
     * @param InCapacity Number of pairs to make room for
     */
    void Reserve(SizeType InCapacity)
    {
        HashTableType::Reserve(InCapacity);
    }

    /** @brief Remove all pairs, but keep the allocated storage for reuse */
    void Clear()
    {
        HashTableType::Clear();
    }

    /** @brief Remove all pairs and release the storage, returning the map to its default state */
    void Reset()
    {
        HashTableType::Reset();
    }

    /**
     * @brief Rehash the map into at least the specified number of buckets
     * @param InBucketCount Minimum number of buckets, rounded up to a power of two
     */
    void Rehash(SizeType InBucketCount)
    {
        HashTableType::Rehash(InBucketCount);
    }

    /**
     * @brief Check if the map is empty
     * @return Returns true if the map contains no pairs
     */
    NODISCARD bool IsEmpty() const
    {
        return HashTableType::IsEmpty();
    }

    /**
     * @brief Retrieve the number of pairs in the map
     * @return Returns the number of pairs
     */
    NODISCARD SizeType Size() const
    {
        return HashTableType::Size();
    }

    /**
     * @brief Retrieve the number of allocated slots
     * @return Returns the number of slots
     */
    NODISCARD SizeType Capacity() const
    {
        return HashTableType::Capacity();
    }

    /**
     * @brief Retrieve the largest number of pairs the map can address
     * @return Returns the maximum number of pairs
     */
    NODISCARD SizeType MaxSize() const
    {
        return HashTableType::MaxSize();
    }

    /**
     * @brief Retrieve the number of allocated slots
     * @return Returns the number of buckets
     */
    NODISCARD SizeType BucketCount() const
    {
        return HashTableType::BucketCount();
    }

    /**
     * @brief Retrieve the current ratio of pairs to buckets
     * @return Returns the load factor
     */
    NODISCARD float LoadFactor() const
    {
        return HashTableType::LoadFactor();
    }

    /**
     * @brief Retrieve the load factor at which the map grows
     * @return Returns the maximum load factor
     */
    NODISCARD float MaxLoadFactor() const
    {
        return HashTableType::MaxLoadFactor();
    }

    /**
     * @brief Retrieve the number of bytes the map has allocated on the heap
     * @return Returns the allocated size in bytes, or zero while the storage is inline
     */
    NODISCARD uint64 GetAllocatedSize() const
    {
        return HashTableType::GetAllocatedSize();
    }

    /**
     * @brief Check if the storage of the map currently lives on the heap
     * @return Returns true if the storage is heap-allocated, false while it is inline
     */
    NODISCARD bool IsHeapAllocated() const
    {
        return HashTableType::IsHeapAllocated();
    }

    /**
     * @brief Retrieve all keys of the map
     * @return Returns an array containing a copy of every key
     */
    NODISCARD TArray<KeyType> GetKeys() const
    {
        TArray<KeyType> Keys;
        Keys.Reserve(Size());
        Foreach([&Keys](const KeyType& Key, const ValueType&)
        {
            Keys.Emplace(Key);
        });
        return Keys;
    }

    /**
     * @brief Retrieve all values of the map
     * @return Returns an array containing a copy of every value
     */
    NODISCARD TArray<ValueType> GetValues() const
    {
        TArray<ValueType> Values;
        Values.Reserve(Size());

        Foreach([&Values](const KeyType&, const ValueType& Value)
        {
            Values.Emplace(Value);
        });

        return Values;
    }

    /**
     * @brief Call a lambda for every pair of the map
     * @param Lambda Callable invoked with the key and value of each pair
     */
    template<typename LambdaType>
    void Foreach(LambdaType&& Lambda)
    {
        for (SizeType Index = HashTableType::FirstOccupied(); Index < HashTableType::BucketCount(); Index = HashTableType::NextOccupied(Index))
        {
            PairType& Pair = HashTableType::GetElement(Index);
            Lambda(Pair.First, Pair.Second);
        }
    }

    /**
     * @brief Call a lambda for every pair of the map
     * @param Lambda Callable invoked with the key and value of each pair
     */
    template<typename LambdaType>
    void Foreach(LambdaType&& Lambda) const
    {
        for (SizeType Index = HashTableType::FirstOccupied(); Index < HashTableType::BucketCount(); Index = HashTableType::NextOccupied(Index))
        {
            const PairType& Pair = HashTableType::GetElement(Index);
            Lambda(Pair.First, Pair.Second);
        }
    }

    /**
     * @brief Create an iterator to the first pair of the map
     * @return Returns the iterator
     */
    NODISCARD IteratorType CreateIterator()
    {
        return IteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Create an iterator to the first pair of the map
     * @return Returns the iterator
     */
    NODISCARD ConstIteratorType CreateIterator() const
    {
        return ConstIteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Create a constant iterator to the first pair of the map
     * @return Returns the iterator
     */
    NODISCARD ConstIteratorType CreateConstIterator() const
    {
        return ConstIteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Retrieve an iterator to the first pair of the map
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE IteratorType begin()
    {
        return CreateIterator();
    }

    /**
     * @brief Retrieve an iterator to the first pair of the map
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE ConstIteratorType begin() const
    {
        return CreateConstIterator();
    }

    /**
     * @brief Retrieve an iterator to the end of the map
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE IteratorType end()
    {
        return IteratorType(*this, HashTableType::BucketCount());
    }

    /**
     * @brief Retrieve an iterator to the end of the map
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE ConstIteratorType end() const
    {
        return ConstIteratorType(*this, HashTableType::BucketCount());
    }

    /**
     * @brief Retrieve the value of a key, adding it with a default constructed value if it is missing
     * @param Key Key to look up
     * @return Returns a reference to the value of the key
     */
    NODISCARD ValueType& operator[](const KeyType& Key)
    {
        return FindOrAdd(Key);
    }

    /**
     * @brief Retrieve the value of a key, which must be in the map
     * @param Key Key to look up
     * @return Returns a reference to the value of the key
     */
    NODISCARD const ValueType& operator[](const KeyType& Key) const
    {
        const ValueType* Found = Find(Key);
        CHECK(Found != nullptr);
        return *Found;
    }

    /**
     * @brief Compare the contents of two maps, ignoring the order the pairs are stored in
     * @param Other Map to compare against
     * @return Returns true if both maps contain the same pairs
     */
    NODISCARD bool operator==(const TMap& Other) const
    {
        return HashTableType::EqualTo(static_cast<const HashTableType&>(Other));
    }

    /**
     * @brief Compare the contents of two maps, ignoring the order the pairs are stored in
     * @param Other Map to compare against
     * @return Returns true if the maps differ
     */
    NODISCARD bool operator!=(const TMap& Other) const
    {
        return !(*this == Other);
    }

};

template<typename InKeyType, typename InValueType, int32 NumInlineElements>
using TInlineMap = TMap<InKeyType, InValueType, TInlineHashTableAllocator<TPair<InKeyType, InValueType>, HashSlotsForElements(NumInlineElements)>>;
