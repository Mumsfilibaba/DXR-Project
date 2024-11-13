#pragma once
#include "Core/CoreTypes.h"

template<typename T>
struct TNot
{
    inline static constexpr bool Value = !T::Value;
};

template<typename... ArgsType>
struct TAnd;

template<typename T, typename... ArgsType>
struct TAnd<T, ArgsType...>
{
    inline static constexpr bool Value = (T::Value && TAnd<ArgsType...>::Value);
};

template<typename T>
struct TAnd<T>
{
    inline static constexpr bool Value = T::Value;
};

template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    inline static constexpr bool Value = (T::Value || TOr<ArgsType...>::Value);
};

template<typename T>
struct TOr<T>
{
    inline static constexpr bool Value = T::Value;
};

template<int64 Arg0, int64... ArgsType>
struct TMin;

template<int64 Arg0>
struct TMin<Arg0>
{
    inline static constexpr int64 Value = Arg0;
};

template<int64 Arg0, int64 Arg1, int64... ArgsType>
struct TMin<Arg0, Arg1, ArgsType...>
{
    inline static constexpr int64 Value = (Arg0 <= Arg1) ? TMin<Arg0, ArgsType...>::Value : TMin<Arg1, ArgsType...>::Value;
};

template<int64 Arg0, int64... ArgsType>
struct TMax;

template<int64 Arg0>
struct TMax<Arg0>
{
    inline static constexpr int64 Value = Arg0;
};

template<int64 Arg0, int64 Arg1, int64... ArgsType>
struct TMax<Arg0, Arg1, ArgsType...>
{
    inline static constexpr int64 Value = (Arg0 >= Arg1) ? TMax<Arg0, ArgsType...>::Value : TMax<Arg1, ArgsType...>::Value;
};
