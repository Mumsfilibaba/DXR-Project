#pragma once
#include "Core/Templates/TypeTraits/AddTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/MetaProgrammingFuncs.h"

template<typename T>
struct TIsCopyConstructable
{
    static constexpr bool Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsCopyAssignable
{
    static constexpr bool Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsMoveConstructable
{
    static constexpr bool Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsMoveAssignable
{
    static constexpr bool Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T, typename... Args>
struct TIsNothrowConstructible
{
    static constexpr bool Value = TIsConstructible<T, Args...>::Value && noexcept(T(DeclVal<Args>()...));
};

template<typename T, typename U>
struct TIsNothrowAssignable
{
    static constexpr bool Value = TIsAssignable<T, U>::Value && noexcept(DeclVal<T>() = DeclVal<U>());
};

template<typename T>
struct TIsNothrowMoveConstructable
{
    static constexpr bool Value = TIsNothrowConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowMoveAssignable
{
    static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyConstructable
{
    static constexpr bool Value = TIsNothrowConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyAssignable
{
    static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddLValueReference<const T>::Type>::Value;
};