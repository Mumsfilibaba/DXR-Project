#pragma once
#include "IsArray.h"

#include <initializer_list>

template<typename T>
struct TIsContiguousContainer
{
    enum { Value = TIsBoundedArray<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<T&>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<T&&>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<const T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<volatile T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<const volatile T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T>
struct TIsContiguousContainer<std::initializer_list<T>>
{
    enum { Value = true };
};