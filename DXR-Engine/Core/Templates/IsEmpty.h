#pragma once

/* Determines if the class is final or not */
template<typename T>
struct TIsEmpty
{
    static constexpr bool Value = __is_empty( T );
};