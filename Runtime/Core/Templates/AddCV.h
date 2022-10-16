#pragma once

template<typename T>
struct TAddCV
{
    typedef const volatile T Type;
};

template<typename T>
struct TAddConst
{
    typedef const T Type;
};
 
template<typename T>
struct TAddVolatile
{
    typedef volatile T Type;
};