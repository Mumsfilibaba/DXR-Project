#pragma once
#include "Delegate.h"
#include "MulticastDelegateBase.h"

#define DECLARE_MULTICAST_DELEGATE(DelegateName, ...)                           \
    /** The type of delegates that will be executed by the MulticastDelegate */ \
    typedef TDelegate<void(__VA_ARGS__)> DelegateName##Type;                    \
                                                                                \
    class DelegateName                                                          \
        : public TMulticastDelegate<__VA_ARGS__>                                \
    {                                                                           \
    };

template<typename... ArgTypes>
class TMulticastDelegate : public FMulticastDelegateBase
{
    using Super = FMulticastDelegateBase;

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
     * @brief - Default constructor
     */
    FORCEINLINE TMulticastDelegate()
        : Super()
    {
    }

    /**
     * @brief          - Add a function to the delegate
     * @param Function - Function to bind to the delegate
     * @param Payload  - Arguments to bind to the delegate
     */
    template<typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateStatic<PayloadTypes...>(Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief          - Add a member-function to the delegate
     * @param This     - Pointer to an instance to bind to the delegate
     * @param Function - MemberFunction to bind to the delegate
     * @param Payload  - Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief          - Add a member-function to the delegate
     * @param This     - Pointer to an instance to bind to the delegate
     * @param Function - MemberFunction to bind to the delegate
     * @param Payload  - Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief          - Add a const member-function to the delegate
     * @param This     - Pointer to an instance to bind to the delegate
     * @param Function - MemberFunction to bind to the delegate
     * @param Payload  - Arguments to bind to the delegate
     */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<InstanceType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief          - Add a const member-function to the delegate
     * @param This     - Pointer to an instance to bind to the delegate
     * @param Function - MemberFunction to bind to the delegate
     * @param Payload  - Arguments to bind to the delegate
     */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<ClassType, PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief         - Bind a lambda to the delegate
     * @param Functor - Functor to bind to the delegate
     * @param Payload - Arguments to bind to the delegate
     */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE FDelegateHandle AddLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        return Super::AddDelegate(DelegateType::template CreateLambda<FunctorType, PayloadTypes...>(Functor, Forward<PayloadTypes>(Payload)...));
    }

    /**
     * @brief          - Add any delegate
     * @param Delegate - Delegate to add
     * @return         - Returns the delegate-handle of the added delegate
     */
    FORCEINLINE FDelegateHandle Add(const DelegateType& Delegate)
    {
        return Super::AddDelegate(Delegate);
    }

    /**
     * @brief      - Execute all bound delegates
     * @param Args - Arguments for the function-calls
     */
    FORCEINLINE void Broadcast(ArgTypes... Args)
    {
        Super::Lock();

        for (int32 Index = 0; Index < Delegates.Size(); Index++)
        {
            DelegateType& Delegate = static_cast<DelegateType&>(Delegates[Index]);

            FDelegateHandle Handle = Delegate.GetHandle();
            if (Handle.IsValid())
            {
                Delegate.Execute(Forward<ArgTypes>(Args)...);
            }
        }

        Super::Unlock();
    }
};
