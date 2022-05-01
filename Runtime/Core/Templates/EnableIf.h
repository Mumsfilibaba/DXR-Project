#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TEnableIf

template<bool Condition, typename T = void>
struct TEnableIf
{
};

template<typename T>
struct TEnableIf<true, T>
{
    typedef T Type;
};
