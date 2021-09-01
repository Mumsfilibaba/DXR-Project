#pragma once
#include "Delegate.h"
#include "MulticastDelegateBase.h"

/* Macros for delcaring delegate types */

#define DECLARE_MULTICAST_DELEGATE( DelegateName, ... )               \
    class DelegateName : public TMulticastDelegate<__VA_ARGS__> \
    {                                                                 \
    };

/* Multicast delegate */
template<typename... ArgTypes>
class TMulticastDelegate : public CMulticastDelegateBase
{
    using Super = CMulticastDelegateBase;

    using DelegateType = TDelegate<void( ArgTypes... )>;

    template<typename... PayloadTypes>
    using FunctionType = typename TFunctionType<void( ArgTypes..., PayloadTypes... )>::Type;

    template<typename ClassType>
    using MemberFunctionType = typename TMemberFunctionType<false, ClassType, void( ArgTypes... )>::Type;

    template<typename ClassType>
    using ConstMemberFunctionType = typename TMemberFunctionType<true, ClassType, void( ArgTypes... )>::Type;

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

    /* Empty constructor */
    FORCEINLINE TMulticastDelegate()
        : Super()
    {
    }

    /* Bind "normal" function */
    template<typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddStatic( FunctionType<PayloadTypes...> Function, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateStatic<PayloadTypes...>( Function, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Bind member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, MemberFunctionType<InstanceType> Function, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateRaw<InstanceType, PayloadTypes...>( This, Function, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Bind member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, MemberFunctionType<ClassType> Function, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>( This, Function, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Bind const member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, ConstMemberFunctionType<InstanceType> Function, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateRaw<InstanceType, PayloadTypes...>( This, Function, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Bind const member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, ConstMemberFunctionType<ClassType> Function, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateRaw<InstanceType, ClassType, PayloadTypes...>( This, Function, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Bind Lambda or other functor */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddLambda( FunctorType Functor, PayloadTypes... Payload )
    {
        return Super::AddDelegate( DelegateType::template CreateLambda<FunctorType, PayloadTypes...>( Functor, Forward<PayloadTypes>( Payload )... ) );
    }

    /* Add a "standard" delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle Add( const DelegateType& Delegate )
    {
        return Super::AddDelegate( Delegate );
    }

    /* Broadcast to all bound delegates */
    FORCEINLINE void Broadcast( ArgTypes&&... Args )
    {
        Super::Lock();

        for ( int32 Index = 0; Index < Delegates.Size(); Index++ )
        {
            DelegateType& Delegate = (DelegateType&)Delegates[Index];

            CDelegateHandle Handle = Delegate.GetHandle();
            if ( Handle.IsValid() )
            {
                Delegate.Execute( Forward<ArgTypes>( Args )... );
            }
        }

        Super::Unlock();
    }
};