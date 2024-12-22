#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/EqualTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

template<typename T>
struct TIsNullptr
{
    static constexpr bool Value = TIsSame<NULLPTR_TYPE, typename TRemoveCV<T>::Type>::Value;
};

template<typename T>
struct TIsPointer : TFalseType { };

template<typename T>
struct TIsPointer<T*> : TTrueType { };

template <typename T>
struct TIsMemberPointer : TFalseType { };

template <typename T, typename U>
struct TIsMemberPointer<T U::*> : TTrueType { };

template <typename T>
struct TIsMemberPointer<const T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<volatile T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<const volatile T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template<typename T>
struct TIsNullable
{
    static constexpr bool Value = TOr<TIsPointer<T>, TIsMemberPointer<T>>::Value;
};

template<typename F, typename T>
struct TIsPointerConvertible
{
    static constexpr bool Value = TIsConvertible<typename TAddPointer<F>::Type, typename TAddPointer<T>::Type>::Value;
};