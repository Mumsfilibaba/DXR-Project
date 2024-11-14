#pragma once
#include "Core/Templates/TypeTraits/AddTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/MetaProgrammingFuncs.h"

template<typename T>
struct TIsCopyConstructable
{
    inline static constexpr bool Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsCopyAssignable
{
    inline static constexpr bool Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsMoveConstructable
{
    inline static constexpr bool Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsMoveAssignable
{
    inline static constexpr bool Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value;
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
    inline static constexpr bool Value = TIsNothrowConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowMoveAssignable
{
    inline static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyConstructable
{
    inline static constexpr bool Value = TIsNothrowConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyAssignable
{
    inline static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddLValueReference<const T>::Type>::Value;
};