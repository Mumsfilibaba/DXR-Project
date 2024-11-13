#pragma once
#include "Core/CoreTypes.h"

template<typename T>
struct TRemoveCV
{
    using Type = T;
};

template<typename T>
struct TRemoveCV<const T>
{
    using Type = T;
};

template<typename T>
struct TRemoveCV<volatile T>
{
    using Type = T;
};

template<typename T>
struct TRemoveCV<const volatile T>
{
    using Type = T;
};

template<typename T>
struct TRemoveConst
{
    using Type = T;
};

template<typename T>
struct TRemoveConst<const T>
{
    using Type = T;
};

template<typename T>
struct TRemoveVolatile
{
    using Type = T;
};

template<typename T>
struct TRemoveVolatile<volatile T>
{
    using Type = T;
};

template<typename T>
struct TRemoveExtent
{
    using Type = T;
};

template<typename T>
struct TRemoveExtent<T[]>
{
    using Type = T;
};

template<typename T, TSIZE N>
struct TRemoveExtent<T[N]>
{
    using Type = T;
};

template<typename T>
struct TRemoveAllExtents
{
    using Type = T;
};

template<typename T>
struct TRemoveAllExtents<T[]>
{
    using Type = typename TRemoveAllExtents<T>::Type;
};

template<typename T, TSIZE N>
struct TRemoveAllExtents<T[N]>
{
    using Type = typename TRemoveAllExtents<T>::Type;
};

template<typename T>
struct TRemovePointer
{
    using Type = T;
};

template<typename T>
struct TRemovePointer<T*>
{
    using Type = T;
};

template<typename T>
struct TRemovePointer<T* const>
{
    using Type = T;
};

template<typename T>
struct TRemovePointer<T* volatile>
{
    using Type = T;
};

template<typename T>
struct TRemovePointer<T* const volatile>
{
    using Type = T;
};

template<typename T>
struct TRemoveReference
{
    using Type = T;
};

template<typename T>
struct TRemoveReference<T&>
{
    using Type = T;
};

template<typename T>
struct TRemoveReference<T&&>
{
    using Type = T;
};

template<typename T>
struct TRemoveCVRef
{
    using Type = typename TRemoveCV<typename TRemoveReference<T>::Type>::Type;
};
