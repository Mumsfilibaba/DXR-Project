#pragma once

template<typename T>
struct TIsPOD
{
    enum { Value = __is_pod(T) };
};