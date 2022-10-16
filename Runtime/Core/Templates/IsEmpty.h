#pragma once

template<typename T>
struct TIsEmpty
{
    enum { Value = __is_empty(T) };
};