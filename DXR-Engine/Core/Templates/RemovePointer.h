#pragma once

template<typename T>
struct _TRemovePointer
{
    typedef T Type;
};

template<typename T>
struct _TRemovePointer<T*>
{
    typedef T Type;
};

template<typename T>
struct _TRemovePointer<T* const>
{
    typedef T Type;
};

template<typename T>
struct _TRemovePointer<T* volatile>
{
    typedef T Type;
};

template<typename T>
struct _TRemovePointer<T* const volatile>
{
    typedef T Type;
};

/* Removes pointer and retrives the type */
template<typename T>
using TRemovePointer = typename _TRemovePointer<T>::Type;