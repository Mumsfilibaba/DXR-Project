#pragma once

/* Takes the T::Value and invert the result */
template<typename T>
struct TNot
{
    enum
    {
        Value = !T::Value
    };
};