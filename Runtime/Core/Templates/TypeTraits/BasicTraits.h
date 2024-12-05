#pragma once
#include "Core/CoreTypes.h"

template <typename T>
struct TVoid
{
    typedef VOID_TYPE Type;
};

template<typename T>
struct TTypeIdentity
{
    typedef T Type;
};

template<typename T>
struct TUnderlyingType
{
    typedef typename TTypeIdentity<__underlying_type(T)>::Type Type;
};

template<typename T, T InValue>
struct TIntegralConstant
{
    static constexpr T Value = InValue;

    typedef T ValueType;
    typedef TIntegralConstant<T, InValue> Type;

    constexpr operator ValueType() const noexcept
    {
        return Value;
    }
};

template<typename T>
struct TAlignmentOf
{
    static constexpr SIZE_T Value = alignof(T);
};

template<typename T>
struct TIsEmpty
{
    static constexpr bool Value = __is_empty(T);
};

template<typename T>
struct TIsEnum
{
    static constexpr bool Value = __is_enum(T);
};

template<typename T>
struct TIsScopedEnum
{
    static constexpr bool Value = __is_scoped_enum(T);
};

template<typename T>
struct TIsUnion
{
    static constexpr bool Value = __is_union(T);
};

template<typename T>
struct TIsFinal
{
    static constexpr bool Value = __is_final(T);
};

template<typename T>
struct TIsPOD
{
    static constexpr bool Value = __is_pod(T);
};

template<typename T>
struct TIsPolymorphic
{
    static constexpr bool Value = __is_polymorphic(T);
};

template<typename T, typename F>
struct TIsAssignable
{
    static constexpr bool Value = __is_assignable(T, F);
};

template<typename B, typename D>
struct TIsBaseOf
{
    static constexpr bool Value = __is_base_of(B, D);
};

template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    static constexpr bool Value = __is_constructible(T, ArgTypes...);
};

template<typename T>
struct TIsDestructible
{
    static constexpr bool Value = __is_destructible(T);
};

template<typename T>
struct THasVirtualDestructor
{
    static constexpr bool Value = __has_virtual_destructor(T);
};

template<typename F, typename T>
struct TIsConvertible
{
    static constexpr bool Value = __is_convertible_to(F, T);
};

template<typename T>
struct TIsTriviallyCopyable
{
    static constexpr bool Value = __is_trivially_copyable(T);
};

template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    static constexpr bool Value = __is_trivially_constructible(T, ArgTypes...);
};

template<typename T>
struct TIsTriviallyDestructable
{
    static constexpr bool Value = __is_trivially_destructible(T);
};

template<typename T>
struct TIsStandardLayout
{
    static constexpr bool Value = __is_standard_layout(T);
};
