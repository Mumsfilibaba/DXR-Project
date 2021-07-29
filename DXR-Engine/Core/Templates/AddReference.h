#pragma once
#include "Identity.h"
#include "RemoveReference.h"

/* Adds a lvalue reference to the type */
template<typename T>
struct TAddLValueReference
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type&>::Type Type;
};

/* Adds a rvalue reference to the type */
template<typename T>
struct TAddRValueReference
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type&&>::Type Type;
};

/* Adds a reference of choice */
template<typename T>
struct TAddReference
{
    typedef typename TAddLValueReference<T>::Type LValue;
    typedef typename TAddRValueReference<T>::Type RValue;
};