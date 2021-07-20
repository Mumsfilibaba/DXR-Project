#pragma once
#include "IsSame.h"
#include "RemoveCV.h"
#include "Or.h"

/* Determine if type is a integer point type */
template<typename T>
struct TIsInteger
{
    static constexpr bool Value = false;
};

template<>
struct TIsInteger<bool>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<char>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<signed char>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned char>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<wchar_t>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<char16_t>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<char32_t>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<short>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned short>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<int>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned int>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<long>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned long>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<long long>
{
    static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned long long>
{
    static constexpr bool Value = true;
};

template <typename T>
struct TIsInteger<const T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<volatile T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<const volatile T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};