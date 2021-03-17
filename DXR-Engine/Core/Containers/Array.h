#pragma once
#include "Utilities.h"
#include "Iterator.h"
#include "Allocator.h"

#include <initializer_list>

// TArray - Dynamic Array similar to std::vector

template<typename T, typename TAllocator = Mallocator>
class TArray
{
public:
    typedef T*                        Iterator;
    typedef const T*                  ConstIterator;
    typedef TReverseIterator<T>       ReverseIterator;
    typedef TReverseIterator<const T> ConstReverseIterator;
    typedef uint32                    SizeType;

    TArray() noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
    }

    explicit TArray(SizeType Size) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalConstruct(Size);
    }

    explicit TArray(SizeType Size, const T& Value) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalConstruct(Size, Value);
    }

    template<typename TInput>
    explicit TArray(TInput Begin, TInput End) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalConstruct(Begin, End);
    }

    TArray(std::initializer_list<T> List) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalConstruct(List.begin(), List.end());
    }

    TArray(const TArray& Other) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalConstruct(Other.Begin(), Other.End());
    }

    TArray(TArray&& Other) noexcept
        : mArray(nullptr)
        , mSize(0)
        , mCapacity(0)
        , mAllocator()
    {
        InternalMove(::Forward<TArray>(Other));
    }

    ~TArray() 
    {
        Clear();
        InternalReleaseData();
        mCapacity = 0;
    }

    void Clear() noexcept
    {
        InternalDestructRange(mArray, mArray + mSize);
        mSize = 0;
    }

    void Assign(SizeType Size) noexcept
    {
        Clear();
        InternalConstruct(Size);
    }

    void Assign(SizeType Size, const T& Value) noexcept
    {
        Clear();
        InternalConstruct(Size, Value);
    }

    template<typename TInput>
    void Assign(TInput Begin, TInput End) noexcept
    {
        Clear();
        InternalConstruct(Begin, End);
    }

    void Assign(std::initializer_list<T> List) noexcept
    {
        Clear();
        InternalConstruct(List.begin(), List.end());
    }

    void Fill(const T& Value) noexcept
    {
        T* ArrayBegin = mArray;
        T* ArrayEnd   = ArrayBegin + mSize;

        while (ArrayBegin != ArrayEnd)
        {
            *ArrayBegin = Value;
            ArrayBegin++;
        }
    }

    void Fill(T&& Value) noexcept
    {
        T* ArrayBegin = mArray;
        T* ArrayEnd   = ArrayBegin + mSize;

        while (ArrayBegin != ArrayEnd)
        {
            *ArrayBegin = ::Move(Value);
            ArrayBegin++;
        }
    }

    void Resize(SizeType InSize) noexcept
    {
        if (InSize > mSize)
        {
            if (InSize > mCapacity)
            {
                InternalRealloc(InSize);
            }

            InternalDefaultConstructRange(mArray + mSize, mArray + InSize);
        }
        else if (InSize < mSize)
        {
            InternalDestructRange(mArray + InSize, mArray + mSize);
        }

        mSize = InSize;
    }

    void Resize(SizeType InSize, const T& Value) noexcept
    {
        if (InSize > mSize)
        {
            if (InSize > mCapacity)
            {
                InternalRealloc(InSize);
            }

            InternalCopyEmplace(InSize - mSize, Value, mArray + mSize);
        }
        else if (InSize < mSize)
        {
            InternalDestructRange(mArray + InSize, mArray + mSize);
        }

        mSize = InSize;
    }

    void Reserve(SizeType Capacity) noexcept
    {
        if (Capacity != mCapacity)
        {
            SizeType OldSize = mSize;
            if (Capacity < mSize)
            {
                mSize = Capacity;
            }

            T* TempData = InternalAllocateElements(Capacity);
            InternalMoveEmplace(mArray, mArray + mSize, TempData);
            InternalDestructRange(mArray, mArray + OldSize);

            InternalReleaseData();
            mArray      = TempData;
            mCapacity = Capacity;
        }
    }

    template<typename... TArgs>
    T& EmplaceBack(TArgs&&... Args) noexcept
    {
        if (mSize >= mCapacity)
        {
            const SizeType NewCapacity = InternalGetResizeFactor();
            InternalRealloc(NewCapacity);
        }

        T* DataEnd = mArray + mSize;
        new(reinterpret_cast<void*>(DataEnd)) T(::Forward<TArgs>(Args)...);
        mSize++;
        return (*DataEnd);
    }

    T& PushBack(const T& Element) noexcept
    {
        return EmplaceBack(Element);
    }

    T& PushBack(T&& Element) noexcept
    {
        return EmplaceBack(::Move(Element));
    }

    template<typename... TArgs>
    Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
    {
        Assert(InternalIsIteratorOwner(Pos));

        if (Pos == End())
        {
            const SizeType OldSize = mSize;
            EmplaceBack(::Forward<TArgs>(Args)...);
            return End() - 1;
        }

        const SizeType Index = InternalIndex(Pos);
        T* DataBegin = mArray + Index;
        if (mSize >= mCapacity)
        {
            const SizeType NewCapacity = InternalGetResizeFactor();
            InternalEmplaceRealloc(NewCapacity, DataBegin, 1);
            DataBegin = mArray + Index;
        }
        else
        {
            // Construct the range so that we can move to It
            T* DataEnd = mArray + mSize;
            InternalDefaultConstructRange(DataEnd, DataEnd + 1);
            InternalMemmoveForward(DataBegin, DataEnd, DataEnd);
            InternalDestruct(DataBegin);
        }

        new (reinterpret_cast<void*>(DataBegin)) T(::Forward<TArgs>(Args)...);
        mSize++;
        return Iterator(DataBegin);
    }

    Iterator Insert(Iterator Pos, const T& Value) noexcept
    {
        return Emplace(Pos, Value);
    }

    Iterator Insert(Iterator Pos, T&& Value) noexcept
    {
        return Emplace(Pos, ::Move(Value));
    }

    Iterator Insert(ConstIterator Pos, const T& Value) noexcept
    {
        return Emplace(Pos, Value);
    }

    Iterator Insert(ConstIterator Pos, T&& Value) noexcept
    {
        return Emplace(Pos, ::Move(Value));
    }

    Iterator Insert(Iterator Pos, std::initializer_list<T> List) noexcept
    {
        return Insert(ConstIterator(Pos), List);
    }

    Iterator Insert(ConstIterator Pos, std::initializer_list<T> List) noexcept
    {
        Assert(InternalIsIteratorOwner(Pos));

        if (Pos == End())
        {
            for (const T& Value : List)
            {
                EmplaceBack(::Move(Value));
            }

            return End() - 1;
        }

        const SizeType ListSize = static_cast<SizeType>(List.size());
        const SizeType NewSize  = mSize + ListSize;
        const SizeType Index    = InternalIndex(Pos);

        T* RangeBegin = mArray + Index;
        if (NewSize >= mCapacity)
        {
            const SizeType NewCapacity = InternalGetResizeFactor(NewSize);
            InternalEmplaceRealloc(NewCapacity, RangeBegin, ListSize);
            RangeBegin = mArray + Index;
        }
        else
        {
            // Construct the range so that we can move to It
            T* DataEnd    = mArray + mSize;
            T* NewDataEnd = mArray + mSize + ListSize;
            T* RangeEnd   = RangeBegin + ListSize;
            InternalDefaultConstructRange(DataEnd, NewDataEnd);
            InternalMemmoveForward(RangeBegin, DataEnd, NewDataEnd - 1);
            InternalDestructRange(RangeBegin, RangeEnd);
        }

        // TODO: Get rid of const_cast
        InternalMoveEmplace(const_cast<T*>(List.begin()), const_cast<T*>(List.end()), RangeBegin);
        mSize = NewSize;
        return Iterator(RangeBegin);
    }

    template<typename TInput>
    Iterator Insert(Iterator Pos, TInput Begin, TInput End) noexcept
    {
        return Insert(Pos, Begin, End);
    }

    template<typename TInput>
    Iterator Insert(ConstIterator Pos, TInput InBegin, TInput InEnd) noexcept
    {
        Assert(InternalIsIteratorOwner(Pos));

        if (Pos == End())
        {
            for (TInput It = InBegin; It != InEnd; It++)
            {
                EmplaceBack(*It);
            }

            return End() - 1;
        }

        const SizeType RangeSize = InternalDistance(InBegin, InEnd);
        const SizeType NewSize   = mSize + RangeSize;
        const SizeType Index     = InternalIndex(Pos);

        T* RangeBegin = mArray + Index;
        if (NewSize >= mCapacity)
        {
            const SizeType NewCapacity = InternalGetResizeFactor(NewSize);
            InternalEmplaceRealloc(NewCapacity, RangeBegin, RangeSize);
            RangeBegin = mArray + Index;
        }
        else
        {
            // Construct the range so that we can move to it
            T* DataEnd    = mArray + mSize;
            T* NewDataEnd = mArray + mSize + RangeSize;
            T* RangeEnd   = RangeBegin + RangeSize;
            InternalDefaultConstructRange(DataEnd, NewDataEnd);
            InternalMemmoveForward(RangeBegin, DataEnd, NewDataEnd - 1);
            InternalDestructRange(RangeBegin, RangeEnd);
        }

        InternalCopyEmplace(Begin, End, RangeBegin);
        mSize = NewSize;
        return Iterator(RangeBegin);
    }

    Iterator Append(const TArray& Other)
    {
        return Insert(End(), Other.Begin(), Other.End());
    }

    void PopBack() noexcept
    {
        if (!IsEmpty())
        {
            InternalDestruct(mArray + (--mSize));
        }
    }

    Iterator Erase(Iterator Pos) noexcept
    {
        return Erase(ConstIterator(Pos));
    }

    Iterator Erase(ConstIterator Pos) noexcept
    {
        Assert(InternalIsIteratorOwner(Pos));
        Assert(Pos < End());

        if (Pos == End() - 1)
        {
            PopBack();
            return End();
        }

        const SizeType Index = InternalIndex(Pos);
        T* DataBegin = mArray + Index;
        T* DataEnd   = mArray + mSize;
        InternalMemmoveBackwards(DataBegin + 1, DataEnd, DataBegin);
        InternalDestruct(DataEnd - 1);

        mSize--;
        return Iterator(DataBegin);
    }

    Iterator Erase(Iterator Begin, Iterator End) noexcept
    {
        return Erase(ConstIterator(Begin), ConstIterator(End));
    }

    Iterator Erase(ConstIterator InBegin, ConstIterator InEnd) noexcept
    {
        Assert(InBegin < InEnd);
        Assert(InternalIsRangeOwner(InBegin, InEnd));

        T* DataBegin = mArray + InternalIndex(InBegin);
        T* DataEnd   = mArray + InternalIndex(InEnd);

        const SizeType elementCount = InternalDistance(DataBegin, DataEnd);
        if (InEnd == End())
        {
            InternalDestructRange(DataBegin, DataEnd);
        }
        else
        {
            T* RealEnd = mArray + mSize;
            InternalMemmoveBackwards(DataEnd, RealEnd, DataBegin);
            InternalDestructRange(RealEnd - elementCount, RealEnd);
        }

        mSize -= elementCount;
        return Iterator(DataBegin);
    }

    void Swap(TArray& Other) noexcept
    {
        TArray TempArray(::Move(*this));
        *this = ::Move(Other);
        Other = ::Move(TempArray);
    }

    void ShrinkToFit() noexcept
    {
        if (mCapacity > mSize)
        {
            InternalRealloc(mSize);
        }
    }

    bool IsEmpty() const noexcept { return (mSize == 0); }

    T& Front() noexcept
    {
        Assert(!IsEmpty());
        return mArray[0];
    }

    Iterator Begin() noexcept { return Iterator(mArray); }
    Iterator End() noexcept { return Iterator(mArray + mSize); }

    ConstIterator Begin() const noexcept { return Iterator(mArray); }
    ConstIterator End() const noexcept { return Iterator(mArray + mSize); }

    const T& Front() const noexcept
    {
        Assert(mSize > 0);
        return mArray[0];
    }

    T& Back() noexcept
    {
        Assert(mSize > 0);
        return mArray[mSize - 1];
    }

    const T& Back() const noexcept
    {
        Assert(mSize > 0);
        return mArray[mSize - 1];
    }

    T* Data() noexcept { return mArray; }
    const T* Data() const noexcept { return mArray; }

    SizeType LastIndex() const noexcept { return mSize > 0 ? mSize - 1 : 0; }
    SizeType Size() const noexcept { return mSize; }
    SizeType SizeInBytes() const noexcept { return mSize * sizeof(T); }

    SizeType Capacity() const noexcept { return mCapacity; }
    SizeType CapacityInBytes() const noexcept { return mCapacity * sizeof(T); }

    T& At(SizeType Index) noexcept
    {
        Assert(Index < mSize);
        return mArray[Index];
    }

    const T& At(SizeType Index) const noexcept
    {
        Assert(Index < mSize);
        return mArray[Index];
    }

    TArray& operator=(const TArray& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Clear();
            InternalConstruct(Other.Begin(), Other.End());
        }

        return *this;
    }

    TArray& operator=(TArray&& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Clear();
            InternalMove(::Forward<TArray>(Other));
        }

        return *this;
    }

    TArray& operator=(std::initializer_list<T> List) noexcept
    {
        Assign(List);
        return *this;
    }

    T& operator[](SizeType Index) noexcept { return At(Index); }
    const T& operator[](SizeType Index) const noexcept { return At(Index); }

    // STL iterator functions - Enables Range-based for-loops
