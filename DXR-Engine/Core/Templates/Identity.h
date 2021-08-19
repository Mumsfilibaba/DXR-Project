#pragma once

/* Returns the type */
template<typename T>
struct TIdentity
{
    typedef T Type;
};

/* Returns the value */
template<bool InValue>
struct TValue
{
    enum
    {
        Value = InValue
    };
};