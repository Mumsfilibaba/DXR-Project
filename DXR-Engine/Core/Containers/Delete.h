#pragma once
#include "CoreDefines.h"

#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/IsConvertible.h"

/* TDefaultDelete for scalar types */
template<typename T>
struct TDefaultDelete
{
    typedef typename TRemoveExtent<T>::Type ElementType;

    TDefaultDelete() = default;
    TDefaultDelete( const TDefaultDelete& ) = default;
    TDefaultDelete& operator=( const TDefaultDelete& ) = default;
    ~TDefaultDelete() = default;

    template<typename U, typename = TEnableIf<TIsConvertible<U*, T*>::Value>>
    FORCEINLINE TDefaultDelete( const TDefaultDelete<U>& ) noexcept
    {
    }

    FORCEINLINE void DeleteElement( ElementType* Pointer ) noexcept
    {
        delete[] Pointer;
    }

    template<typename U, typename = TEnableIf<TIsConvertible<U*, T*>::Value>>
    FORCEINLINE TDefaultDelete& operator=( const TDefaultDelete<U>& ) noexcept
    {
        return *this;
    }
};

/* TDefaultDelete for array types */
template<typename T>
struct TDefaultDelete<T[]>
{
    typedef typename TRemoveExtent<T>::Type ElementType;

    TDefaultDelete() = default;
    TDefaultDelete( const TDefaultDelete& ) = default;
    TDefaultDelete& operator=( const TDefaultDelete& ) = default;
    ~TDefaultDelete() = default;

    template<typename U, typename = TEnableIf<TIsConvertible<U*, T*>::Value>>
    FORCEINLINE TDefaultDelete( const TDefaultDelete<U[]>& ) noexcept
    {
    }

    FORCEINLINE void DeleteElement( ElementType* Pointer ) noexcept
    {
        delete[] Pointer;
    }

    template<typename U, typename = TEnableIf<TIsConvertible<U*, T*>::Value>>
    FORCEINLINE TDefaultDelete& operator=( const TDefaultDelete<U[]>& ) noexcept
    {
        return *this;
    }
};