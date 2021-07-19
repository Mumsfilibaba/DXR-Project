#pragma once

/* Check if type is pointer type */
template<typename T>
struct TIsPointer
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsPointer<T*>
{
    static constexpr bool Value = true;
};