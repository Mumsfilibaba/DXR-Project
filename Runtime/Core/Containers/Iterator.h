#pragma once
#include "Pair.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ReferenceWrapper.h"
#include "Core/Templates/BitReference.h"

// TODO: Put some functionality into a base-class
template<typename ArrayType, typename ElementType>
class TArrayIterator
{
    typedef const typename TRemoveCV<ArrayType>::Type   ConstArrayType;
    typedef const typename TRemoveCV<ElementType>::Type ConstElementType;

public:
    typedef typename ArrayType::SizeType SizeType;

    TArrayIterator(const TArrayIterator&) = default;
    TArrayIterator(TArrayIterator&&)      = default;
    ~TArrayIterator()                     = default;

    TArrayIterator& operator=(const TArrayIterator&) = default;
    TArrayIterator& operator=(TArrayIterator&&)      = default;

    static_assert(TIsSigned<SizeType>::Value, "TArrayIterator wants a signed SizeType");

    /**
     * @brief            - Create a new iterator
     * @param InArray    - Array to iterate
     * @param StartIndex - Index in the array to start
     */
    FORCEINLINE explicit TArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false 
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return Index >= 0 && Index <= Array.Get().Size();
    }

    /** @return - Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return Index == Array.Get().Size();
    }

    /** @return - Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* GetPointer() const noexcept
    {
        CHECK(IsValid());
        return Array.Get().Data() + Index;
    }

    /** @return - Returns the index to the element that the iterator represents */
    FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index;
    }

public:

    /** @return - Returns a raw pointer to the data */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return GetPointer();
    }

    /** @return - Returns a reference to the data */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return *GetPointer();
    }

    /**
     * @brief  - Increment the index for the iterator  
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Increment the index for the iterator  
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator++(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Decrement the index for the iterator  
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator--(int) noexcept
    {
        TArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Add a new value to the iterator
     * @param Other - Value to add
     * @return      - Returns a new iterator with the result from adding Other to this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator+(SizeType Other) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator += Other;
    }

    /**
     * @brief       - Subtract a new value to the iterator
     * @param Other - Value to subtract
     * @return      - Returns a new iterator with the result from subtracting Other to this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator-(SizeType Other) const noexcept
    {
        TArrayIterator NewIterator(*this);
        return NewIterator -= Other;
    }

    /**
     * @brief       - Add a value to the iterator and store it in this instance
     * @param Other - Value to add
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator+=(SizeType Other) noexcept
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Subtract a value to the iterator and store it in this instance
     * @param Other - Value to subtract
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator-=(SizeType Other) noexcept
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TArrayIterator& Other) const noexcept
    {
        return Index == Other.Index && Other.IsFrom(Array);
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TArrayIterator<ConstArrayType, ConstElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};


template<typename ArrayType, typename ElementType>
FORCEINLINE TArrayIterator<ArrayType, ElementType> operator+(typename TArrayIterator<ArrayType, ElementType>::SizeType LHS, TArrayIterator<ArrayType, ElementType>& Other) noexcept
{
    TArrayIterator NewIterator(Other);
    return NewIterator += LHS;
}


template<typename ArrayType, typename ElementType>
class TReverseArrayIterator
{
    typedef const typename TRemoveCV<ArrayType>::Type   ConstArrayType;
    typedef const typename TRemoveCV<ElementType>::Type ConstElementType;

public:
    typedef typename ArrayType::SizeType SizeType;

    TReverseArrayIterator(const TReverseArrayIterator&) = default;
    TReverseArrayIterator(TReverseArrayIterator&&)      = default;
    ~TReverseArrayIterator()                            = default;

    TReverseArrayIterator& operator=(const TReverseArrayIterator&) = default;
    TReverseArrayIterator& operator=(TReverseArrayIterator&&)      = default;

    static_assert(TIsSigned<SizeType>::Value, "TReverseArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TReverseArrayIterator require ArrayType and ElementType to have the same constness");

    /**
     * @brief            - Create a new iterator
     * @param InArray    - Array to iterate
     * @param StartIndex - Index in the array to start
     */
    FORCEINLINE explicit TReverseArrayIterator(ArrayType& InArray, SizeType StartIndex) noexcept
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const noexcept
    {
        const ArrayType* FromPointer = ::AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return Index >= 0 && Index <= Array.Get().Size();
    }

    /** @return - Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return Index == 0;
    }

    /** @return - Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* GetPointer() const noexcept
    {
        CHECK(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    /** @return - Returns the index to the element that the iterator represents */
    NODISCARD FORCEINLINE SizeType GetIndex() const noexcept
    {
        return Index - 1;
    }

