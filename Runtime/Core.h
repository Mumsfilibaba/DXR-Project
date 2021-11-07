#pragma once
#include "CoreDefines.h"
#include "CoreTypes.h"

#include "Core/Memory/New.h"

//<--
// TODO: Move to templates
// TODO: use TUnderlyingType instead
// Preparation for enum class usage
#if 0
template <int64 SIZE>
struct _EnumIntegerSize;

template <>
struct _EnumIntegerSize<1>
{
    typedef int8 Type;
};

template <>
struct _EnumIntegerSize<2>
{
    typedef int16 Type;
};

template <>
struct _EnumIntegerSize<4>
{
    typedef int32 Type;
};

template <>
struct _EnumIntegerSize<8>
{
    typedef int64 Type;
};

template<typename T>
struct _EnumIntegerType
{
    typedef typename _EnumIntegerSize<sizeof( T )>::Type Type;
};

template<typename T>
using EnumIntegerType = typename _EnumIntegerType<T>::Type;

#define IMPLEMENT_ENUM_OPERATORS(Type) \
    inline Type  operator|(Type LHS, Type RHS) noexcept { return Type(((EnumIntegerType<Type>)LHS) | ((EnumIntegerType<Type>)RHS)); } \
    inline Type  operator&(Type LHS, Type RHS) noexcept { return Type(((EnumIntegerType<Type>)LHS) & ((EnumIntegerType<Type>)RHS)); } \
    inline Type  operator^(Type LHS, Type RHS) noexcept { return Type(((EnumIntegerType<Type>)LHS) ^ ((EnumIntegerType<Type>)RHS)); } \
\
    inline Type  operator~(Type LHS) noexcept { return Type(~((EnumIntegerType<Type>)LHS)); } \
\
    inline Type& operator|=(Type& LHS, Type RHS) noexcept { return (Type&)(((EnumIntegerType<Type>&)LHS) |= ((EnumIntegerType<Type>)RHS)); } \
    inline Type& operator&=(Type& LHS, Type RHS) noexcept { return (Type&)(((EnumIntegerType<Type>&)LHS) &= ((EnumIntegerType<Type>)RHS)); } \
    inline Type& operator^=(Type& LHS, Type RHS) noexcept { return (Type&)(((EnumIntegerType<Type>&)LHS) ^= ((EnumIntegerType<Type>)RHS)); }

#endif
//-->
