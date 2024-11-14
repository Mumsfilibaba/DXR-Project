#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

#include <initializer_list>

template<typename T>
struct TIsContiguousContainer
{
    inline static constexpr bool Value = TIsBoundedArray<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&&>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<volatile T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const volatile T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T>
struct TIsContiguousContainer<std::initializer_list<T>> : TTrueType { };