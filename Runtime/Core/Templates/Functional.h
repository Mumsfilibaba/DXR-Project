#pragma once
#include "Utility.h"
#include "TypeTraits.h"

template<typename FuncType, typename... ArgTypes>
struct TIsInvokable;

template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsInvokableReturn;

template<typename FuncType, typename... ArgTypes>
struct TIsNothrowInvokable;

template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsNothrowInvokableReturn;

namespace InvokeInternals
{
    // Invoke for general callables
    template<typename FuncType, typename... ArgTypes>
    inline auto Invoke(FuncType&& Func, ArgTypes&&... Args)
        noexcept(noexcept(Forward<FuncType>(Func)(Forward<ArgTypes>(Args)...)))
        -> decltype(Forward<FuncType>(Func)(Forward<ArgTypes>(Args)...))
    {
        return Forward<FuncType>(Func)(Forward<ArgTypes>(Args)...);
    }

    // Invoke for member function pointers (object)
    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto Invoke(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        noexcept(noexcept((Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...)))
        -> typename TEnableIf<
            TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value,
            decltype((Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...))
           >::Type
    {
        return (Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...);
    }

    // Invoke for member function pointers (pointer)
    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto Invoke(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        noexcept(noexcept(((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...)))
        -> decltype(((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...))
    {
        return ((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...);
    }

    // Invoke for member object pointers (object)
    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto Invoke(ReturnType ObjectType::* Member, InstanceType&& Obj)
        noexcept(noexcept(Obj.*Member))
        -> typename TEnableIf<
            TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value,
            decltype(Obj.*Member)
           >::Type
    {
        return Obj.*Member;
    }

    // Invoke for member object pointers (pointer)
    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto Invoke(ReturnType ObjectType::* Member, InstanceType&& Obj)
        noexcept(noexcept((*Forward<InstanceType>(Obj)).*Member))
        -> decltype((*Forward<InstanceType>(Obj)).*Member)
    {
        return (*Forward<InstanceType>(Obj)).*Member;
    }

    // InvokeAndReturn for general callables with return type
    template<typename ReturnType, typename FuncType, typename... ArgTypes>
    inline auto InvokeAndReturn(FuncType&& Func, ArgTypes&&... Args)
        noexcept(noexcept(static_cast<ReturnType>(Invoke(Forward<FuncType>(Func), Forward<ArgTypes>(Args)...))))
        -> typename TEnableIf<
            TIsInvokableReturn<FuncType, ReturnType, ArgTypes...>::Value,
            ReturnType
           >::Type
    {
        return static_cast<ReturnType>(Invoke(Forward<FuncType>(Func), Forward<ArgTypes>(Args)...));
    }

    // InvokeAndReturn for member function pointers (object) with return type
    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto InvokeAndReturn(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        noexcept(noexcept(static_cast<ReturnType>((Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...))))
        -> typename TEnableIf<
            TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value &&
            TIsInvokableReturn<decltype(Func), ReturnType, ArgTypes...>::Value,
            ReturnType
           >::Type
    {
        return static_cast<ReturnType>((Forward<InstanceType>(Obj).*Func)(Forward<ArgTypes>(Args)...));
    }

    // InvokeAndReturn for member function pointers (pointer) with return type
    template<typename ReturnType, typename ObjectType, typename InstanceType, typename... ArgTypes>
    inline auto InvokeAndReturn(ReturnType ObjectType::* Func, InstanceType&& Obj, ArgTypes&&... Args)
        noexcept(noexcept(static_cast<ReturnType>(((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...))))
        -> typename TEnableIf<
            !TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value &&
            TIsInvokableReturn<decltype(Func), ReturnType, ArgTypes...>::Value,
            ReturnType
           >::Type
    {
        return static_cast<ReturnType>(((*Forward<InstanceType>(Obj)).*Func)(Forward<ArgTypes>(Args)...));
    }

    // InvokeAndReturn for member object pointers (object) with return type
    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto InvokeAndReturn(ReturnType ObjectType::* Member, InstanceType&& Obj)
        noexcept(noexcept(static_cast<ReturnType>(Obj.*Member)))
        -> typename TEnableIf<
            TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value &&
            TIsConvertible<decltype(Obj.*Member), ReturnType>::Value,
            ReturnType
           >::Type
    {
        return static_cast<ReturnType>(Obj.*Member);
    }

    // InvokeAndReturn for member object pointers (pointer) with return type
    template<typename ReturnType, typename ObjectType, typename InstanceType>
    inline auto InvokeAndReturn(ReturnType ObjectType::* Member, InstanceType&& Obj)
        noexcept(noexcept(static_cast<ReturnType>((*Forward<InstanceType>(Obj)).*Member)))
        -> typename TEnableIf<
            !TIsBaseOf<ObjectType, typename TDecay<decltype(Obj)>::Type>::Value &&
            TIsConvertible<decltype((*Forward<InstanceType>(Obj)).*Member), ReturnType>::Value,
            ReturnType
           >::Type
    {
        return static_cast<ReturnType>((*Forward<InstanceType>(Obj)).*Member);
    }
}

// Public Invoke function remains unchanged
template<typename FuncType, typename... ArgTypes>
inline decltype(auto) Invoke(FuncType&& Func, ArgTypes&&... Args)
{
    return InvokeInternals::Invoke(::Forward<FuncType>(Func), ::Forward<ArgTypes>(Args)...);
}

// Public InvokeAndReturn function
template<typename ReturnType, typename FuncType, typename... ArgTypes>
inline typename TEnableIf<TIsInvokableReturn<FuncType, ReturnType, ArgTypes...>::Value, ReturnType >::Type InvokeAndReturn(FuncType&& Func, ArgTypes&&... Args)
{
    return InvokeInternals::InvokeAndReturn<ReturnType>(::Forward<FuncType>(Func), ::Forward<ArgTypes>(Args)...);
}

// TIsInvokable
template<typename FuncType, typename... ArgTypes>
struct TIsInvokable
{
private:
    template<typename Fn, typename = void, typename... Args>
    struct TIsInvokableImpl
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename... Args>
    struct TIsInvokableImpl<Fn, typename TVoid<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...))>::Type, Args...>
    {
        inline static constexpr bool Value = true;
    };

public:
    inline static constexpr bool Value = TIsInvokableImpl<FuncType, void, ArgTypes...>::Value;
};

// TIsInvokableReturn
template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsInvokableReturn
{
private:
    template<typename Fn, typename Ret, typename = void, typename... Args>
    struct TIsInvokableReturnImpl
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename Ret, typename... Args>
    struct TIsInvokableReturnImpl<Fn, Ret, typename TVoid<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...))>::Type, Args...>
    {
        inline static constexpr bool Value = TIsConvertible<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...)), Ret>::Value;
    };

public:
    inline static constexpr bool Value = TIsInvokableReturnImpl<FuncType, ReturnType, void, ArgTypes...>::Value;
};

// TIsNothrowInvokable
template<typename FuncType, typename... ArgTypes>
struct TIsNothrowInvokable
{
private:
    template<typename Fn, typename = void, typename... Args>
    struct TIsNothrowInvokableImpl
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename... Args>
    struct TIsNothrowInvokableImpl<Fn, typename TVoid<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...))>::Type, Args...>
    {
        inline static constexpr bool Value = noexcept(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...));
    };

public:
    inline static constexpr bool Value = TIsNothrowInvokableImpl<FuncType, void, ArgTypes...>::Value;
};

// TIsNothrowInvokableReturn
template<typename FuncType, typename ReturnType, typename... ArgTypes>
struct TIsNothrowInvokableReturn
{
private:
    template<typename Fn, typename Ret, typename = void, typename... Args>
    struct TIsNothrowInvokableReturnImpl
    {
        inline static constexpr bool Value = false;
    };

    template<typename Fn, typename Ret, typename... Args>
    struct TIsNothrowInvokableReturnImpl<Fn, Ret, typename TVoid<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...))>::Type, Args...>
    {
        inline static constexpr bool Value = TIsConvertible<decltype(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...)), Ret>::Value
            && noexcept(InvokeInternals::Invoke(DeclVal<Fn>(), DeclVal<Args>()...));
    };

public:
    inline static constexpr bool Value = TIsNothrowInvokableReturnImpl<FuncType, ReturnType, void, ArgTypes...>::Value;
};
