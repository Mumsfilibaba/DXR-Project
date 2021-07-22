#pragma once

/* Takes the T::Value and invert the result */
template<typename T>
struct TNot
{
    static constexpr bool Value = !T::Value;
};