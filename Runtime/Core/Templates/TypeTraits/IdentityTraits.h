#pragma once
#include "Core/CoreTypes.h"

template <typename T>
struct TVoid
{
    typedef void_type Type;
};

template<typename T>
struct TIdentity
{
    typedef T Type;
};

template<bool Val>
struct TValue
{
    inline static constexpr bool Value = Val;
};
