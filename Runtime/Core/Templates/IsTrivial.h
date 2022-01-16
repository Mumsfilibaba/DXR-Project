#pragma once
#include "And.h"

/*
* Detmine if the object is trivially copyable. This in practise means that these
* objects can be safley copied with Memory::Memcpy
*/
template<typename T>
struct TIsTriviallyCopyable
{
    enum
    {
        Value = __is_trivially_copyable(T)
    };
};

/*
* Detmine if the object is trivially constructable, that is that it has a compiler generated
* constructor.
*/
template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    enum
    {
        Value = __is_trivially_constructible(T, ArgTypes...)
    };
};

/*
* Detmine if the object is trivially destructable, that is that it has a compiler generated
* destructor.
*/
template<typename T>
struct TIsTriviallyDestructable
{
    enum
    {
        Value = __is_trivially_destructible(T)
    };
};

/* Determines if the type is trivially copyable and default constructable */
template<typename T>
struct TIsTrivial
{
    enum
    {
        Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value
    };
};