#pragma once
#include "Core/CoreDefines.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

template<typename InElementType>
struct TDefaultDelete
{
    using ElementType = InElementType;

    TDefaultDelete()                      = default;
    TDefaultDelete(const TDefaultDelete&) = default;
    TDefaultDelete(TDefaultDelete&&)      = default;
    ~TDefaultDelete()                     = default;

    TDefaultDelete& operator=(const TDefaultDelete&) = default;
    TDefaultDelete& operator=(TDefaultDelete&&)      = default;

    template<typename OtherType>
    FORCEINLINE TDefaultDelete(const TDefaultDelete<OtherType>&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete(TDefaultDelete<OtherType>&&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
    }

    FORCEINLINE void Call(ElementType* Pointer) noexcept
    {
        delete Pointer;
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete& operator=(const TDefaultDelete<OtherType>&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete& operator=(TDefaultDelete<OtherType>&&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        return *this;
    }
};

template<typename InElementType>
struct TDefaultDelete<InElementType[]>
{
    typedef typename TRemoveExtent<InElementType>::Type ElementType;

    TDefaultDelete()                      = default;
    TDefaultDelete(const TDefaultDelete&) = default;
    TDefaultDelete(TDefaultDelete&&)      = default;
    ~TDefaultDelete()                     = default;

    TDefaultDelete& operator=(const TDefaultDelete&) = default;
    TDefaultDelete& operator=(TDefaultDelete&&)      = default;

    template<typename OtherType>
    FORCEINLINE TDefaultDelete(const TDefaultDelete<OtherType>&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete(TDefaultDelete<OtherType>&&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
    }

    FORCEINLINE void Call(ElementType* Pointer) noexcept
    {
        delete[] Pointer;
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete& operator=(const TDefaultDelete<OtherType>&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TDefaultDelete& operator=(TDefaultDelete<OtherType>&&) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        return *this;
    }
};