public:
    Iterator begin() noexcept { return mArray; }
    Iterator end() noexcept { return mArray + mSize; }

    ConstIterator begin() const noexcept { return mArray; }
    ConstIterator end() const noexcept { return mArray + mSize; }

    ConstIterator cbegin() const noexcept { return mArray; }
    ConstIterator cend() const noexcept { return mArray + mSize; }

    ReverseIterator rbegin() noexcept { return ReverseIterator(end()); }
    ReverseIterator rend() noexcept { return ReverseIterator(begin()); }

    ConstReverseIterator rbegin() const noexcept { return ConstReverseIterator(end()); }
    ConstReverseIterator rend() const noexcept { return ConstReverseIterator(begin()); }

    ConstReverseIterator crbegin() const noexcept { return ConstReverseIterator(end()); }
    ConstReverseIterator crend() const noexcept { return ConstReverseIterator(begin()); }

private:
    // Check that the iterator belongs to this TArray
    bool InternalIsRangeOwner(ConstIterator InBegin, ConstIterator InEnd) const noexcept
    {
        return (InBegin < InEnd) && (InBegin >= Begin()) && (InEnd <= End());
    }

    bool InternalIsIteratorOwner(ConstIterator It) const noexcept
    {
        return (It >= Begin()) && (It <= End());
    }

    // Helpers
    template<typename TInput>
    const T* InternalUnwrapConst(TInput It) noexcept
    {
        if constexpr (std::is_pointer<TInput>())
        {
            return It;
        }
        else
        {
            return It.Ptr;
        }
    }

    template<typename TInput>
    SizeType InternalDistance(TInput InBegin, TInput InEnd) noexcept
    {
        constexpr bool IsPointer        = std::is_pointer<TInput>();
        constexpr bool IsCustomIterator = std::is_same<TInput, Iterator>() || std::is_same<TInput, ConstIterator>();

        // Handle outside pointers
        if constexpr (IsPointer || IsCustomIterator)
        {
            return static_cast<SizeType>(InternalUnwrapConst(InEnd) - InternalUnwrapConst(InBegin));
        }
        else
        {
            return static_cast<SizeType>(std::distance(InBegin, InEnd));
        }
    }

    template<typename TInput>
    SizeType InternalIndex(TInput Pos) noexcept
    {
        return static_cast<SizeType>(InternalUnwrapConst(Pos) - InternalUnwrapConst(begin()));
    }

    SizeType InternalGetResizeFactor() const noexcept
    {
        return InternalGetResizeFactor(mSize);
    }

    SizeType InternalGetResizeFactor(SizeType BaseSize) const noexcept
    {
        return BaseSize + (mCapacity / 2) + 1;
    }

    T* InternalAllocateElements(SizeType Capacity) noexcept
    {
        const SizeType SizeInBytes = sizeof(T) * Capacity;
        return reinterpret_cast<T*>(mAllocator.Allocate(SizeInBytes));
    }

    void InternalReleaseData() noexcept
    {
        if (mArray)
        {
            mAllocator.Free(mArray);
            mArray = nullptr;
        }
    }

    void InternalAllocData(SizeType Capacity) noexcept
    {
        if (Capacity > mCapacity)
        {
            InternalReleaseData();
            mArray    = InternalAllocateElements(Capacity);
            mCapacity = Capacity;
        }
    }

    void InternalRealloc(SizeType Capacity) noexcept
    {
        T* TempData = InternalAllocateElements(Capacity);
        InternalMoveEmplace(mArray, mArray + mSize, TempData);
        InternalDestructRange(mArray, mArray + mSize);

        InternalReleaseData();
        mArray    = TempData;
        mCapacity = Capacity;
    }

    void InternalEmplaceRealloc(SizeType Capacity, T* EmplacePos, SizeType Count) noexcept
    {
        Assert(Capacity >= mSize + Count);

        const SizeType Index = InternalIndex(EmplacePos);
        T* TempData = InternalAllocateElements(Capacity);
        InternalMoveEmplace(mArray, EmplacePos, TempData);
        if (EmplacePos != mArray + mSize)
        {
            InternalMoveEmplace(EmplacePos, mArray + mSize, TempData + Index + Count);
        }

        InternalDestructRange(mArray, mArray + mSize);

        InternalReleaseData();
        mArray    = TempData;
        mCapacity = Capacity;
    }

    // Construct
    void InternalConstruct(SizeType InSize) noexcept
    {
        if (InSize > 0)
        {
            InternalAllocData(InSize);
            mSize = InSize;
            InternalDefaultConstructRange(mArray, mArray + InSize);
        }
    }

    void InternalConstruct(SizeType InSize, const T& Value) noexcept
    {
        if (InSize > 0)
        {
            InternalAllocData(InSize);
            InternalCopyEmplace(InSize, Value, mArray);
            mSize = InSize;
        }
    }

    template<typename TInput>
    void InternalConstruct(TInput InBegin, TInput InEnd) noexcept
    {
        const SizeType Distance = InternalDistance(InBegin, InEnd);
        if (Distance > 0)
        {
            InternalAllocData(Distance);
            InternalCopyEmplace(InBegin, InEnd, mArray);
            mSize = Distance;
        }
    }

    void InternalMove(TArray&& Other) noexcept
    {
        InternalReleaseData();

        mArray    = Other.mArray;
        mSize     = Other.mSize;
        mCapacity = Other.mCapacity;

        Other.mArray    = nullptr;
        Other.mSize     = 0;
        Other.mCapacity = 0;
    }

    // Emplace
    template<typename TInput>
    void InternalCopyEmplace(TInput Begin, TInput End, T* Dest) noexcept
    {
        // This function assumes that there is no overlap
        constexpr bool IsTrivial        = std::is_trivially_copy_constructible<T>();
        constexpr bool IsPointer        = std::is_pointer<TInput>();
        constexpr bool IsCustomIterator = std::is_same<TInput, Iterator>() || std::is_same<TInput, ConstIterator>();

        if constexpr (IsTrivial && (IsPointer || IsCustomIterator))
        {
            const SizeType Count   = InternalDistance(Begin, End);
            const SizeType CpySize = Count * sizeof(T);
            memcpy(Dest, InternalUnwrapConst(Begin), CpySize);
        }
        else
        {
            while (Begin != End)
            {
                new(reinterpret_cast<void*>(Dest)) T(*Begin);
                Begin++;
                Dest++;
            }
        }
    }

    void InternalCopyEmplace(SizeType Size, const T& Value, T* Dest) noexcept
    {
        T* ItEnd = Dest + Size;
        while (Dest != ItEnd)
        {
            new(reinterpret_cast<void*>(Dest)) T(Value);
            Dest++;
        }
    }

    void InternalMoveEmplace(T* InBegin, T* InEnd, T* Dest) noexcept
    {
        // This function assumes that there is no overlap
        if constexpr (std::is_trivially_move_constructible<T>())
        {
            const SizeType Count   = InternalDistance(InBegin, InEnd);
            const SizeType CpySize = Count * sizeof(T);
            ::memcpy(Dest, InBegin, CpySize);
        }
        else
        {
            while (InBegin != InEnd)
            {
                new(reinterpret_cast<void*>(Dest)) T(::Move(*InBegin));
                InBegin++;
                Dest++;
            }
        }
    }

    void InternalMemmoveBackwards(T* InBegin, T* InEnd, T* Dest) noexcept
    {
        Assert(InBegin <= InEnd);
        if (InBegin == InEnd)
        {
            return;
        }

        Assert(InEnd <= mArray + mCapacity);

        // ::Move each object in the range to the destination
        const SizeType Count = InternalDistance(InBegin, InEnd);
        if constexpr (std::is_trivially_move_assignable<T>())
        {
            const SizeType CpySize = Count * sizeof(T);
            ::memmove(Dest, InBegin, CpySize); // Assumes that data can overlap
        }
        else
        {
            while (InBegin != InEnd)
            {
                if constexpr (std::is_move_assignable<T>())
                {
                    (*Dest) = ::Move(*InBegin);
                }
                else if constexpr (std::is_copy_assignable<T>())
                {
                    (*Dest) = (*InBegin);
                }

                Dest++;
                InBegin++;
            }
        }
    }

    void InternalMemmoveForward(T* InBegin, T* InEnd, T* Dest) noexcept
    {
        // Move each object in the range to the destination, starts in the "End" and moves forward

        const SizeType Count = InternalDistance(InBegin, InEnd);
        if constexpr (std::is_trivially_move_assignable<T>())
        {
            if (Count > 0)
            {
                const SizeType CpySize    = Count * sizeof(T);
                const SizeType OffsetSize = (Count - 1) * sizeof(T);
                ::memmove(reinterpret_cast<char*>(Dest) - OffsetSize, InBegin, CpySize);
            }
        }
        else
        {
            while (InEnd != InBegin)
            {
                InEnd--;
                if constexpr (std::is_move_assignable<T>())
                {
                    (*Dest) = ::Move(*InEnd);
                }
                else if constexpr (std::is_copy_assignable<T>())
                {
                    (*Dest) = (*InEnd);
                }
                Dest--;
            }
        }
    }

    void InternalDestruct(const T* Pos) noexcept
    {
        if constexpr (std::is_trivially_destructible<T>() == false)
        {
            (*Pos).~T();
        }
    }

    void InternalDestructRange(const T* InBegin, const T* InEnd) noexcept
    {
        Assert(InBegin <= InEnd);
        Assert(InEnd - InBegin <= mCapacity);

        if constexpr (std::is_trivially_destructible<T>() == false)
        {
            while (InBegin != InEnd)
            {
                InternalDestruct(InBegin);
                InBegin++;
            }
        }
    }

    void InternalDefaultConstructRange(T* InBegin, T* InEnd) noexcept
    {
        Assert(InBegin <= InEnd);

        if constexpr (std::is_default_constructible<T>())
        {
            while (InBegin != InEnd)
            {
                new(reinterpret_cast<void*>(InBegin)) T();
                InBegin++;
            }
        }
    }

private:
    T*         mArray;
    SizeType   mSize;
    SizeType   mCapacity;
    TAllocator mAllocator;
};
