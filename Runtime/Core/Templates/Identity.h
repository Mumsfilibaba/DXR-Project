#pragma once

template<typename T>
struct TIdentity
{
    typedef T Type;
};

template<bool InValue>
struct TValue
{
    enum { Value = InValue };
};