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
    struct TIsInvokableHelper
    {
        enum
        {
            Value = false
        };
    };

    template<typename Fn, typename... Args>
    struct TIsInvokableHelper<Fn, typename TVoid<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... )) >::Type, Args...>
    {
        enum
        {
            Value = true
        };
    };

public:

    enum
    {
        Value = TIsInvokableHelper<FuncType, void, ArgTypes...>::Value
    };
};

/* Determines if the type is invokable or not, with returntype */
template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsInvokableR
{
private:
    template<typename Fn, typename Ret, typename = void, typename... Args>
    struct TIsInvokableRHelper
    {
        enum
        {
            Value = false
        };
    };

    template<typename Fn, typename Ret, typename... Args>
    struct TIsInvokableRHelper<Fn, Ret, typename TVoid<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... ))>::Type, Args...>
    {
        enum
        {
            Value = TIsConvertible<decltype(Internal::Invoke( DeclVal<Fn>(), DeclVal<Args>()... )), Ret>::Value
        };
    };

public:

    enum
    {
        Value = TIsInvokableRHelper<FuncType, ReturnType, void, ArgTypes...>::Value
    };
};
