#pragma once
#include "UnderlyingType.h"

#define ENUM_OPERATORS(EnumType)                                                                           \
inline constexpr EnumType operator|(EnumType Lhs, EnumType Rhs) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)Lhs) | ((TUnderlyingType<EnumType>::Type)Rhs));      \
}                                                                                                          \
inline EnumType& operator|=(EnumType& Lhs, EnumType Rhs) noexcept                                          \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)Lhs) |= ((TUnderlyingType<EnumType>::Type)Rhs)); \
}                                                                                                          \
inline constexpr EnumType operator&(EnumType Lhs, EnumType Rhs) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)Lhs) & ((TUnderlyingType<EnumType>::Type)Rhs));      \
}                                                                                                          \
inline EnumType& operator&=(EnumType& Lhs, EnumType Rhs) noexcept                                          \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)Lhs) &= ((TUnderlyingType<EnumType>::Type)Rhs)); \
}                                                                                                          \
inline constexpr EnumType operator~(EnumType Lhs) noexcept                                                 \
{                                                                                                          \
    return EnumType(~((TUnderlyingType<EnumType>::Type)Lhs));                                              \
}                                                                                                          \
inline constexpr EnumType operator^(EnumType Lhs, EnumType Rhs) noexcept                                   \
{                                                                                                          \
    return EnumType(((TUnderlyingType<EnumType>::Type)Lhs) ^ ((TUnderlyingType<EnumType>::Type)Rhs));      \
}                                                                                                          \
inline EnumType& operator^=(EnumType&Lhs, EnumType Rhs) noexcept                                           \
{                                                                                                          \
    return (EnumType&)(((TUnderlyingType<EnumType>::Type&)Lhs) ^= ((TUnderlyingType<EnumType>::Type)Rhs)); \
}