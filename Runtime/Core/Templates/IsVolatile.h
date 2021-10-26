#pragma once

/* Determines if the type is volatile or not */
template<typename T>
struct TIsVolatile
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsVolatile<volatile T>
{
    enum
    {
        Value = true
    };
};