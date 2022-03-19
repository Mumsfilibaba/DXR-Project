#pragma once
#include "Delegate.h"
#include "MulticastDelegateBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Macros for declaring multi-cast delegate types */

#define DECLARE_MULTICAST_DELEGATE(DelegateName, ...)                          \
    /* The type pf delegates that will be executed by the multicast-delegate*/ \
    typedef TDelegate<void(__VA_ARGS__)> DelegateName##Type;                   \
    /* Multicast-delegate type */                                              \
    class DelegateName : public TMulticastDelegate<__VA_ARGS__>                \
    {                                                                          \
    };

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Multicast delegate

template<typename... ArgTypes>
class TMulticastDelegate : public CMulticastDelegateBase
{
    using Super = CMulticastDelegateBase;

    using DelegateType = TDelegate<void(ArgTypes...)>;

    template<typename... PayloadTypes>
    using FunctionType = typename TFunctionType<void(ArgTypes..., PayloadTypes...)>::Type;

    template<typename ClassType, typename... PayloadTypes>
    using MemberFunctionType = typename TMemberFunctionType<false, ClassType, void(ArgTypes..., PayloadTypes...)>::Type;

    template<typename ClassType, typename... PayloadTypes>
    using ConstMemberFunctionType = typename TMemberFunctionType<true, ClassType, void(ArgTypes..., PayloadTypes...)>::Type;

public:
    using Super::UnbindAll;
    using Super::Swap;
    using Super::Unbind;
    using Super::UnbindIfBound;
    using Super::IsBound;
    using Super::IsObjectBound;
    using Super::GetCount;

private:
    using Super::AddDelegate;
    using Super::CopyFrom;
    using Super::MoveFrom;
    using Super::CompactArray;
    using Super::Lock;
    using Super::Unlock;
    using Super::IsLocked;

public:

    /**
     * Default constructor
     */
    FORCEINLINE TMulticastDelegate()
        : Super()
    { }

    /**
     * Add a function to the delegate
     *
     * @param Function: Function to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateStatic<PayloadTypes...>(Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Add a member-function to the delegate
     *
     * @param This: Pointer to an instance to bind to the delegate
     * @param Function: MemberFunction to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Add a member-function to the delegate
     *
     * @param This: Pointer to an instance to bind to the delegate
     * @param Function: MemberFunction to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Add a const member-function to the delegate
     *
     * @param This: Pointer to an instance to bind to the delegate
     * @param Function: MemberFunction to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Add a const member-function to the delegate
     *
     * @param This: Pointer to an instance to bind to the delegate
     * @param Function: MemberFunction to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Bind a lambda to the delegate
     *
     * @param Functor: Functor to bind to the delegate
     * @param Payload: Arguments to bind to the delegate
     */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateLambda<FunctorType, PayloadTypes...>(Functor, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * Add any delegate
     * 
     * @param Delegate: Delegate to add
     * @return: Returns the delegate-handle of the added delegate
     */
    FORCEINLINE CDelegateHandle Add(const DelegateType& Delegate)
    {
        return Super::AddDelegate(Delegate);
    }

    /**
     * Execute all bound delegates
     * 
     * @param Args: Arguments for the function-calls
     */
    FORCEINLINE void Broadcast(ArgTypes... Args)
    {
        Super::Lock();

        for (int32 Index = 0; Index < Delegates.Size(); Index++)
        {
            DelegateType& Delegate = static_cast<DelegateType&>(Delegates[Index]);

            CDelegateHandle Handle = Delegate.GetHandle();
            if (Handle.IsValid())
            {
                Delegate.Execute(Forward<ArgTypes>(Args)...);
            }
        }

        Super::Unlock();
    }
};