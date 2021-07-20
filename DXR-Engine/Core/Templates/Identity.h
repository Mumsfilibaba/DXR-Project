#pragma once

/* Returns the type (C++20) */
template<typename T>
struct TIdentity
{
    typedef T Type;
};