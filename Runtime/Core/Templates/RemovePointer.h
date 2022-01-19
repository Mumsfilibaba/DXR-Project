#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Removes the pointer of the type

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