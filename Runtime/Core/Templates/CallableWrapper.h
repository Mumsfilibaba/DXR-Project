#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Wrapper for a callable

template<typename ReturnType, typename... ArgTypes>
struct TCallableWrapper
{
    typedef ReturnType(*Type)(ArgTypes...);
};

template<typename ReturnType>
struct TCallableWrapper<ReturnType, void>
{
    typedef ReturnType(*Type)(void);
};