#pragma once

/* Determines if the class is final or not */
template<typename T>
struct TIsFinal
{
    static constexpr bool Value = __is_final( T );
};