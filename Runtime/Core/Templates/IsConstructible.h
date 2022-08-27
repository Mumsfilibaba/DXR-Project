#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsConstructible

template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    enum { Value = __is_constructible(T, ArgTypes...) };
};