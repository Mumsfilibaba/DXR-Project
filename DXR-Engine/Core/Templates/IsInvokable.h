#pragma once
#include "Invoke.h"
#include "Void.h"
#include "DeclVal.h"
#include "IsConvertible.h"

// TODO: This may need another check

/* Determines if the type is invokable or not */
template<typename FuncType, typename... ArgTypes>
struct TIsInvokable
{
private:
    template<typename Fn, typename = void, typename... Args>
    struct TIsInvokableImpl
    {
        enum
        {
            Value = false
        };
    };

    template<typename Fn, typename... Args>
    struct TIsInvokableImpl<Fn, typename TVoid<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... )) >::Type, Args...>
    {
        enum
        {
            Value = true
        };
    };

public:
    enum
    {
        Value = TIsInvokableImpl<FuncType, void, ArgTypes...>::Value
    };
};

/* Determines if the type is invokable or not, with returntype */
template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsInvokableR
{
private:
    template<typename Fn, typename ReturnType, typename = void, typename... Args>
    struct TIsInvokableRImpl
    {
        enum
        {
            Value = false
        };
    };

    template<typename Fn, typename Ret, typename... Args>
    struct TIsInvokableRImpl<Fn, Ret, typename TVoid<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... )) >::Type, Args...>
    {
        enum
        {
            Value = TIsConvertible<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... )), Ret>::Value
        };
    };

public:
    enum
    {
        Value = TIsInvokableRImpl<FuncType, ReturnType, void, ArgTypes...>::Value
    };
};
