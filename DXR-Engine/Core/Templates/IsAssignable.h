#pragma once

/* Determines weather the type can be assigned by the specified type */
template<typename T, typename FromType>
struct TIsAssignable
{
    static constexpr bool Value = __is_assignable(T, FromType);
};