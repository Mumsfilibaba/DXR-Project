#pragma once
#include "Core/CoreTypes.h"

enum EInPlace
{
    InPlace = 0
};

template<typename T>
struct TInPlaceType
{
    explicit TInPlaceType() = default;
};

template<int32 Index>
struct TInPlaceIndex
{
    explicit TInPlaceIndex() = default;
};