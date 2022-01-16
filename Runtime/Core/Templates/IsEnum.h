#pragma once

/* Determines if the type is a enum type */
template<typename T>
struct TIsEnum
{
    enum
    {
        Value = __is_enum(T)
    };
};