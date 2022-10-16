#pragma once

template<typename FromType, typename ToType>
struct TIsConvertible
{
    enum { Value = __is_convertible_to(FromType, ToType) };
};

template<typename FromType, typename ToType>
struct TIsPointerConvertible
{
    enum { Value = TIsConvertible<FromType*, ToType*>::Value };
};
