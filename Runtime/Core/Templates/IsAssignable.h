#pragma once

template<typename T, typename FromType>
struct TIsAssignable
{
    enum { Value = __is_assignable(T, FromType) };
};