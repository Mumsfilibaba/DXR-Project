#pragma once
#include "Core/CoreDefines.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/Move.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TDefaultDelete

template<typename T>
struct TDefaultDelete
{
    using ElementType = typename TRemoveExtent<T>::Type;

    TDefaultDelete() = default;
    TDefaultDelete(const TDefaultDelete&) = default;
    TDefaultDelete(TDefaultDelete&&) = default;
    ~TDefaultDelete() = default;

    TDefaultDelete& operator=(const TDefaultDelete&) = default;
    TDefaultDelete& operator=(TDefaultDelete&&) = default;

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete(const TDefaultDelete<U>&) noexcept
    { }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete(TDefaultDelete<U>&&) noexcept
    { }

    FORCEINLINE void DeleteElement(ElementType* Pointer) noexcept
    {
        delete Pointer;
    }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete& operator=(const TDefaultDelete<U>&) noexcept
    {
        return *this;
    }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete& operator=(TDefaultDelete<U>&&) noexcept
    {
        return *this;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TDefaultDelete

template<typename T>
struct TDefaultDelete<T[]>
{
    typedef typename TRemoveExtent<T>::Type ElementType;

    TDefaultDelete() = default;
    TDefaultDelete(const TDefaultDelete&) = default;
    TDefaultDelete(TDefaultDelete&&) = default;
    ~TDefaultDelete() = default;
    TDefaultDelete& operator=(const TDefaultDelete&) = default;
    TDefaultDelete& operator=(TDefaultDelete&&) = default;

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete(const TDefaultDelete<U>&) noexcept
    { }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete(TDefaultDelete<U>&&) noexcept
    { }

    FORCEINLINE void DeleteElement(ElementType* Pointer) noexcept
    {
        delete[] Pointer;
    }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete& operator=(const TDefaultDelete<U>&) noexcept
    {
        return *this;
    }

    template<typename U, typename = TEnableIf<TIsPointerConvertible<U, T>::Value>>
    FORCEINLINE TDefaultDelete& operator=(TDefaultDelete<U>&&) noexcept
    {
        return *this;
    }
};
