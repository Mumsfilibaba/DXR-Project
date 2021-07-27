#pragma once

/* Determines if the type is a union type */
template<typename T>
struct TIsUnion
{
    enum
    {
        Value = __is_union( T )
    };
};