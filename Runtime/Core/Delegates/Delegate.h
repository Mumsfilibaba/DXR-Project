#pragma once
#include "DelegateBase.h"

#include "Core/Templates/FunctionType.h"

/* Macros for declaring delegate types */

#define DECLARE_DELEGATE( DelegateName, ... ) \
    typedef TDelegate<void(__VA_ARGS__)> DelegateName;

#define DECLARE_RETURN_DELEGATE( DelegateName, ReturnType, ... ) \
    typedef TDelegate<ReturnType(__VA_ARGS__)> DelegateName;

/* Delegate class, similar to TFunction, but allows direct binding of functions instead of binding a functor type*/
template<typename InvokableType>
class TDelegate;

template<typename ReturnType, typename... ArgTypes>
class TDelegate<ReturnType(ArgTypes...)> : public CDelegateBase
{
    using Super = CDelegateBase;

    using DelegateInstance = TDelegateInstance<ReturnType, ArgTypes...>;

    template<typename... PayloadTypes>
    using FunctionType = typename TFunctionType<ReturnType(ArgTypes..., PayloadTypes...)>::Type;

    template<typename ClassType, typename... PayloadTypes>
    using MemberFunctionType = typename TMemberFunctionType<false, ClassType, ReturnType(ArgTypes..., PayloadTypes...)>::Type;

    template<typename ClassType, typename... PayloadTypes>
    using ConstMemberFunctionType = typename TMemberFunctionType<true, ClassType, ReturnType(ArgTypes..., PayloadTypes...)>::Type;

public:
    using Super::GetBoundObject;
    using Super::Unbind;
    using Super::Swap;
    using Super::IsBound;
    using Super::IsObjectBound;
    using Super::UnbindIfBound;
    using Super::GetHandle;

private:
    using Super::Release;
    using Super::CopyFrom;
    using Super::GetDelegate;
    using Super::GetStorage;

public:

    /* Helper for creating a static delegate */
    template<typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindStatic<PayloadTypes...>(Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /* Helper for creating a member delegate */
    template<typename InstanceType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");

        TDelegate<ReturnType(ArgTypes...)> Delegate;
        Delegate.BindRaw<InstanceType, InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /* Helper for creating a member delegate */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");

        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /* Helper for creating a const member delegate */
    template<typename InstanceType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /* Helper for creating a const member delegate */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /* Helper for creating a lambda delegate */
    template<typename FunctorType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindLambda<FunctorType, PayloadTypes...>(Forward<FunctorType>(Functor), Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

public:

    /* Constructor for a empty delegate */
    FORCEINLINE explicit TDelegate()
        : Super()
    {
    }

    /* Bind "normal" function */
    template<typename... PayloadTypes>
    FORCEINLINE void BindStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TFunctionDelegateInstance<ReturnType(ArgTypes...), PayloadTypes...>>(Function, Forward<PayloadTypes>(Payload)...);
    }

    /* Bind member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");
        Bind<TMemberDelegateInstance<false, InstanceType, InstanceType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /* Bind member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");
        Bind<TMemberDelegateInstance<false, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /* Bind const member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TMemberDelegateInstance<true, InstanceType, InstanceType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /* Bind const member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TMemberDelegateInstance<true, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /* Bind Lambda or other functor */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE void BindLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        Bind<TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...), PayloadTypes...>>(Forward<FunctorType>(Functor), Forward<PayloadTypes>(Payload)...);
    }

    /* Executes the delegate */
    FORCEINLINE ReturnType Execute(ArgTypes... Args)
    {
        Assert(IsBound());
        return GetDelegateInstance()->Execute(Forward<ArgTypes>(Args)...);
    }

    /* Executes the delegate if there is any bound */
    FORCEINLINE bool ExecuteIfBound(ArgTypes... Args)
    {
        if (IsBound())
        {
            GetDelegateInstance()->Execute(Forward<ArgTypes>(Args)...);
            return true;
        }
        else
        {
            return false;
        }
    }

private:

    /* Binding and allocating storage */
    template<typename DelegateType, typename... ConstructorArgs>
    FORCEINLINE void Bind(ConstructorArgs&&... Args)
    {
        Release();

        void* Memory = AllocateStorage(sizeof(DelegateType));
        new (Memory) DelegateType(Forward<ConstructorArgs>(Args)...);
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE DelegateInstance* GetDelegateInstance() noexcept
    {
        return reinterpret_cast<DelegateInstance*>(GetDelegate());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const DelegateInstance* GetDelegateInstance() const noexcept
    {
        return reinterpret_cast<const DelegateInstance*>(GetDelegate());
    }
};