#pragma once
#include "Core/CoreTypes.h"

template <typename T>
struct TVoid
{
    typedef void_type Type;
};

template<typename T>
struct TIdentity
{
    typedef T Type;
};

template<typename T>
struct TUnderlyingType
{
    typedef typename TIdentity<__underlying_type(T)>::Type Type;
};

template<typename T, T InValue>
struct TIntegralConstant
{
    inline static constexpr T Value = InValue;

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
    inline static constexpr TSIZE Value = alignof(T);
};

template<typename T>
struct TIsEmpty
{
    inline static constexpr bool Value = __is_empty(T);
};

template<typename T>
struct TIsEnum
{
    inline static constexpr bool Value = __is_enum(T);
};

template<typename T>
struct TIsUnion
{
    inline static constexpr bool Value = __is_union(T);
};

template<typename T>
struct TIsFinal
{
    inline static constexpr bool Value = __is_final(T);
};

template<typename T>
struct TIsPOD
{
    inline static constexpr bool Value = __is_pod(T);
};

template<typename T>
struct TIsPolymorphic
{
    inline static constexpr bool Value = __is_polymorphic(T);
};

template<typename T, typename F>
struct TIsAssignable
{
    inline static constexpr bool Value = __is_assignable(T, F);
};

template<typename B, typename D>
struct TIsBaseOf
{
    inline static constexpr bool Value = __is_base_of(B, D);
};

template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    inline static constexpr bool Value = __is_constructible(T, ArgTypes...);
};

template<typename F, typename T>
struct TIsConvertible
{
    inline static constexpr bool Value = __is_convertible_to(F, T);
};

template<typename T>
struct TIsTriviallyCopyable
{
    inline static constexpr bool Value = __is_trivially_copyable(T);
};

template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    inline static constexpr bool Value = __is_trivially_constructible(T, ArgTypes...);
};

template<typename T>
struct TIsTriviallyDestructable
{
    inline static constexpr bool Value = __is_trivially_destructible(T);
};
