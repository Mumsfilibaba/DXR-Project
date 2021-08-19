#pragma once
#include "MulticastDelegate.h"

/* Base type for events */
template<typename... ArgTypes>
class TEventBase : public TMulticastBase<ArgTypes...>
{
    typedef TMulticastBase<ArgTypes...>    Base;
    typedef TDelegate<void( ArgTypes... )> TDelegateType;

protected:

    /* Empty constructor */
    FORCEINLINE TEventBase()
        : Base()
    {
    }

    /* Copy constructor */
    FORCEINLINE TEventBase( const TEventBase& Other )
        : Base( Other )
    {
    }

    /* Move constructor */
    FORCEINLINE TEventBase( TEventBase&& Other )
        : Base( Move( Other ) )
    {
    }

    FORCEINLINE ~TEventBase()
    {
        UnbindAll();
    }

    /* Unbind all delegates */
    FORCEINLINE void UnbindAll()
    {
        for ( IDelegate* Delegate : Base::Delegates )
        {
            Assert( Delegate != nullptr );
            delete Delegate;
        }

        Base::Delegates.Clear();
    }

    /* Swaps two events */
    FORCEINLINE void Swap( TEventBase& Other )
    {
        Base::Swap( Other );
    }

    /* Copy assignment operator */
    FORCEINLINE TEventBase& operator=( const TEventBase& RHS )
    {
        TEventBase( RHS ).Swap( *this );
        return *this;
    }

    /* Move assignment operator */
    FORCEINLINE TEventBase& operator=( TEventBase&& RHS )
    {
        TEventBase( Move( RHS ) ).Swap( *this );
        return *this;
    }
};

/* Default event type */
template<typename... TArgs>
class TEvent : public TEventBase<TArgs...>
{
protected:
    typedef TEventBase<TArgs...> Base;

    typedef typename Base::IDelegate IDelegate;

    void Broadcast( TArgs... Args )
    {
        for ( IDelegate* Delegate : Base::Delegates )
        {
            Assert( Delegate != nullptr );
            Delegate->Execute( Forward<TArgs>( Args )... );
        }
    }

    void operator()( TArgs... Args )
    {
        return Broadcast( Forward<TArgs>( Args )... );
    }
};

/* Special event type for events with not params */
template<>
class TEvent<void> : public TEventBase<void>
{
protected:
    typedef TEventBase<void> Base;

    typedef typename Base::IDelegate IDelegate;

    void Broadcast()
    {
        for ( IDelegate* Delegate : Base::Delegates )
        {
            Assert( Delegate != nullptr );
            Delegate->Execute();
        }
    }

    void operator()()
    {
        return Broadcast();
    }
};

#define DECLARE_EVENT(EventType, EventDispatcherType, ...) \
    class EventType : public TEvent<__VA_ARGS__> \
    { \
        friend class EventDispatcherType; \
    }; \