#pragma once
#include "And.h"
#include "IsSame.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsInteger

template<typename T>
struct TIsInteger
{
    enum { Value = false };
};

template<>
struct TIsInteger<bool>
{
    enum { Value = true };
};

template<>
struct TIsInteger<CHAR>
{
    enum { Value = true };
};

template<>
struct TIsInteger<signed char>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned char>
{
    enum { Value = true };
};

template<>
struct TIsInteger<WIDECHAR>
{
    enum { Value = true };
};

template<>
struct TIsInteger<char16_t>
{
    enum { Value = true };
};

template<>
struct TIsInteger<char32_t>
{
    enum { Value = true };
};

template<>
struct TIsInteger<short>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned short>
{
    enum { Value = true };
};

template<>
struct TIsInteger<int>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned int>
{
    enum { Value = true };
};

template<>
struct TIsInteger<long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<long long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned long long>
{
    enum { Value = true };
};

template <typename T>
struct TIsInteger<const T>
{
    enum { Value = TIsInteger<T>::Value };
};

template <typename T>
struct TIsInteger<volatile T>
{
    enum { Value = TIsInteger<T>::Value };
};

template <typename T>
struct TIsInteger<const volatile T>
{
    enum { Value = TIsInteger<T>::Value };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsIntegerNotBool

template<typename T>
struct TIsIntegerNotBool
{
    enum { Value = TAnd<TIsInteger<T>, TIsNotSame<T, bool>>::Value };
};
