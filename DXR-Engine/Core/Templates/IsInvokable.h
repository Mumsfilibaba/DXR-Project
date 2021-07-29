#pragma once
#include "Invoke.h"
#include "Void.h"
#include "DeclVal.h"
#include "IsConvertible.h"

namespace Internal
{
    template<typename T, typename = void, typename... ArgTypes>
    struct _TIsInvokable
    {
        enum
        {
            Value = false;
        };
    };

    template<typename T, typename... ArgTypes>
    struct _TIsInvokable<T, typename TVoid<decltype(Invoke( DeclVal<T>(), DeclVal<ArgTypes>()... )) >::Type, ArgTypes...>
    {
        enum
        {
            Value = true;
        };
    };

    template<typename T, typename ReturnType, typename = void, typename... ArgTypes>
    struct _TIsInvokableR
    {
        enum
        {
            Value = false;
        };
    };

    template<typename T, typename ReturnType, typename... ArgTypes>
    struct _TIsInvokableR<T, ReturnType, typename TVoid<decltype(Invoke( DeclVal<T>(), DeclVal<ArgTypes>()... )) >::Type, ArgTypes...>
    {
        enum
        {
            Value = TIsConvertible<decltype(Invoke( DeclVal<T>(), DeclVal<ArgTypes>()... )), ReturnType>::Value;
        };
    };
}

/* Determines if the type is invokable or not */
template<typename T, typename... ArgTypes>
struct TIsInvokable : private Internal::_TIsInvokable<T, void, ArgTypes...>
{
};

/* Determines if the type is invokable or no, with returntype */
template<typename T, typename ReturnType, typename... ArgTypes>
struct TIsInvokableR : private Internal::_TIsInvokableR<T, ReturnType, void, ArgTypes...>
{
};
