#pragma once
#include "DelegateBase.h"
#include "Core/Templates/TypeTraits.h"

#define DECLARE_DELEGATE(DelegateName, ...) \
    typedef TDelegate<void(__VA_ARGS__)> DelegateName;

#define DECLARE_RETURN_DELEGATE(DelegateName, ReturnType, ...) \
    typedef TDelegate<ReturnType(__VA_ARGS__)> DelegateName;

template<typename InvokableType>
class TDelegate;

template<typename ReturnType, typename... ArgTypes>
class TDelegate<ReturnType(ArgTypes...)> : public FDelegateBase
{
    using Super = FDelegateBase;

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
    using Super::CopyFrom;
    using Super::GetDelegate;
    using Super::GetStorage;

public:

    /**
     * @brief Create a static delegate from a function
     * @param Function Function to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the function and payload 
     */
    template<typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindStatic<PayloadTypes...>(Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /**
     * @brief Create a member-function delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the instance, function and payload
     */
    template<typename InstanceType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");

        TDelegate<ReturnType(ArgTypes...)> Delegate;
        Delegate.BindRaw<InstanceType, InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /**
     * @brief Create a member-function delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the instance, function and payload
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");

        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /**
     * @brief Create a const member-function delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the instance, function and payload
     */
    template<typename InstanceType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /**
     * @brief Create a const member-function delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the instance, function and payload
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

    /**
     * @brief Create a lambda delegate
     * @param Functor Functor to bind to a delegate
     * @param Payload Arguments to bind to a delegate
     * @return Returns a delegate bound with the functor and payload
     */
    template<typename FunctorType, typename... PayloadTypes>
    static FORCEINLINE TDelegate CreateLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        TDelegate Delegate;
        Delegate.BindLambda<FunctorType, PayloadTypes...>(Forward<FunctorType>(Functor), Forward<PayloadTypes>(Payload)...);
        return Delegate;
    }

public:

    /**
     * @brief Default constructor
     */
    FORCEINLINE explicit TDelegate()
        : Super()
    {
    }

    /**
     * @brief Bind a function to the delegate
     * @param Function Function to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename... PayloadTypes>
    FORCEINLINE void BindStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TFunctionDelegateInstance<ReturnType(ArgTypes...), PayloadTypes...>>(Function, Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Bind a member-function to the delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");
        Bind<TMemberDelegateInstance<false, InstanceType, InstanceType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Bind a member-function to the delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        static_assert(!TIsConst<InstanceType>::Value, "Cannot bind a non-const function to a const instance");
        Bind<TMemberDelegateInstance<false, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Bind a const member-function to the delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TMemberDelegateInstance<true, InstanceType, InstanceType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Bind a const member-function to the delegate
     * @param This Pointer to an instance to bind to the delegate
     * @param Function MemberFunction to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE void BindRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        Bind<TMemberDelegateInstance<true, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...>>(This, Function, Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Bind a lambda to the delegate
     * @param Functor Functor to bind to the delegate
     * @param Payload Arguments to bind to the delegate
     */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE void BindLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        Bind<TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...), PayloadTypes...>>(Forward<FunctorType>(Functor), Forward<PayloadTypes>(Payload)...);
    }

    /**
     * @brief Executes the delegate 
     * @param Args Arguments to pass to the call
     * @return The return value for the call
     */ 
    FORCEINLINE ReturnType Execute(ArgTypes... Args) const
    {
        CHECK(IsBound());
        return GetDelegateInstance()->Execute(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief Executes the delegate if there is a delegate bound
     * @param Args Arguments to pass to the call
     * @return Returns true if the call was perform, otherwise false
     */
    FORCEINLINE bool ExecuteIfBound(ArgTypes... Args) const
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
    template<typename DelegateType, typename... ConstructorArgs>
    FORCEINLINE void Bind(ConstructorArgs&&... Args)
    {
        Unbind();
        void* Memory = AllocateStorage(sizeof(DelegateType));
        new (Memory) DelegateType(::Forward<ConstructorArgs>(Args)...);
    }

    FORCEINLINE DelegateInstance* GetDelegateInstance() noexcept
    {
        return reinterpret_cast<DelegateInstance*>(GetDelegate());
    }

    FORCEINLINE const DelegateInstance* GetDelegateInstance() const noexcept
    {
        return reinterpret_cast<const DelegateInstance*>(GetDelegate());
    }
};
