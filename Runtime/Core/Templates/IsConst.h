#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determines if the type is const or not

template<typename T>
struct TIsConst
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsConst<const T>
{
    enum
    {
        Value = true
    };
};
