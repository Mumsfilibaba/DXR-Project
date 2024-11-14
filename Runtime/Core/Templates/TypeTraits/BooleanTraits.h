#pragma once
#include "Core/Templates/TypeTraits/BasicTraits.h"

typedef TIntegralConstant<bool, true>  TTrueType;
typedef TIntegralConstant<bool, false> TFalseType;

template<bool bIdentityValue>
struct TValue
{
    static constexpr bool Value = bIdentityValue;
};

template<typename T>
struct TNot
{
    static constexpr bool Value = !T::Value;
};

template<typename... ArgsType>
struct TAnd;

template<typename T, typename... ArgsType>
struct TAnd<T, ArgsType...>
{
    static constexpr bool Value = (T::Value && TAnd<ArgsType...>::Value);
};

template<typename T>
struct TAnd<T>
{
    static constexpr bool Value = T::Value;
};

template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    static constexpr bool Value = (T::Value || TOr<ArgsType...>::Value);
};

template<typename T>
struct TOr<T>
{
    static constexpr bool Value = T::Value;
};

template<int64 Arg0, int64... Args>
struct TMin;

template<int64 Arg>
struct TMin<Arg>
{
    static constexpr int64 Value = Arg;
};

template<int64 Arg0, int64 Arg1, int64... Args>
struct TMin<Arg0, Arg1, Args...>
{
    static constexpr int64 Value = TMin<(Arg0 <= Arg1 ? Arg0 : Arg1), Args...>::Value;
};

template<int64 Arg0, int64... Args>
struct TMax;

template<int64 Arg>
struct TMax<Arg>
{
    static constexpr int64 Value = Arg;
};

template<int64 Arg0, int64 Arg1, int64... Args>
struct TMax<Arg0, Arg1, Args...>
{
    static constexpr int64 Value = TMax<(Arg0 >= Arg1 ? Arg0 : Arg1), Args...>::Value;
};
