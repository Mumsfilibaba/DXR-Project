#pragma once
#include "MulticastBase.h"

/* Multicast delegate */
template<typename... ArgTypes>
class TMulticastDelegate : public TMulticastBase<ArgTypes...>
{
    typedef typename TMulticastBase<ArgTypes...>    Base;
    typedef typename TDelegate<void( ArgTypes... )> TDelegateType;

public:

    using Base::BroadCast;

    /* Broadcast to all bound delegates */
    FORCEINLINE void Broadcast( ArgTypes&&... Args )
    {
        Base::Broadcast( Forward<ArgTypes>( Args )... );
    }

    /* Unbind all bound delegates */
    FORCEINLINE void UnbindAll()
    {
        Base::UnbindAll();
    }

    /* Swap */
    FORCEINLINE void Swap( TMulticastDelegate& Other )
    {
        Base::Swap( Other );
    }

    /* Broadcast to all bound delegates */
    FORCEINLINE void operator()( ArgTypes&&... Args )
    {
        return Broadcast( Forward<ArgTypes>( Args )... );
    }
};
