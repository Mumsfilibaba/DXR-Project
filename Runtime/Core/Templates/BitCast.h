#pragma once
#include "Core/Core.h"

template<typename T, typename F>
constexpr T BitCast(F Value)
{
    union
    {
        F Src;
        T Dst;
    } UnionCast;

    UnionCast.Src = Value;
    return UnionCast.Dst;
}