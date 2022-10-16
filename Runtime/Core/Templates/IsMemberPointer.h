#pragma once

template <typename T>
struct TIsMemberPointer
{
    enum { Value = false };
};

template <
    typename T,
    typename U>
struct TIsMemberPointer<T U::*>
{
    enum { Value = true };
};

template <typename T>
struct TIsMemberPointer<const T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};

template <typename T>
struct TIsMemberPointer<volatile T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};

template <typename T>
struct TIsMemberPointer<const volatile T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};