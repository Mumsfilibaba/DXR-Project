#pragma once

/* Determines if the type is a enum type */
template<typename T>
struct TIsEnum
{
    static constexpr bool Value = __is_enum( T );
};