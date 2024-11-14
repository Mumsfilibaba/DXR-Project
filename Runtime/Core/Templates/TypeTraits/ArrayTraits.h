#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

template<typename T>
struct TIsArray : TFalseType { };

template<typename T>
struct TIsArray<T[]> : TTrueType { };

template<typename T, TSIZE N>
struct TIsArray<T[N]> : TTrueType { };

template<typename T>
struct TIsBoundedArray : TFalseType { };

template<typename T, TSIZE N>
struct TIsBoundedArray<T[N]> : TTrueType { };

template<typename T>
struct TIsUnboundedArray : TFalseType { };

template<typename T>
struct TIsUnboundedArray<T[]> : TTrueType { };

template<typename T>
struct TRank : TIntegralConstant<TSIZE, 0> { };

template<typename T>
struct TRank<T[]> : TIntegralConstant<TSIZE, TRank<T>::Value + 1> { };

template<typename T, TSIZE N>
struct TRank<T[N]> : TIntegralConstant<TSIZE, TRank<T>::Value + 1> { };

template<typename T, TSIZE N = 0>
struct TExtent : TIntegralConstant<TSIZE, 0> { };

template<typename T>
struct TExtent<T[], 0> : TIntegralConstant<TSIZE, 0> { };

template<typename T, TSIZE N>
struct TExtent<T[], N> : TExtent<T, N - 1> { };

template<typename T, TSIZE I>
struct TExtent<T[I], 0> : TIntegralConstant<TSIZE, I> { };

template<typename T, TSIZE I, unsigned N>
struct TExtent<T[I], N> : TExtent<T, N - 1> { };

template<typename T>
struct TArrayElement
{
    typedef T Type;
};

template<typename T>
struct TArrayElement<T[]>
{
    typedef T Type;
};

template<typename T, TSIZE N>
struct TArrayElement<T[N]>
{
    typedef T Type;
};

template<typename T>
struct TIsMultiDimensionalArray : TIntegralConstant<bool, (TRank<T>::Value > 1)> { };