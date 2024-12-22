#pragma once
#include "Core/CoreTypes.h"

template<typename T>
struct TRemoveCV
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<const T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<volatile T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<const volatile T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveConst
{
    typedef T Type;
};

template<typename T>
struct TRemoveConst<const T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveVolatile
{
    typedef T Type;
};

template<typename T>
struct TRemoveVolatile<volatile T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveExtent
{
    typedef T Type;
};

template<typename T>
struct TRemoveExtent<T[]>
{
    typedef T Type;
};

template<typename T, SIZE_T N>
struct TRemoveExtent<T[N]>
{
    typedef T Type;
};

template<typename T>
struct TRemoveAllExtents
{
    typedef T Type;
};

template<typename T>
struct TRemoveAllExtents<T[]>
{
    typedef typename TRemoveAllExtents<T>::Type Type;
};

template<typename T, SIZE_T N>
struct TRemoveAllExtents<T[N]>
{
    typedef typename TRemoveAllExtents<T>::Type Type;
};

template<typename T>
struct TRemovePointer
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T*>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* const>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* volatile>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* const volatile>
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&>
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&&>
{
    typedef T Type;
};

template<typename T>
struct TRemoveCVRef
{
    typedef typename TRemoveCV<typename TRemoveReference<T>::Type>::Type Type;
};
