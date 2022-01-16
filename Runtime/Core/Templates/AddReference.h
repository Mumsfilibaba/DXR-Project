#pragma once
#include "Identity.h"

/* Adds a lvalue reference to the type */
template<typename T>
struct TAddLValueReference
{
private:
    template <class U>
    static TIdentity<U&> TryAdd(int);

    template <class U>
    static TIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

/* Adds a rvalue reference to the type */
template<typename T>
struct TAddRValueReference
{
private:
    template <class U>
    static TIdentity<U&&> TryAdd(int);

    template <class U>
    static TIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

/* Adds a reference of choice */
template<typename T>
struct TAddReference
{
    typedef typename TAddLValueReference<T>::Type LValue;
    typedef typename TAddRValueReference<T>::Type RValue;
};