#pragma once

template<typename T>
struct TIsFinal
{
    enum { Value = __is_final(T) };
};