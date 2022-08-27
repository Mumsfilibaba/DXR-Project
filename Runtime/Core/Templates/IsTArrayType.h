#pragma once

/** Checks if the type is a TArray-type(TArray, TArrayView, TStaticArray) */

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsTArrayType

template<typename T>
struct TIsTArrayType
{
    enum { Value = false };
};