#pragma once
#include "Utility.h"
#include "TypeTraits.h"

namespace Internal
{
    template<typename FuncType, typename... ArgTypes>
    inline auto Invoke(FuncType&& Func, ArgTypes&&... Args)
        -> decltype(Forward<FuncType>(Func)(Forward<ArgTypes>(Args)...))
    {
        return Forward<FuncType>(Func)(Forward<ArgTypes>(Args)...);
    }

    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto Invoke(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        -> typename TEnableIf<TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value, decltype((Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...))>::Type
    {
        return (Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...);
    }

    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto Invoke(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        -> decltype(((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...))
    {
        return ((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...);
    }

    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto Invoke(ReturnType ObjectType::* Member, InstanceType&& Obj)
        -> typename TEnableIf<TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value, decltype(Obj.*Member)>::Type
    {
        return Obj.*Member;
    }

    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto Invoke(ReturnType ObjectType::* Member, InstanceType&& Obj)
        -> decltype((*Forward<InstanceType>(Obj)).*Member)
    {
        return (*Forward<InstanceType>(Obj)).*Member;
    }
}

template<typename FuncType, typename... ArgTypes>
inline decltype(auto) Invoke(FuncType&& Func, ArgTypes&&... Args)
{
    return Internal::Invoke(::Forward<FuncType>(Func), ::Forward<ArgTypes>(Args)...);
}

// TODO: InvokeR

// TODO: This may need another check
template<typename FuncType, typename... ArgTypes>
struct TIsInvokable
{
private:
    template<typename Fn, typename = void, typename... Args>
    struct TIsInvokableHelper
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename... Args>
    struct TIsInvokableHelper<Fn, typename TVoid<decltype(Internal::Invoke(DeclVal<Fn>(), DeclVal<Args>()...)) >::Type, Args...>
    {
        inline static constexpr bool Value = true;
    };

public:
    inline static constexpr bool Value = TIsInvokableHelper<FuncType, void, ArgTypes...>::Value;
};

template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsInvokableR
{
private:
    template<typename Fn, typename Ret, typename = void, typename... Args>
    struct TIsInvokableRHelper
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename Ret, typename... Args>
    struct TIsInvokableRHelper<Fn, Ret, typename TVoid<decltype(Internal::Invoke(DeclVal<Fn>(), DeclVal<Args>()...))>::Type, Args...>
    {
        inline static constexpr bool Value = TIsConvertible<decltype(Internal::Invoke(DeclVal<Fn>(), DeclVal<Args>()...)), Ret>::Value;
    };

public:
    inline static constexpr bool Value = TIsInvokableRHelper<FuncType, ReturnType, void, ArgTypes...>::Value;
};