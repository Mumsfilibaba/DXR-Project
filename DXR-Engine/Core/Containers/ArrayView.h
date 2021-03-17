#pragma once
#include "Utilities.h"
#include "Iterator.h"

// TArrayView - View of an array similar to std::span

template<typename T>
class TArrayView
{
public:
    typedef T*                        Iterator;
    typedef const T*                  ConstIterator;
    typedef TReverseIterator<T>       ReverseIterator;
    typedef TReverseIterator<const T> ConstReverseIterator;
    typedef uint32                    SizeType;

    TArrayView() noexcept
        : mView(nullptr)
        , mSize(0)
    {
    }

    template<typename TArrayType>
    explicit TArrayView(TArrayType& Array) noexcept
        : mView(Array.Data())
        , mSize(Array.Size())
    {
    }
    
    template<const SizeType N>
    explicit TArrayView(T(&Array)[N]) noexcept
        : mView(Array)
        , mSize(N)
    {
    }

    template<typename TInputIterator>
    explicit TArrayView(TInputIterator Begin, TInputIterator End) noexcept
        : mView(Begin)
        , mSize(SizeType(End - Begin))
    {
    }

    TArrayView(const TArrayView& Other) noexcept
        : mView(Other.mView)
        , mSize(Other.mSize)
    {
    }

    TArrayView(TArrayView&& Other) noexcept
        : mView(Other.mView)
        , mSize(Other.mSize)
    {
        Other.mView = nullptr;
        Other.mSize = 0;
    }

    bool IsEmpty() const noexcept { return (mSize == 0); }

    T& Front() noexcept { return mView[0]; }
    const T& Front() const noexcept { return mView[0]; }

    T& Back() noexcept { return mView[mSize - 1]; }
    const T& Back() const noexcept { return mView[mSize - 1]; }

    T& At(SizeType Index) noexcept
    {
        Assert(Index < mSize);
        return mView[Index];
    }

    const T& At(SizeType Index) const noexcept
    {
        Assert(Index < mSize);
        return mView[Index];
    }

    void Swap(TArrayView& Other) noexcept
    {
        TArrayView TempView(::Move(*this));
        *this = ::Move(Other);
        Other = ::Move(TempView);
    }
    
    Iterator Begin() noexcept { return Iterator(mView); }
    Iterator End() noexcept { return Iterator(mView + mSize); }

    ConstIterator Begin() const noexcept { return Iterator(mView); }
    ConstIterator End() const noexcept { return Iterator(mView + mSize); }

    SizeType LastIndex() const noexcept { return mSize > 0 ? mSize - 1 : 0; }
    SizeType Size() const noexcept { return mSize; }
    SizeType SizeInBytes() const noexcept { return mSize * sizeof(T); }

    T* Data() noexcept { return mView; }
    const T* Data() const noexcept { return mView; }

    T& operator[](SizeType Index) noexcept { return At(Index); }
    const T& operator[](SizeType Index) const noexcept { return At(Index); }

    TArrayView& operator=(const TArrayView& Other) noexcept
    {
        mView = Other.mView;
        mSize = Other.mSize;
        return *this;
    }

    TArrayView& operator=(TArrayView&& Other) noexcept
    {
        if (this != &Other)
        {
            mView = Other.mView;
            mSize = Other.mSize;
            Other.mView = nullptr;
            Other.mSize = 0;
        }

        return *this;
    }

    // STL iterator functions - Enables Range-based for-loops
public:
    Iterator begin() noexcept { return mView; }
    Iterator end() noexcept { return mView + mSize; }

    ConstIterator begin() const noexcept { return mView; }
    ConstIterator end() const noexcept { return mView + mSize; }

    ConstIterator cbegin() const noexcept { return mView; }
    ConstIterator cend() const noexcept { return mView + mSize; }

    ReverseIterator rbegin() noexcept { return ReverseIterator(end()); }
    ReverseIterator rend() noexcept { return ReverseIterator(begin()); }

    ConstReverseIterator rbegin() const noexcept { return ConstReverseIterator(end()); }
    ConstReverseIterator rend() const noexcept { return ConstReverseIterator(begin()); }

    ConstReverseIterator crbegin() const noexcept { return ConstReverseIterator(end()); }
    ConstReverseIterator crend() const noexcept { return ConstReverseIterator(begin()); }

private:
    T*       mView;
    SizeType mSize;
};
