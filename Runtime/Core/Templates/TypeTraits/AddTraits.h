#pragma once
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

template<typename T>
struct TAddCV
{
    typedef const volatile T Type;
};

template<typename T>
struct TAddConst
{
    typedef const T Type;
};

template<typename T>
struct TAddVolatile
{
    typedef volatile T Type;
};

template<typename T>
struct TAddLValueReference
{
private:
    template <class U>
    static TTypeIdentity<U&> TryAdd(int);

    template <class U>
    static TTypeIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

template<typename T>
struct TAddRValueReference
{
private:
    template <class U>
    static TTypeIdentity<U&&> TryAdd(int);

    template <class U>
    static TTypeIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

template<typename T>
struct TAddPointer
{
private:
    template <typename U>
    static TTypeIdentity<typename TRemoveReference<U>::Type*> TryAdd(int);

    template <typename U>
    static TTypeIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};
