#pragma once
#include "Identity.h"
#include "RemoveReference.h"

/* Adds a lvalue reference to the type */
template<typename T>
struct TAddLeftReference
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type&>::Type Type;
};

/* Adds a rvalue reference to the type */
template<typename T>
struct TAddRightReference
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type&&>::Type Type;
};