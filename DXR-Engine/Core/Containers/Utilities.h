#pragma once
#include "Core.h"

#include <type_traits>

/*
 * TRemoveExtent - Removes array type
 */

template<typename T>
struct _TRemoveExtent
{
    using TType = T;
};

template<typename T>
struct _TRemoveExtent<T[]>
{
    using TType = T;
};

template<typename T, size_t SIZE>
struct _TRemoveExtent<T[SIZE]>
{
    using TType = T;
};

template<typename T>
using TRemoveExtent = typename _TRemoveExtent<T>::TType;

/*
 * Move
 */

 // Move an object by converting it into a rvalue
template<typename T>
constexpr TRemoveReference<T>&& Move( T&& Arg ) noexcept
{
    return static_cast<TRemoveReference<T>&&>(Arg);
}

/*
 * Forward
 */

 // Forward an object by converting it into a rvalue from an lvalue
template<typename T>
constexpr T&& Forward( TRemoveReference<T>& Arg ) noexcept
{
    return static_cast<T&&>(Arg);
}

// Forward an object by converting it into a rvalue from an rvalue
template<typename T>
constexpr T&& Forward( TRemoveReference<T>&& Arg ) noexcept
{
    return static_cast<T&&>(Arg);
}

/*
 * TEnableIf
 */

template<bool TCondition, typename T = void>
struct _TEnableIf
{
};

template<typename T>
struct _TEnableIf<true, T>
{
    using Type = T;
};

template<bool TCondition, typename T = void>
using TEnableIf = typename _TEnableIf<TCondition, T>::Type;

/*
 * TIsArray - Check for array type
 */

template<typename T>
struct _TIsArray
{
    static constexpr bool Value = false;
};

template<typename T>
struct _TIsArray<T[]>
{
    static constexpr bool Value = true;
};

template<typename T, int32 N>
struct _TIsArray<T[N]>
{
    static constexpr bool Value = true;
};

template<typename T>
inline constexpr bool TIsArray = _TIsArray<T>::Value;