#pragma once

/* Adds const and volatile to type */
template<typename T>
struct TAddCV
{
    typedef const volatile T Type;
};

/* Adds const to type */
template<typename T>
struct TAddConst
{
    typedef const T Type;
};

/* Adds volatile to type */
template<typename T>
struct TAddVolatile
{
    typedef volatile T Type;
};