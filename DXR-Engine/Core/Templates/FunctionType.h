#pragma once

/* Determine the type of a function */
template<typename ReturnType, typename... ArgTypes>
struct TFunctionType
{
    typedef ReturnType( *Type )(ArgTypes...);
};

/* Determine the type of a member function */
template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType
{
    typedef ReturnType( ClassType::* Type )(ArgTypes...);
};

/* Determine the type of a const member function */
template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TConstMemberFunctionType
{
    typedef ReturnType( ClassType::* Type )(ArgTypes...) const;
};