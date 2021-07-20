#pragma once

/* Checks if type is a member pointer */
template <typename T>
struct TIsMemberPointer
{
    static constexpr bool Value = false;
};

template <typename T, typename U>
struct TIsMemberPointer<T U::*>
{
    static constexpr bool Value = true;
};

template <typename T>
struct TIsMemberPointer<const T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<volatile T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<const volatile T>
{
    static constexpr bool Value = TIsMemberPointer<T>::Value;
};