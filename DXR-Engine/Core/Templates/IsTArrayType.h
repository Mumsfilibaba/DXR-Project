#pragma once

/* Checks if the type is a TArray-type (TArray, TArrayView, TFixedArray) */
template<typename T>
struct TIsTArrayType
{
    enum
    {
        Value = false;
    };
};