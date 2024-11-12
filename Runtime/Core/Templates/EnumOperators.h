#pragma once
#include "TypeTraits.h"

#define ENUM_CLASS_OPERATORS(EnumType) \
    constexpr EnumType operator|(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) | ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator|=(EnumType& LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) |= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr EnumType operator&(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) & ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator&=(EnumType& LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) &= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr EnumType operator~(EnumType LHS) noexcept \
    { \
        return EnumType(~((TUnderlyingType<EnumType>::Type)LHS)); \
    } \
    constexpr EnumType operator^(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) ^ ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator^=(EnumType&LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) ^= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr bool IsEnumFlagSet(EnumType EnumMask, EnumType EnumFlag) noexcept \
    { \
        return ToUnderlying((EnumMask) & (EnumFlag)) != 0; \
    }
