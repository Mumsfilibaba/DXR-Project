#pragma once
#include "Move.h"
#include "Decay.h"
#include "EnableIf.h"
#include "IsMemberPointer.h"
#include "IsBaseOf.h"

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

template <typename FuncType, typename... ArgTypes>
inline decltype(auto) Invoke(FuncType&& Func, ArgTypes&&... Args)
{
    return Internal::Invoke(Forward<FuncType>(Func), Forward<ArgTypes>(Args)...);
}

// TODO: InvokeR?