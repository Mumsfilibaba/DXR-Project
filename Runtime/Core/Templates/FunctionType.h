#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine the type of a function

template<typename FunctionType>
struct TFunctionType;

template<typename ReturnType, typename... ArgTypes>
struct TFunctionType<ReturnType(ArgTypes...)>
{
    typedef ReturnType(*Type)(ArgTypes...);
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine the type of a member function

template<bool IsConst, typename ClassType, typename FunctionType>
struct TMemberFunctionType;

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<false, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...);
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine the type of a const member function

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<true, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...) const;
};