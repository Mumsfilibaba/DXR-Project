#pragma once
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMin

template<int32 Arg0, int32... RestArgs>
struct TMin;

template<int32 Arg0>
struct TMin<Arg0>
{
    enum { Value = Arg0 };
};

template<int32 Arg0, int32 Arg1, int32... RestArgs>
struct TMin<Arg0, Arg1, RestArgs...>
{
    enum { Value = (Arg0 <= Arg1) ? TMin<Arg0, RestArgs...>::Value : TMin<Arg1, RestArgs...>::Value };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMax

template<int32 Arg0, int32... RestArgs>
struct TMax;

template<int32 Arg0>
struct TMax<Arg0>
{
    enum { Value = Arg0 };
};

template<int32 Arg0, int32 Arg1, int32... RestArgs>
struct TMax<Arg0, Arg1, RestArgs...>
{
	enum { Value = (Arg0 >= Arg1) ? TMax<Arg0, RestArgs...>::Value : TMax<Arg1, RestArgs...>::Value };
};