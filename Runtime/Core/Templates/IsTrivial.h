#pragma once
#include "And.h"

/** Determine if the object is trivially copyable. This in practice means that these
 *  objects can be safley copied with Memory::Memcpy
 */

template<typename T>
struct TIsTriviallyCopyable
{
    enum { Value = __is_trivially_copyable(T) };
};

template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    enum { Value = __is_trivially_constructible(T, ArgTypes...) };
};

template<typename T>
struct TIsTriviallyDestructable
{
    enum { Value = __is_trivially_destructible(T) };
};

template<typename T>
struct TIsTrivial
{
    enum { Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value };
};