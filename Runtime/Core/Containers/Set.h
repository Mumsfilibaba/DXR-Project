#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/HashTable.h"
#include <initializer_list>

template<typename InElementType, typename AllocatorType = TDefaultHashTableAllocator<InElementType>>
class TSet : private THashTable<InElementType, TDefaultKeyFuncs<InElementType>, false, AllocatorType>
{
    template<typename SetType, typename ElementType>
    friend class TSetIterator;

public:
    typedef InElementType                                                                ElementType;
    typedef THashTable<ElementType, TDefaultKeyFuncs<ElementType>, false, AllocatorType> HashTableType;
    typedef TSetIterator<TSet, ElementType>                                              IteratorType;
    typedef TSetIterator<const TSet, const ElementType>                                  ConstIteratorType;
    typedef int32                                                                        SizeType;

    /** @brief Default constructor, creates an empty set */
    TSet() = default;

    /** @brief Copy-constructor */
    TSet(const TSet&) = default;

    /** @brief Move-constructor */
    TSet(TSet&&) = default;

    /** @brief Copy-assignment */
    TSet& operator=(const TSet&) = default;

    /** @brief Move-assignment */
    TSet& operator=(TSet&&) = default;

    /**
     * @brief Constructor that creates a set from an std::initializer_list, skipping duplicates
     * @param InitializerList Initializer list containing the elements to add
     */
    TSet(std::initializer_list<ElementType> InitializerList)
    {
        Reserve(static_cast<SizeType>(InitializerList.size()));
        for (const ElementType& Element : InitializerList)
        {
            Add(Element);
        }
    }

    /**
     * @brief Add all elements of another set that are not already in this set
     * @param Other Set to append the elements from
     */
    void Append(const TSet& Other)
    {
        for (const ElementType& Element : Other)
        {
            Add(Element);
        }
    }

    /**
     * @brief Add an element, unless an equal element is already in the set
     * @param InElement Element to add
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    const ElementType& Add(const ElementType& InElement)
    {
        return HashTableType::InsertUniqueOrGet(InElement);
    }

    /**
     * @brief Add an element, unless an equal element is already in the set
     * @param InElement Element to add
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    const ElementType& Add(ElementType&& InElement)
    {
        return HashTableType::InsertUniqueOrGet(Move(InElement));
    }

    /**
     * @brief Add an element, unless an equal element is already in the set
     * @param InElement Element to add
     * @param OutAlreadyInSet Assigned true if the element was already in the set, may be nullptr
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    const ElementType& Add(const ElementType& InElement, bool* OutAlreadyInSet)
    {
        const bool bAlready = HashTableType::Find(InElement) != nullptr;
        if (OutAlreadyInSet)
        {
            *OutAlreadyInSet = bAlready;
        }

        return HashTableType::InsertUniqueOrGet(InElement);
    }

    /**
     * @brief Add an element, unless an equal element is already in the set
     * @param InElement Element to add
     * @param OutAlreadyInSet Assigned true if the element was already in the set, may be nullptr
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    const ElementType& Add(ElementType&& InElement, bool* OutAlreadyInSet)
    {
        const bool bAlready = HashTableType::Find(InElement) != nullptr;
        if (OutAlreadyInSet)
        {
            *OutAlreadyInSet = bAlready;
        }

        return HashTableType::InsertUniqueOrGet(Move(InElement));
    }

    /**
     * @brief Construct an element in place and add it, unless an equal element is already in the set
     * @param Args Arguments to construct the element from
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    template<typename... ArgTypes>
    const ElementType& Emplace(ArgTypes&&... Args)
    {
        ElementType NewElement(Forward<ArgTypes>(Args)...);
        return Add(Move(NewElement));
    }

    /**
     * @brief Construct an element in place and add it, unless an equal element is already in the set
     * @param OutAlreadyInSet Assigned true if the element was already in the set, may be nullptr
     * @param Args Arguments to construct the element from
     * @return Returns a reference to the added element, or to the equal element already in the set
     */
    template<typename... ArgTypes>
    const ElementType& Emplace(bool* OutAlreadyInSet, ArgTypes&&... Args)
    {
        ElementType NewElement(Forward<ArgTypes>(Args)...);
        return Add(Move(NewElement), OutAlreadyInSet);
    }

    /**
     * @brief Find an element equal to the specified element
     * @param InElement Element to look for
     * @return Returns a pointer to the element in the set, or nullptr if it is not in the set
     */
    NODISCARD const ElementType* Find(const ElementType& InElement) const
    {
        return HashTableType::Find(InElement);
    }

    /**
     * @brief Find an element equal to the specified element, adding it if it is missing
     * @param InElement Element to look for
     * @return Returns a reference to the element in the set
     */
    NODISCARD const ElementType& FindOrAdd(const ElementType& InElement)
    {
        return Add(InElement);
    }

    /**
     * @brief Find an element equal to the specified element, adding it if it is missing
     * @param InElement Element to look for
     * @return Returns a reference to the element in the set
     */
    NODISCARD const ElementType& FindOrAdd(ElementType&& InElement)
    {
        return Add(Move(InElement));
    }

    /**
     * @brief Remove the element equal to the specified element
     * @param InElement Element to remove
     */
    void Remove(const ElementType& InElement)
    {
        HashTableType::RemoveOne(InElement);
    }

