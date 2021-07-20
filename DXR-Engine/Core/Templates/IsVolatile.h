#pragma once

/* Determines if the type is volatile or not */
template<typename T>
struct TIsVolatile
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsVolatile<volatile T>
{
    static constexpr bool Value = true;
};