public:

    /** @return - Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return GetPointer();
    }

    /** @return - Returns a reference to the data */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return *GetPointer();
    }

    /**
     * @brief  - Increment the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator++() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Increment the index for the iterator
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator++(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator--() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Decrement the index for the iterator
     * @return - Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator--(int) noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        NewIterator++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Add a new value to the iterator
     * @param Other - Value to add
     * @return      - Returns a new iterator with the result from adding Other to this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator+(SizeType Other) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator += Other; // Uses operator, therefore +=
    }

    /**
     * @brief       - Subtract a new value to the iterator
     * @param Other - Value to subtract
     * @return      - Returns a new iterator with the result from subtracting Other to this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator-(SizeType Other) const noexcept
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator -= Other; // Uses operator, therefore -=
    }

    /**
     * @brief       - Add a value to the iterator and store it in this instance
     * @param Other - Value to add
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator+=(SizeType Other) noexcept
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Subtract a value to the iterator and store it in this instance
     * @param Other - Value to subtract
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator-=(SizeType Other) noexcept
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseArrayIterator& Other) const noexcept
    {
        return Index == Other.Index && Other.IsFrom(Array);
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseArrayIterator<ConstArrayType, ConstElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};


template<typename ArrayType, typename ElementType>
NODISCARD FORCEINLINE TReverseArrayIterator<ArrayType, ElementType> operator+(typename TReverseArrayIterator<ArrayType, ElementType>::SizeType LHS, TReverseArrayIterator<ArrayType, ElementType>& Other) noexcept
{
    TReverseArrayIterator NewIterator(Other);
    return NewIterator += LHS;
}


template<typename BitArrayType, typename StorageType>
class TBitArrayIterator
{
    typedef const typename TRemoveCV<StorageType>::Type  ConstStorageType;
    typedef const typename TRemoveCV<BitArrayType>::Type ConstBitArrayType;

public:
    typedef TBitReference<StorageType>      BitReferenceType;
    typedef TBitReference<ConstStorageType> ConstBitReferenceType;

    TBitArrayIterator(const TBitArrayIterator&) = default;
    TBitArrayIterator(TBitArrayIterator&&)      = default;
    ~TBitArrayIterator()                        = default;

    TBitArrayIterator& operator=(const TBitArrayIterator&) = default;
    TBitArrayIterator& operator=(TBitArrayIterator&&)      = default;

    /**
     * @brief            - Constructor taking array and index of the iterator
     * @param InBitArray - Array that the iterator belongs to
     * @param InIndex    - Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex) noexcept
        : Index(InIndex)
        , BitArray(InBitArray)
    {
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const noexcept
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        const auto Count = BitArray.Get().Count();
        return Index >= 0 && Index <= Count;
    }

    /** @return - Returns the value of the bit */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue() noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /** @return - Returns the value of the bit */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TBitArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator++(int) noexcept
    {
        TBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TBitArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator--(int) noexcept
    {
        TBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TBitArrayIterator& Other) const noexcept
    {
        return Index == Other.Index && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns false if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator!=(const TBitArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /** @return - Returns a reference to the data */
    NODISCARD FORCEINLINE BitReferenceType& operator*() noexcept
    {
        return GetBitValue();
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TBitArrayIterator<ConstBitArrayType, ConstStorageType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TBitArrayIterator<ConstBitArrayType, ConstStorageType>(BitArray, Index);
    }

private:
    TReferenceWrapper<BitArrayType> BitArray;
    uint32                          Index;
};


template<typename BitArrayType, typename StorageType>
class TReverseBitArrayIterator
{
    typedef const typename TRemoveCV<StorageType>::Type  ConstStorageType;
    typedef const typename TRemoveCV<BitArrayType>::Type ConstBitArrayType;

public:
    typedef TBitReference<StorageType>      BitReferenceType;
    typedef TBitReference<ConstStorageType> ConstBitReferenceType;

    TReverseBitArrayIterator(const TReverseBitArrayIterator&) = default;
    TReverseBitArrayIterator(TReverseBitArrayIterator&&)      = default;
    ~TReverseBitArrayIterator()                               = default;

    TReverseBitArrayIterator& operator=(const TReverseBitArrayIterator&) = default;
    TReverseBitArrayIterator& operator=(TReverseBitArrayIterator&&)      = default;

    /**
     * @brief            - Constructor taking array and index of the iterator
     * @param InBitArray - Array that the iterator belongs to
     * @param InIndex    - Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TReverseBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex) noexcept
        : Index(InIndex)
        , BitArray(InBitArray)
    {
    }

    /**
     * @brief           - Check if the iterator belongs to a certain array
     * @param FromArray - Array to check
     * @return          - Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const noexcept
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        const auto Count = BitArray.Get().Count();
        return Index >= 0 && Index <= Count;
    }

    /** @return - Returns the value of the bit */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue() noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /** @return - Returns the value of the bit */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const noexcept
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TReverseBitArrayIterator operator++() noexcept
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator++(int) noexcept
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TReverseBitArrayIterator operator--() noexcept
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator--(int) noexcept
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseBitArrayIterator& Other) const noexcept
    {
        return Index == Other.Index && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief       - Compare operator
     * @param Other - Other iterator to compare to
     * @return      - Returns false if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseBitArrayIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /** @return - Returns a reference to the data */
    NODISCARD FORCEINLINE BitReferenceType& operator*() noexcept
    {
        return GetBitValue();
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseBitArrayIterator<ConstBitArrayType, ConstStorageType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseBitArrayIterator<ConstBitArrayType, ConstStorageType>(BitArray, Index);
    }

private:
    TReferenceWrapper<BitArrayType> BitArray;
    uint32                          Index;
};


template<typename MapType, typename KeyType, typename ValueType>
class TMapIterator
{
public:
    typedef typename MapType::BaseMapType::iterator   BaseIteratorType;
    typedef const typename TRemoveCV<KeyType>::Type   ConstKeyType;
    typedef const typename TRemoveCV<ValueType>::Type ConstValueType;
    typedef const typename TRemoveCV<MapType>::Type   ConstMapType;

    TMapIterator(const TMapIterator&) = default;
    TMapIterator(TMapIterator&&)      = default;
    ~TMapIterator()                   = default;

    TMapIterator& operator=(const TMapIterator&) = default;
    TMapIterator& operator=(TMapIterator&&)      = default;

    explicit TMapIterator(MapType& InMap, BaseIteratorType InBaseIterator)
        : Map(InMap)
        , BaseIterator(InBaseIterator)
    {
    }

    /** @return - Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return BaseIterator == Map.Get().BaseMap.end();
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return true;
    }

    /** @return - Returns the key for this iterator */
    NODISCARD FORCEINLINE const KeyType& GetKey() const
    {
        return BaseIterator->first;
    }

    /** @return - Returns the value for this iterator */
    NODISCARD FORCEINLINE ValueType& GetValue()
    {
        return BaseIterator->second;
    }

    /** @return - Returns the value for this iterator */
    NODISCARD FORCEINLINE const ValueType& GetValue() const
    {
        return BaseIterator->second;
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TMapIterator operator++() noexcept
    {
        BaseIterator++;
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TMapIterator operator++(int) noexcept
    {
        TMapIterator NewIterator(*this);
        BaseIterator++;
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TMapIterator operator--() noexcept
    {
        BaseIterator--;
        return *this;
    }

    /**
     * @brief  - Post-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TMapIterator operator--(int) noexcept
    {
        TMapIterator NewIterator(*this);
        BaseIterator--;
        return NewIterator;
    }

    /**
     * @brief  - Retrieve key and value pair
     * @return - Returns references to the key and values from this iterator
     */
    NODISCARD FORCEINLINE TPair<ConstKeyType&, ValueType&> operator*() noexcept
    {
        return TPair<ConstKeyType&, ValueType&>{ BaseIterator->first, BaseIterator->second };
    }

    /**
     * @brief  - Retrieve key and value pair
     * @return - Returns references to the key and values from this iterator
     */
    NODISCARD FORCEINLINE TPair<ConstKeyType&, ConstValueType&> operator*() const noexcept
    {
        return TPair<ConstKeyType&, ConstValueType&>{ BaseIterator->first, BaseIterator->second };
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TMapIterator& Other) const noexcept
    {
        return BaseIterator == Other.BaseIterator;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TMapIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TMapIterator<ConstMapType, ConstKeyType, ConstValueType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TMapIterator<ConstMapType, ConstKeyType, ConstValueType>(Map, BaseIterator);
    }

private:
    TReferenceWrapper<MapType> Map;
    BaseIteratorType           BaseIterator;
};


template<typename SetType, typename ElementType>
class TSetIterator
{
public:
    typedef typename SetType::BaseSetType::iterator     BaseIteratorType;
    typedef const typename TRemoveCV<ElementType>::Type ConstElementType;
    typedef const typename TRemoveCV<SetType>::Type     ConstSetType;

    TSetIterator(const TSetIterator&) = default;
    TSetIterator(TSetIterator&&)      = default;
    ~TSetIterator()                   = default;

    TSetIterator& operator=(const TSetIterator&) = default;
    TSetIterator& operator=(TSetIterator&&)      = default;

    explicit TSetIterator(SetType& InSet, BaseIteratorType InBaseIterator)
        : Set(InSet)
        , BaseIterator(InBaseIterator)
    {
    }

    /** @return - Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const noexcept
    {
        return BaseIterator == Set.Get().BaseSet.end();
    }

    /** @return - Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return true;
    }

    /** @return - Returns the element for this iterator */
    NODISCARD FORCEINLINE const ElementType& GetElement() const
    {
        return *BaseIterator;
    }

public:

    /**
     * @brief  - Pre-increment operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TSetIterator operator++() noexcept
    {
        BaseIterator++;
        return *this;
    }

    /**
     * @brief  - Post-increment operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TSetIterator operator++(int) noexcept
    {
        TSetIterator NewIterator(*this);
        BaseIterator++;
        return NewIterator;
    }

    /**
     * @brief  - Pre-decrement operator
     * @return - Returns a iterator with the next index
     */
    FORCEINLINE TSetIterator operator--() noexcept
    {
        BaseIterator--;
        return *this;
    }

    /**
     * @brief  - Post-decrement operator
     * @return - Returns a iterator with the current index
     */
    FORCEINLINE TSetIterator operator--(int) noexcept
    {
        TSetIterator NewIterator(*this);
        BaseIterator--;
        return NewIterator;
    }

    /**
     * @brief  - Retrieve key and value pair
     * @return - Returns references to the key and values from this iterator
     */
    NODISCARD FORCEINLINE const ElementType& operator*() const noexcept
    {
        return *BaseIterator;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TSetIterator& Other) const noexcept
    {
        return BaseIterator == Other.BaseIterator;
    }

    /**
     * @brief       - Compare this and another instance
     * @param Other - Value to compare with
     * @return      - Returns false if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TSetIterator& Other) const noexcept
    {
        return !(*this == Other);
    }

    /**
     * @brief  - Create a constant iterator from this
     * @return - Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TSetIterator<ConstSetType, ConstElementType>() const noexcept
    {
        // The array type must be const here in order to make the dereference work properly
        return TSetIterator<ConstSetType, ConstElementType>(Set, BaseIterator);
    }

private:
    TReferenceWrapper<SetType> Set;
    BaseIteratorType           BaseIterator;
};
