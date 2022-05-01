#pragma once
#include "Identity.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAddLValueReference

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAddRValueReference

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAddReference

template<typename T>
struct TAddReference
{
    typedef typename TAddLValueReference<T>::Type LValue;
    typedef typename TAddRValueReference<T>::Type RValue;
};