    /**
     * @brief Remove the element equal to the specified element
     * @param InElement Element to remove
     * @return Returns true if an element was removed
     */
    NODISCARD bool RemoveKey(const ElementType& InElement)
    {
        return HashTableType::RemoveOne(InElement);
    }

    /**
     * @brief Remove the element equal to the specified element and retrieve the removed element
     * @param InElement Element to remove
     * @param OutRemovedElement Assigned the removed element, may be nullptr
     * @return Returns true if an element was removed
     */
    NODISCARD bool RemoveKey(const ElementType& InElement, ElementType* OutRemovedElement)
    {
        if (const ElementType* Found = Find(InElement))
        {
            if (OutRemovedElement)
            {
                *OutRemovedElement = *Found;
            }

            return HashTableType::RemoveOne(InElement);
        }

        return false;
    }

    /**
     * @brief Check if an element is in the set
     * @param InElement Element to look for
     * @return Returns true if an equal element is in the set
     */
    NODISCARD bool Contains(const ElementType& InElement) const
    {
        return HashTableType::Find(InElement) != nullptr;
    }

    /**
     * @brief Grow the set so that the specified number of elements fits without rehashing
     * @param InCapacity Number of elements to make room for
     */
    void Reserve(SizeType InCapacity)
    {
        HashTableType::Reserve(InCapacity);
    }

    /** @brief Remove all elements, but keep the allocated storage for reuse */
    void Clear()
    {
        HashTableType::Clear();
    }

    /** @brief Remove all elements and release the storage, returning the set to its default state */
    void Reset()
    {
        HashTableType::Reset();
    }

    /**
     * @brief Check if the set is empty
     * @return Returns true if the set contains no elements
     */
    NODISCARD bool IsEmpty() const
    {
        return HashTableType::IsEmpty();
    }

    /**
     * @brief Retrieve the number of elements in the set
     * @return Returns the number of elements
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
     * @brief Retrieve the largest number of elements the set can address
     * @return Returns the maximum number of elements
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
     * @brief Retrieve the current ratio of elements to buckets
     * @return Returns the load factor
     */
    NODISCARD float LoadFactor() const
    {
        return HashTableType::LoadFactor();
    }

    /**
     * @brief Retrieve the load factor at which the set grows
     * @return Returns the maximum load factor
     */
    NODISCARD float MaxLoadFactor() const
    {
        return HashTableType::MaxLoadFactor();
    }

    /**
     * @brief Retrieve the number of bytes the set has allocated on the heap
     * @return Returns the allocated size in bytes, or zero while the storage is inline
     */
    NODISCARD uint64 GetAllocatedSize() const
    {
        return HashTableType::GetAllocatedSize();
    }

    /**
     * @brief Check if the storage of the set currently lives on the heap
     * @return Returns true if the storage is heap-allocated, false while it is inline
     */
    NODISCARD bool IsHeapAllocated() const
    {
        return HashTableType::IsHeapAllocated();
    }

    /**
     * @brief Retrieve all elements of the set
     * @return Returns an array containing a copy of every element
     */
    NODISCARD TArray<ElementType> GetValues() const
    {
        TArray<ElementType> Values;
        Values.Reserve(Size());

        for (const ElementType& Element : *this)
        {
            Values.Add(Element);
        }

        return Values;
    }

    /**
     * @brief Create an iterator to the first element of the set
     * @return Returns the iterator
     */
    NODISCARD IteratorType CreateIterator()
    {
        return IteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Create an iterator to the first element of the set
     * @return Returns the iterator
     */
    NODISCARD ConstIteratorType CreateIterator() const
    {
        return ConstIteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Create a constant iterator to the first element of the set
     * @return Returns the iterator
     */
    NODISCARD ConstIteratorType CreateConstIterator() const
    {
        return ConstIteratorType(*this, HashTableType::FirstOccupied());
    }

    /**
     * @brief Retrieve an iterator to the first element of the set
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE IteratorType begin()
    {
        return CreateIterator();
    }

    /**
     * @brief Retrieve an iterator to the first element of the set
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE ConstIteratorType begin() const
    {
        return CreateConstIterator();
    }

    /**
     * @brief Retrieve an iterator to the end of the set
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE IteratorType end()
    {
        return IteratorType(*this, HashTableType::BucketCount());
    }

    /**
     * @brief Retrieve an iterator to the end of the set
     * @return Returns the iterator
     */
    NODISCARD FORCEINLINE ConstIteratorType end() const
    {
        return ConstIteratorType(*this, HashTableType::BucketCount());
    }

    /**
     * @brief Compare the contents of two sets, ignoring the order the elements are stored in
     * @param Other Set to compare against
     * @return Returns true if both sets contain the same elements
     */
    NODISCARD bool operator==(const TSet& Other) const
    {
        return HashTableType::EqualTo(static_cast<const HashTableType&>(Other));
    }

    /**
     * @brief Compare the contents of two sets, ignoring the order the elements are stored in
     * @param Other Set to compare against
     * @return Returns true if the sets differ
     */
    NODISCARD bool operator!=(const TSet& Other) const
    {
        return !(*this == Other);
    }

};

template<typename InElementType, int32 NumInlineElements>
using TInlineSet = TSet<InElementType, TInlineHashTableAllocator<InElementType, HashSlotsForElements(NumInlineElements)>>;
