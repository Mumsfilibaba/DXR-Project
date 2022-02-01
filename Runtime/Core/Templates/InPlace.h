#pragma once
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInPlace

struct CInPlace
{
    explicit CInPlace() = default;
};

inline constexpr CInPlace InPlace = CInPlace();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TInPlace

template<typename T>
struct TInPlace
{
    explicit TInPlace() = default;
};

template<typename T>
inline constexpr TInPlace<T> InPlaceType = TInPlace<T>();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TInPlaceIndex

template<int32 Index>
struct TInPlaceIndex
{
    explicit TInPlace() = default;
};

template<int32 Index>
inline constexpr TInPlaceIndex<Index> InPlaceIndex = TInPlaceIndex<Index>();