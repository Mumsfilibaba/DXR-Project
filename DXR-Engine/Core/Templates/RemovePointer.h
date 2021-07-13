#pragma once

template<typename T>
struct _TRemovePointer
{
    using Type = T;
};

template<typename T>
struct _TRemovePointer<T*>
{
    using Type = T;
};

template<typename T>
struct _TRemovePointer<T* const>
{
    using Type = T;
};

template<typename T>
struct _TRemovePointer<T* volatile>
{
    using Type = T;
};

template<typename T>
struct _TRemovePointer<T* const volatile>
{
    using Type = T;
};

/* Removes pointer and retrives the type */
template<typename T>
using TRemovePointer = typename _TRemovePointer<T>::Type;