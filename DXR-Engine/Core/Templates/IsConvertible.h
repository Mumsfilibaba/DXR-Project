#pragma once

/* Determine is two types are convertible */
template<typename FromType, typename ToType>
struct TIsConvertible
{
    enum
    {
        Value = __is_convertible( FromType, ToType )
    };
};