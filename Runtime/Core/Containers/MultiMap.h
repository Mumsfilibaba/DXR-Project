#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/HashTable.h"

template<typename InKeyType, typename InValueType, typename AllocatorType = TDefaultHashTableAllocator<TPair<InKeyType, InValueType>>>
class TMultiMap : private THashTable<TPair<InKeyType, InValueType>, TPairKeyFuncs<InKeyType, InValueType>, true, AllocatorType>
{
    template<typename MapType, typename KeyType, typename ValueType>
    friend class TMapIterator;

public:
    typedef InKeyType                                                                    KeyType;
    typedef InValueType                                                                  ValueType;
    typedef TPair<KeyType, ValueType>                                                    PairType;
    typedef THashTable<PairType, TPairKeyFuncs<KeyType, ValueType>, true, AllocatorType> HashTableType;
    typedef TMapIterator<TMultiMap, KeyType, ValueType>                                  IteratorType;
    typedef TMapIterator<const TMultiMap, const KeyType, const ValueType>                ConstIteratorType;
    typedef int32                                                                        SizeType;

    /** @brief Default constructor, creates an empty map */
    TMultiMap() = default;

    /** @brief Copy-constructor */
    TMultiMap(const TMultiMap&) = default;

    /** @brief Move-constructor */
    TMultiMap(TMultiMap&&) = default;

    /** @brief Copy-assignment */
    TMultiMap& operator=(const TMultiMap&) = default;

    /** @brief Move-assignment */
    TMultiMap& operator=(TMultiMap&&) = default;

    /**
     * @brief Add a key-value pair, keeping any pairs that already use the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the added value
     */
    ValueType& Add(const KeyType& Key, const ValueType& Value)
    {
        return HashTableType::InsertAlways(PairType(Key, Value)).Second;
    }

    /**
     * @brief Add a key-value pair, keeping any pairs that already use the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the added value
     */
    ValueType& Add(KeyType&& Key, ValueType&& Value)
    {
        return HashTableType::InsertAlways(PairType(Move(Key), Move(Value))).Second;
    }

    /**
     * @brief Add a key-value pair, keeping any pairs that already use the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the added value
     */
    ValueType& Add(const KeyType& Key, ValueType&& Value)
    {
        return HashTableType::InsertAlways(PairType(Key, Move(Value))).Second;
    }

    /**
     * @brief Add a key-value pair, keeping any pairs that already use the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the added value
     */
    ValueType& Add(KeyType&& Key, const ValueType& Value)
    {
        return HashTableType::InsertAlways(PairType(Move(Key), Value)).Second;
    }

    /**
     * @brief Add a key-value pair, keeping any pairs that already use the key
     * @param Key Key to add
     * @param Value Value to associate with the key
     * @return Returns a reference to the added value
     */
    ValueType& Emplace(KeyType&& Key, ValueType&& Value)
    {
        return Add(Move(Key), Move(Value));
    }

    /**
     * @brief Find the first value stored under a key
     * @param Key Key to look for
     * @return Returns a pointer to the value, or nullptr if the key is not in the map
     */
    NODISCARD ValueType* Find(const KeyType& Key)
    {
        PairType* Pair = HashTableType::Find(Key);
        return Pair ? AddressOf(Pair->Second) : nullptr;
    }

    /**
     * @brief Find the first value stored under a key
     * @param Key Key to look for
     * @return Returns a pointer to the value, or nullptr if the key is not in the map
     */
    NODISCARD const ValueType* Find(const KeyType& Key) const
    {
        const PairType* Pair = HashTableType::Find(Key);
        return Pair ? AddressOf(Pair->Second) : nullptr;
    }

    /**
     * @brief Find the next value stored under a key, used to walk all values of that key
     * @param Key Key to look for
     * @param Current Value previously returned for the key, or nullptr to retrieve the first value
     * @return Returns a pointer to the next value, or nullptr when there are no more values
     */
    NODISCARD ValueType* FindNext(const KeyType& Key, const ValueType* Current)
    {
        if (!Current)
        {
            return Find(Key);
        }

        SizeType Index = HashTableType::FindSlot(Key);
        while (Index != HashInvalidSlot)
        {
            PairType& Pair = HashTableType::GetElement(Index);
            if (AddressOf(Pair.Second) == Current)
            {
                const SizeType Next = HashTableType::FindSlotAfter(Key, Index);
                return Next != HashInvalidSlot ? AddressOf(HashTableType::GetElement(Next).Second) : nullptr;
            }

            Index = HashTableType::FindSlotAfter(Key, Index);
        }

        return nullptr;
    }

