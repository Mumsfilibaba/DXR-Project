#pragma once
#include "Core/Containers/Pair.h"
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
    TArrayIterator(TArrayIterator&&) = default;
    ~TArrayIterator() = default;

    TArrayIterator& operator=(const TArrayIterator&) = default;
    TArrayIterator& operator=(TArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TArrayIterator wants a signed SizeType");

    /**
     * @brief Create a new iterator
     * @param InArray Array to iterate
     * @param StartIndex Index in the array to start
     */
    FORCEINLINE explicit TArrayIterator(ArrayType& InArray, SizeType StartIndex)
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief Check if the iterator belongs to a certain array
     * @param FromArray Array to check
     * @return Returns true if the iterator is from the array, otherwise false 
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const
    {
        const ArrayType* FromPointer = AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /** @return Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Index >= 0 && Index <= Array.Get().Size();
    }

    /** @return Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const
    {
        return Index == Array.Get().Size();
    }

    /** @return Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* GetPointer() const
    {
        CHECK(IsValid());
        return Array.Get().Data() + Index;
    }

    /** @return Returns the index to the element that the iterator represents */
    FORCEINLINE SizeType GetIndex() const
    {
        return Index;
    }

public:

    /** @return Returns a raw pointer to the data */
    FORCEINLINE ElementType* operator->() const
    {
        return GetPointer();
    }

    /** @return Returns a reference to the data */
    FORCEINLINE ElementType& operator*() const
    {
        return *GetPointer();
    }

    /**
     * @brief Increment the index for the iterator  
     * @return Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator++()
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Increment the index for the iterator  
     * @return Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator++(int)
    {
        TArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Decrement the index for the iterator
     * @return Returns a new iterator with the new value
     */
    FORCEINLINE TArrayIterator operator--()
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Decrement the index for the iterator  
     * @return Returns a new iterator with the previous value
     */
    FORCEINLINE TArrayIterator operator--(int)
    {
        TArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Add a new value to the iterator
     * @param Other Value to add
     * @return Returns a new iterator with the result from adding Other to this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator+(SizeType Other) const
    {
        TArrayIterator NewIterator(*this);
        return NewIterator += Other;
    }

    /**
     * @brief Subtract a new value from the iterator
     * @param Other Value to subtract
     * @return Returns a new iterator with the result from subtracting Other from this value 
     */
    NODISCARD FORCEINLINE TArrayIterator operator-(SizeType Other) const
    {
        TArrayIterator NewIterator(*this);
        return NewIterator -= Other;
    }

    /**
     * @brief Add a value to the iterator and store it in this instance
     * @param Other Value to add
     * @return Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator+=(SizeType Other)
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Subtract a value from the iterator and store it in this instance
     * @param Other Value to subtract
     * @return Returns a reference to this instance
     */
    FORCEINLINE TArrayIterator& operator-=(SizeType Other)
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Compare this and another instance
     * @param Other Value to compare with
     * @return Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TArrayIterator& Other) const
    {
        return Index == Other.Index && Other.IsFrom(Array);
    }

    /**
     * @brief Compare this and another instance
     * @param Other Value to compare with
     * @return Returns true if the iterators are not equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TArrayIterator& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Create a constant iterator from this
     * @return Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TArrayIterator<ConstArrayType, ConstElementType>() const
    {
        // The array type must be const here in order to make the dereference work properly
        return TArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};

template<typename ArrayType, typename ElementType>
FORCEINLINE TArrayIterator<ArrayType, ElementType> operator+(typename TArrayIterator<ArrayType, ElementType>::SizeType LHS, TArrayIterator<ArrayType, ElementType>& Other)
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
    TReverseArrayIterator(TReverseArrayIterator&&) = default;
    ~TReverseArrayIterator() = default;

    TReverseArrayIterator& operator=(const TReverseArrayIterator&) = default;
    TReverseArrayIterator& operator=(TReverseArrayIterator&&) = default;

    static_assert(TIsSigned<SizeType>::Value, "TReverseArrayIterator wants a signed SizeType");
    static_assert(TIsConst<ArrayType>::Value == TIsConst<ElementType>::Value, "TReverseArrayIterator requires ArrayType and ElementType to have the same constness");

    /**
     * @brief Create a new iterator
     * @param InArray Array to iterate
     * @param StartIndex Index in the array to start
     */
    FORCEINLINE explicit TReverseArrayIterator(ArrayType& InArray, SizeType StartIndex)
        : Array(InArray)
        , Index(StartIndex)
    {
        CHECK(IsValid());
    }

    /**
     * @brief Check if the iterator belongs to a certain array
     * @param FromArray Array to check
     * @return Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const ArrayType& FromArray) const
    {
        const ArrayType* FromPointer = ::AddressOf(FromArray);
        return Array.AddressOf() == FromPointer;
    }

    /** @return Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Index >= 0 && Index <= Array.Get().Size();
    }

    /** @return Returns true if the iterator is the end-iterator */
    NODISCARD FORCEINLINE bool IsEnd() const
    {
        return Index == 0;
    }

    /** @return Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* GetPointer() const
    {
        CHECK(IsValid());
        return Array.Get().Data() + GetIndex();
    }

    /** @return Returns the index to the element that the iterator represents */
    NODISCARD FORCEINLINE SizeType GetIndex() const
    {
        return Index - 1;
    }

public:

    /** @return Returns a raw pointer to the data */
    NODISCARD FORCEINLINE ElementType* operator->() const
    {
        return GetPointer();
    }

    /** @return Returns a reference to the data */
    NODISCARD FORCEINLINE ElementType& operator*() const
    {
        return *GetPointer();
    }

    /**
     * @brief Increment the index for the iterator
     * @return Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator++()
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Increment the index for the iterator
     * @return Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator++(int)
    {
        TReverseArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Decrement the index for the iterator
     * @return Returns a new iterator with the new value
     */
    FORCEINLINE TReverseArrayIterator operator--()
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Decrement the index for the iterator
     * @return Returns a new iterator with the previous value
     */
    FORCEINLINE TReverseArrayIterator operator--(int)
    {
        TReverseArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Add a new value to the iterator
     * @param Other Value to add
     * @return Returns a new iterator with the result from adding Other to this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator+(SizeType Other) const
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator += Other; // Uses operator, therefore +=
    }

    /**
     * @brief Subtract a new value from the iterator
     * @param Other Value to subtract
     * @return Returns a new iterator with the result from subtracting Other from this value
     */
    NODISCARD FORCEINLINE TReverseArrayIterator operator-(SizeType Other) const
    {
        TReverseArrayIterator NewIterator(*this);
        return NewIterator -= Other; // Uses operator, therefore -=
    }

    /**
     * @brief Add a value to the iterator and store it in this instance
     * @param Other Value to add
     * @return Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator+=(SizeType Other)
    {
        Index -= Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Subtract a value from the iterator and store it in this instance
     * @param Other Value to subtract
     * @return Returns a reference to this instance
     */
    FORCEINLINE TReverseArrayIterator& operator-=(SizeType Other)
    {
        Index += Other;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Compare this and another instance
     * @param Other Value to compare with
     * @return Returns true if the iterators are equal
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseArrayIterator& Other) const
    {
        return Index == Other.Index && Other.IsFrom(Array);
    }

    /**
     * @brief Compare this and another instance
     * @param Other Value to compare with
     * @return Returns true if the iterators are not equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseArrayIterator& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Create a constant iterator from this
     * @return Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseArrayIterator<ConstArrayType, ConstElementType>() const
    {
        // The array type must be const here in order to make the dereference work properly
        return TReverseArrayIterator<ConstArrayType, ConstElementType>(Array, Index);
    }

private:
    TReferenceWrapper<ArrayType> Array;
    SizeType                     Index;
};

template<typename ArrayType, typename ElementType>
NODISCARD FORCEINLINE TReverseArrayIterator<ArrayType, ElementType> operator+(typename TReverseArrayIterator<ArrayType, ElementType>::SizeType LHS, TReverseArrayIterator<ArrayType, ElementType>& Other)
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
    TBitArrayIterator(TBitArrayIterator&&) = default;
    ~TBitArrayIterator() = default;

    TBitArrayIterator& operator=(const TBitArrayIterator&) = default;
    TBitArrayIterator& operator=(TBitArrayIterator&&) = default;

    /**
     * @brief Constructor taking array and index of the iterator
     * @param InBitArray Array that the iterator belongs to
     * @param InIndex Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex)
        : Index(InIndex)
        , BitArray(InBitArray)
    {
    }

    /**
     * @brief Check if the iterator belongs to a certain array
     * @param FromArray Array to check
     * @return Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /** @return Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        const auto Count = BitArray.Get().Count();
        return Index >= 0 && Index <= Count;
    }

    /** @return Returns the value of the bit */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue()
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /** @return Returns the value of the bit */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief Pre-increment operator
     * @return Returns an iterator with the next index
     */
    FORCEINLINE TBitArrayIterator operator++()
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Post-increment operator
     * @return Returns an iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator++(int)
    {
        TBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Pre-decrement operator
     * @return Returns an iterator with the previous index
     */
    FORCEINLINE TBitArrayIterator operator--()
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Post-decrement operator
     * @return Returns an iterator with the current index
     */
    FORCEINLINE TBitArrayIterator operator--(int)
    {
        TBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Compare operator
     * @param Other Other iterator to compare to
     * @return Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TBitArrayIterator& Other) const
    {
        return Index == Other.Index && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief Compare operator
     * @param Other Other iterator to compare to
     * @return Returns true if the iterators are not equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TBitArrayIterator& Other) const
    {
        return !(*this == Other);
    }

    /** @return Returns a reference to the data */
    NODISCARD FORCEINLINE BitReferenceType& operator*()
    {
        return GetBitValue();
    }

    /**
     * @brief Create a constant iterator from this
     * @return Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TBitArrayIterator<ConstBitArrayType, ConstStorageType>() const
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
    TReverseBitArrayIterator(TReverseBitArrayIterator&&) = default;
    ~TReverseBitArrayIterator() = default;

    TReverseBitArrayIterator& operator=(const TReverseBitArrayIterator&) = default;
    TReverseBitArrayIterator& operator=(TReverseBitArrayIterator&&) = default;

    /**
     * @brief Constructor taking array and index of the iterator
     * @param InBitArray Array that the iterator belongs to
     * @param InIndex Index of the bit inside the BitArray
     */
    FORCEINLINE explicit TReverseBitArrayIterator(const BitArrayType& InBitArray, uint32 InIndex)
        : Index(InIndex)
        , BitArray(InBitArray)
    {
    }

    /**
     * @brief Check if the iterator belongs to a certain array
     * @param FromArray Array to check
     * @return Returns true if the iterator is from the array, otherwise false
     */
    NODISCARD FORCEINLINE bool IsFrom(const BitArrayType& FromArray) const
    {
        const BitArrayType* FromPointer = AddressOf(FromArray);
        return BitArray.AddressOf() == FromPointer;
    }

    /** @return Returns true if the iterator is valid */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        const auto Count = BitArray.Get().Count();
        return Index >= 0 && Index <= Count;
    }

    /** @return Returns the value of the bit */
    NODISCARD FORCEINLINE BitReferenceType GetBitValue()
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

    /** @return Returns the value of the bit */
    NODISCARD FORCEINLINE ConstBitReferenceType GetBitValue() const
    {
        CHECK(IsValid());
        return BitArray.Get().GetBitReference(Index);
    }

public:

    /**
     * @brief Pre-increment operator
     * @return Returns an iterator with the next index
     */
    FORCEINLINE TReverseBitArrayIterator operator++()
    {
        Index++;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Post-increment operator
     * @return Returns an iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator++(int)
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index++;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Pre-decrement operator
     * @return Returns an iterator with the previous index
     */
    FORCEINLINE TReverseBitArrayIterator operator--()
    {
        Index--;
        CHECK(IsValid());
        return *this;
    }

    /**
     * @brief Post-decrement operator
     * @return Returns an iterator with the current index
     */
    FORCEINLINE TReverseBitArrayIterator operator--(int)
    {
        TReverseBitArrayIterator NewIterator(*this);
        Index--;
        CHECK(IsValid());
        return NewIterator;
    }

    /**
     * @brief Compare operator
     * @param Other Other iterator to compare to
     * @return Returns true if the index is the same and the iterators belong to the same BitArray
     */
    NODISCARD FORCEINLINE bool operator==(const TReverseBitArrayIterator& Other) const
    {
        return Index == Other.Index && BitArray.IsFrom(Other.BitArray);
    }

    /**
     * @brief Compare operator
     * @param Other Other iterator to compare to
     * @return Returns true if the iterators are not equal
     */
    NODISCARD FORCEINLINE bool operator!=(const TReverseBitArrayIterator& Other) const
    {
        return !(*this == Other);
    }

    /** @return Returns a reference to the data */
    NODISCARD FORCEINLINE BitReferenceType& operator*()
    {
        return GetBitValue();
    }

    /**
     * @brief Create a constant iterator from this
     * @return Returns a new iterator based on the index from this instance
     */
    NODISCARD FORCEINLINE operator TReverseBitArrayIterator<ConstBitArrayType, ConstStorageType>() const
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
    typedef const typename TRemoveCV<KeyType>::Type   ConstKeyType;
    typedef const typename TRemoveCV<ValueType>::Type ConstValueType;
    typedef const typename TRemoveCV<MapType>::Type   ConstMapType;
    typedef typename MapType::SizeType                SizeType;

    TMapIterator(const TMapIterator&) = default;
    TMapIterator(TMapIterator&&) = default;
    ~TMapIterator() = default;

    TMapIterator& operator=(const TMapIterator&) = default;
    TMapIterator& operator=(TMapIterator&&) = default;

    explicit TMapIterator(MapType& InMap, SizeType InIndex)
        : Map(InMap)
        , Index(InIndex)
    {
    }

    NODISCARD FORCEINLINE bool IsEnd() const
    {
        return Index >= Map.Get().BucketCount() || Index < 0;
    }

    NODISCARD FORCEINLINE bool IsValid() const
    {
        return true;
    }

    NODISCARD FORCEINLINE const KeyType& GetKey() const
    {
        return Map.Get().GetElement(Index).First;
    }

    NODISCARD FORCEINLINE ValueType& GetValue()
    {
        return Map.Get().GetElement(Index).Second;
    }

    NODISCARD FORCEINLINE const ValueType& GetValue() const
    {
        return Map.Get().GetElement(Index).Second;
    }

public:
    FORCEINLINE TMapIterator operator++()
    {
        Index = Map.Get().NextOccupied(Index);
        return *this;
    }

    FORCEINLINE TMapIterator operator++(int)
    {
        TMapIterator NewIterator(*this);
        Index = Map.Get().NextOccupied(Index);
        return NewIterator;
    }

    FORCEINLINE TMapIterator operator--()
    {
        Index = Map.Get().PrevOccupied(Index);
        return *this;
    }

    FORCEINLINE TMapIterator operator--(int)
    {
        TMapIterator NewIterator(*this);
        Index = Map.Get().PrevOccupied(Index);
        return NewIterator;
    }

    NODISCARD FORCEINLINE TPair<ConstKeyType&, ValueType&> operator*()
    {
        auto& Pair = Map.Get().GetElement(Index);
        return TPair<ConstKeyType&, ValueType&>(Pair.First, Pair.Second);
    }

    NODISCARD FORCEINLINE TPair<ConstKeyType&, ConstValueType&> operator*() const
    {
        const auto& Pair = Map.Get().GetElement(Index);
        return TPair<ConstKeyType&, ConstValueType&>(Pair.First, Pair.Second);
    }

    NODISCARD FORCEINLINE bool operator==(const TMapIterator& Other) const
    {
        return Index == Other.Index && Map.AddressOf() == Other.Map.AddressOf();
    }

    NODISCARD FORCEINLINE bool operator!=(const TMapIterator& Other) const
    {
        return !(*this == Other);
    }

    NODISCARD FORCEINLINE operator TMapIterator<ConstMapType, ConstKeyType, ConstValueType>() const
    {
        return TMapIterator<ConstMapType, ConstKeyType, ConstValueType>(Map.Get(), Index);
    }

private:
    TReferenceWrapper<MapType> Map;
    SizeType                   Index;
};

template<typename SetType, typename ElementType>
class TSetIterator
{
public:
    typedef const typename TRemoveCV<ElementType>::Type ConstElementType;
    typedef const typename TRemoveCV<SetType>::Type     ConstSetType;
    typedef typename SetType::SizeType                  SizeType;

    TSetIterator(const TSetIterator&) = default;
    TSetIterator(TSetIterator&&) = default;
    ~TSetIterator() = default;

    TSetIterator& operator=(const TSetIterator&) = default;
    TSetIterator& operator=(TSetIterator&&) = default;

    explicit TSetIterator(SetType& InSet, SizeType InIndex)
        : Set(InSet)
        , Index(InIndex)
    {
    }

    NODISCARD FORCEINLINE bool IsEnd() const
    {
        return Index >= Set.Get().BucketCount() || Index < 0;
    }

    NODISCARD FORCEINLINE bool IsValid() const
    {
        return true;
    }

    NODISCARD FORCEINLINE const ElementType& GetElement() const
    {
        return Set.Get().GetElement(Index);
    }

public:
    FORCEINLINE TSetIterator operator++()
    {
        Index = Set.Get().NextOccupied(Index);
        return *this;
    }

    FORCEINLINE TSetIterator operator++(int)
    {
        TSetIterator NewIterator(*this);
        Index = Set.Get().NextOccupied(Index);
        return NewIterator;
    }

    FORCEINLINE TSetIterator operator--()
    {
        Index = Set.Get().PrevOccupied(Index);
        return *this;
    }

    FORCEINLINE TSetIterator operator--(int)
    {
        TSetIterator NewIterator(*this);
        Index = Set.Get().PrevOccupied(Index);
        return NewIterator;
    }

    NODISCARD FORCEINLINE const ElementType& operator*() const
    {
        return Set.Get().GetElement(Index);
    }

    NODISCARD FORCEINLINE bool operator==(const TSetIterator& Other) const
    {
        return Index == Other.Index && Set.AddressOf() == Other.Set.AddressOf();
    }

    NODISCARD FORCEINLINE bool operator!=(const TSetIterator& Other) const
    {
        return !(*this == Other);
    }

    NODISCARD FORCEINLINE operator TSetIterator<ConstSetType, ConstElementType>() const
    {
        return TSetIterator<ConstSetType, ConstElementType>(Set.Get(), Index);
    }

private:
    TReferenceWrapper<SetType> Set;
    SizeType                   Index;
};
