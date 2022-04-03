#pragma once
#include "UnderlyingType.h"

#define ENUM_OPERATORS(EnumType)                                                                           \
inline constexpr EnumType operator|(EnumType LHS, EnumType RHS) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)LHS) | ((TUnderlyingType<EnumType>::Type)RHS));      \
}                                                                                                          \
inline EnumType& operator|=(EnumType& LHS, EnumType RHS) noexcept                                          \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) |= ((TUnderlyingType<EnumType>::Type)RHS)); \
}                                                                                                          \
inline constexpr EnumType operator&(EnumType LHS, EnumType RHS) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)LHS) & ((TUnderlyingType<EnumType>::Type)RHS));      \
}                                                                                                          \
inline EnumType& operator&=(EnumType& LHS, EnumType RHS) noexcept                                          \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) &= ((TUnderlyingType<EnumType>::Type)RHS)); \
}                                                                                                          \
inline constexpr EnumType operator~(EnumType LHS) noexcept                                                 \
{                                                                                                          \
    return EnumType(~((TUnderlyingType<EnumType>::Type)LHS));                                              \
}                                                                                                          \
inline constexpr EnumType operator^(EnumType LHS, EnumType RHS) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)LHS) ^ ((TUnderlyingType<EnumType>::Type)RHS));      \
}                                                                                                          \
inline EnumType& operator^=(EnumType&LHS, EnumType RHS) noexcept                                           \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) ^= ((TUnderlyingType<EnumType>::Type)RHS)); \
}