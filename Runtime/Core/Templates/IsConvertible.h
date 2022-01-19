#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine is two types are convertible

template<typename FromType, typename ToType>
struct TIsConvertible
{
    enum
    {
        Value = __is_convertible_to(FromType, ToType)
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine if two pointers can be converted

template<typename FromType, typename ToType>
struct TIsPointerConvertible
{
    enum
    {
        Value = TIsConvertible<FromType*, ToType*>::Value
    };
};
