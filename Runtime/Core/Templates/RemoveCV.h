#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRemoveCV

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRemoveConst

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRemoveVolatile

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