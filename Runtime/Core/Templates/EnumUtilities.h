#pragma once
#include "UnderlyingType.h"

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
    }