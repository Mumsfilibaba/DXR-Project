#pragma once

template<typename T>
struct TIsPolymorphic
{
    enum { Value = __is_polymorphic(T) };
};