    /**
     * @brief Find the next value stored under a key, used to walk all values of that key
     * @param Key Key to look for
     * @param Current Value previously returned for the key, or nullptr to retrieve the first value
     * @return Returns a pointer to the next value, or nullptr when there are no more values
     */
    NODISCARD const ValueType* FindNext(const KeyType& Key, const ValueType* Current) const
    {
        if (!Current)
        {
            return Find(Key);
        }

        SizeType Index = HashTableType::FindSlot(Key);
        while (Index != HashInvalidSlot)
        {
            const PairType& Pair = HashTableType::GetElement(Index);
            if (AddressOf(Pair.Second) == Current)
            {
                const SizeType Next = HashTableType::FindSlotAfter(Key, Index);
                return Next != HashInvalidSlot ? AddressOf(HashTableType::GetElement(Next).Second) : nullptr;
            }

            Index = HashTableType::FindSlotAfter(Key, Index);
        }

        return nullptr;
    }

    /**
     * @brief Retrieve every value stored under a key
     * @param Key Key to look for
     * @param OutValues Array the values are appended to, existing elements are kept
     */
    void MultiFind(const KeyType& Key, TArray<ValueType>& OutValues) const
    {
        SizeType Index = HashTableType::FindSlot(Key);
        while (Index != HashInvalidSlot)
        {
            OutValues.Add(HashTableType::GetElement(Index).Second);
            Index = HashTableType::FindSlotAfter(Key, Index);
        }
    }

    /**
     * @brief Count the values stored under a key
     * @param Key Key to look for
     * @return Returns the number of values stored under the key
     */
    NODISCARD SizeType Count(const KeyType& Key) const
    {
        SizeType Result = 0;
        SizeType Index  = HashTableType::FindSlot(Key);

        while (Index != HashInvalidSlot)
        {
            ++Result;
            Index = HashTableType::FindSlotAfter(Key, Index);
        }

        return Result;
    }

    /**
     * @brief Remove every pair with the specified key
     * @param Key Key of the pairs to remove
     * @return Returns the number of removed pairs
     */
    SizeType Remove(const KeyType& Key)
    {
        return HashTableType::RemoveAll(Key);
    }

    /**
     * @brief Remove every pair with the specified key
     * @param Key Key of the pairs to remove
     * @return Returns the number of removed pairs
     */
    SizeType RemoveKey(const KeyType& Key)
    {
        return Remove(Key);
    }

    /**
     * @brief Remove a single pair matching both the specified key and value
     * @param Key Key of the pair to remove
     * @param Value Value of the pair to remove
     * @return Returns true if a pair was removed
     */
    bool RemoveSingle(const KeyType& Key, const ValueType& Value)
    {
        SizeType Index = HashTableType::FindSlot(Key);
        while (Index != HashInvalidSlot)
        {
            if (HashTableType::GetElement(Index).Second == Value)
            {
                return HashTableType::RemoveSlot(Index);
            }

            Index = HashTableType::FindSlotAfter(Key, Index);
        }

        return false;
    }

    /**
     * @brief Check if a key is in the map
     * @param Key Key to look for
     * @return Returns true if at least one pair uses the key
     */
    NODISCARD bool Contains(const KeyType& Key) const
    {
        return HashTableType::Find(Key) != nullptr;
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
     * @brief Retrieve the total number of pairs in the map, counting duplicated keys once per pair
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
     * @brief Retrieve the number of allocated slots
     * @return Returns the number of buckets
     */
    NODISCARD SizeType BucketCount() const
    {
        return HashTableType::BucketCount();
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
     * @brief Call a lambda for every pair of the map, including every duplicate of a key
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
     * @brief Call a lambda for every pair of the map, including every duplicate of a key
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
     * @brief Retrieve the keys of all pairs, which repeats keys that store several values
     * @return Returns an array containing one key per pair
     */
    NODISCARD TArray<KeyType> GetKeys() const
    {
        TArray<KeyType> Keys;
        Keys.Reserve(Size());

        Foreach([&Keys](const KeyType& Key, const ValueType&)
        {
            Keys.Add(Key);
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
            Values.Add(Value);
        });

        return Values;
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
     * @brief Compare the contents of two maps, ignoring the order the pairs are stored in
     * @param Other Map to compare against
     * @return Returns true if both maps contain the same pairs the same number of times
     */
    NODISCARD bool operator==(const TMultiMap& Other) const
    {
        return HashTableType::EqualTo(static_cast<const HashTableType&>(Other));
    }

    /**
     * @brief Compare the contents of two maps, ignoring the order the pairs are stored in
     * @param Other Map to compare against
     * @return Returns true if the maps differ
     */
    NODISCARD bool operator!=(const TMultiMap& Other) const
    {
        return !(*this == Other);
    }

};

template<typename InKeyType, typename InValueType, int32 NumInlineElements>
using TInlineMultiMap = TMultiMap<InKeyType, InValueType, TInlineHashTableAllocator<TPair<InKeyType, InValueType>, HashSlotsForElements(NumInlineElements)>>;
