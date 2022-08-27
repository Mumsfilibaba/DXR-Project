#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsConst

template<typename T>
struct TIsConst
{
    enum { Value = false };
};

template<typename T>
struct TIsConst<const T>
{
    enum { Value = true };
};
