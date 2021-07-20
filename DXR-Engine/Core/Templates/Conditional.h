#pragma once

/* Returns TrueType if condition is true, otherwise returns FalseType */
template<bool Condition, typename TrueType, typename FalseType>
struct TConditional
{
    typedef TrueType Type;
};

template<typename TrueType, typename FalseType>
struct TConditional<false, TrueType, FalseType>
{
    typedef FalseType Type;
};