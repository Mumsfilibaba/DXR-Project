#pragma once


template<typename FunctionType>
struct TFunctionType;

template<typename ReturnType, typename... ArgTypes>
struct TFunctionType<ReturnType(ArgTypes...)>
{
    typedef ReturnType(*Type)(ArgTypes...);
};


template<bool IsConst, typename ClassType, typename FunctionType>
struct TMemberFunctionType;

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<false, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...);
};

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<true, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...) const;
};