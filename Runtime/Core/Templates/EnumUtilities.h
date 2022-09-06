#pragma once
#include "UnderlyingType.h"
#include "EnableIf.h"
#include "IsEnum.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ENUM_CLASS_OPERATORS

#define ENUM_CLASS_OPERATORS(EnumType)                                                                         \
    CONSTEXPR EnumType operator|(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) | ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
    inline EnumType& operator|=(EnumType& LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) |= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
    CONSTEXPR EnumType operator&(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) & ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
    inline EnumType& operator&=(EnumType& LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) &= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
    CONSTEXPR EnumType operator~(EnumType LHS) noexcept                                                        \
    {                                                                                                          \
        return EnumType(~((TUnderlyingType<EnumType>::Type)LHS));                                              \
    }                                                                                                          \
    CONSTEXPR EnumType operator^(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) ^ ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
    inline EnumType& operator^=(EnumType&LHS, EnumType RHS) noexcept                                           \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) ^= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
    CONSTEXPR bool IsEnumFlagSet(EnumType EnumMask, EnumType EnumFlag) noexcept                                \
    {                                                                                                          \
        return (ToUnderlying((EnumMask) & (EnumFlag)) != 0);                                                   \
    }

template<typename EnumType>
CONSTEXPR typename TEnableIf<TIsEnum<EnumType>::Value, EnumType>::Type EnumAdd(EnumType Value, typename TUnderlyingType<EnumType>::Type Offset) noexcept
{
    return static_cast<EnumType>(ToUnderlying(Value) + Offset);
}

template<typename EnumType>
CONSTEXPR typename TEnableIf<TIsEnum<EnumType>::Value, EnumType>::Type EnumSub(EnumType Value, typename TUnderlyingType<EnumType>::Type Offset) noexcept
{
    return static_cast<EnumType>(ToUnderlying(Value) - Offset);
}