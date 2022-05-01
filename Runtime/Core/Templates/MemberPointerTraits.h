#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMemberPointerTraits

template<typename T>
struct TMemberPointerTraits
{
};

template<typename T, typename U>
struct TMemberPointerTraits<U T::*>
{
    typedef U Type;
};