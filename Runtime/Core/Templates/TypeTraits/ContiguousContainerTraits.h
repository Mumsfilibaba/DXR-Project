#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

#include <initializer_list>

template<typename T>
struct TIsContiguousContainer
{
    static constexpr bool Value = TIsBoundedArray<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&>
{
    static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&&>
{
    static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const T>
{
    static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<volatile T>
{
    static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const volatile T>
{
    static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T>
struct TIsContiguousContainer<std::initializer_list<T>> : TTrueType { };