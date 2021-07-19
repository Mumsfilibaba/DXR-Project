#pragma once

/* Determines if the type is const or not */
template<typename T>
struct TIsConst
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsConst<const T>
{
    static constexpr bool Value = true;
};
