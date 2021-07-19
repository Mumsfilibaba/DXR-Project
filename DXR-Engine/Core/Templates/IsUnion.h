#pragma once

/* Determines if the type is a union type */
template<typename T>
struct TIsUnion
{
    static constexpr bool Value = __is_union(T);
};