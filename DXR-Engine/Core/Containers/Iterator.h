#pragma once
#include "Core/Types.h"

template<typename TIteratorType>
class TReverseIterator
{
public:
    TReverseIterator() noexcept
        : mIterator(nullptr)
    {
    }

    TReverseIterator(TIteratorType* Itertor) noexcept
        : mIterator(Itertor)
    {
    }

    TReverseIterator(const TReverseIterator& Other) noexcept
        : mIterator(Other.Raw())
    {
    }

    template<typename U>
    TReverseIterator(const TReverseIterator<U>& Other) noexcept
        : mIterator(Other.Raw())
    {
    }

    ~TReverseIterator() = default;

    TIteratorType* Raw() const { return mIterator; }

    TIteratorType* operator->() const noexcept { return (mIterator - 1); }
    TIteratorType& operator*() const noexcept { return *(mIterator - 1); }
    
    TReverseIterator operator++() noexcept 
    {
        mIterator--;
        return *this;
    }

    TReverseIterator operator++(int32) noexcept
    {
        TReverseIterator Temp = *this;
        mIterator--;
        return Temp;
    }

    TReverseIterator operator--() noexcept
    {
        mIterator++;
        return *this;
    }

    TReverseIterator operator--(int32) noexcept
    {
        TReverseIterator Temp = *this;
        mIterator++;
        return Temp;
    }

    TReverseIterator operator+(int32 Offset) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp += Offset;
    }

    TReverseIterator operator-(int32 Offset) const noexcept
    {
        TReverseIterator Temp = *this;
        return Temp -= Offset;
    }

    TReverseIterator& operator+=(int32 Offset) noexcept
    {
        mIterator -= Offset;
        return *this;
    }

    TReverseIterator& operator-=(int32 Offset) noexcept
    {
        mIterator += Offset;
        return *this;
    }

    bool operator==(const TReverseIterator& Other) const noexcept { return (mIterator == Other.mIterator); }
    bool operator!=(const TReverseIterator& Other) const noexcept { return (mIterator != Other.mIterator); }

    operator TReverseIterator<const TIteratorType>() const noexcept { return TReverseIterator<const TIteratorType>(mIterator); }

private:
    TIteratorType* mIterator;
};