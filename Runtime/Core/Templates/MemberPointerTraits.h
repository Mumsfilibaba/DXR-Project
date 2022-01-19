#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Traits for a member-pointer

template<typename T>
struct TMemberPointerTraits
{
};

template<typename T, typename U>
struct TMemberPointerTraits<U T::*>
{
    typedef U Type;
};