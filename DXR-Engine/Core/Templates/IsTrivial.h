#pragma once
#include "And.h"

/*
* Detmine if the object is trivially copyable. This in practise means that these
* objects can be safley copied with Memory::Memcpy
*/
template<typename T>
struct TIsTriviallyCopyable
{
    static constexpr bool Value = __has_trivial_copy( T );
};

/*
* Detmine if the object is trivially constructable, that is that it has a compiler generated
* constructor.
*/
template<typename T>
struct TIsTriviallyConstructable
{
    static constexpr bool Value = __has_trivial_constructor( T );
};

/*
* Detmine if the object is trivially destructable, that is that it has a compiler generated
* destructor.
*/
template<typename T>
struct TIsTriviallyDestructable
{
    static constexpr bool Value = __has_trivial_destructor( T );
};

/* Determines if the type is trivially copyable and default constructable */
template<typename T>
struct TIsTrivial
{
    static constexpr bool Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value;
};