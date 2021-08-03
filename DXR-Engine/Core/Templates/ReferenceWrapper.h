#pragma once
#include "IsObject.h"
#include "IsFunction.h"
#include "Invoke.h"
#include "AddressOf.h"

/* Wrapper for a reference making them copyable */
template<typename T>
class TReferenceWrapper
{
public:
    typedef typename TRemoveReference<T>::Type Type;

    TReferenceWrapper( const TReferenceWrapper& ) = default;
    TReferenceWrapper( TReferenceWrapper&& ) = default;
    TReferenceWrapper& operator=( const TReferenceWrapper& ) = default;
    TReferenceWrapper& operator=( TReferenceWrapper&& ) = default;

    static_assert(TIsObject<T>::Value || TIsFunction<T>::Value, "TReferenceWrapper requires T to be of object or function type");

    /* Constructor */
    template<typename OtherType, typename =
        typename TEnableIf<TIsSame<typename TRemoveCV<Type>::Type, typename TRemoveCV<typename TRemoveReference<OtherType>::Type>::Type>::Value>::Type>
        FORCEINLINE TReferenceWrapper( OtherType&& In )
        : Pointer( ::AddressOf( In ) )
    {
    }

    /* Retrive reference */
    FORCEINLINE Type& Get() const noexcept
    {
        return *Pointer;
    }

    /* Retrive the raw pointer */
    FORCEINLINE Type* AddressOf() const noexcept
    {
        return Pointer;
    }

    /* Retrive reference */
    FORCEINLINE operator Type&() const noexcept
    {
        return Get();
    }

    /* Invoke if type is invokable */
    template<typename... ArgTypes>
    FORCEINLINE auto operator()( ArgTypes&&... Args ) const noexcept
        -> decltype(Invoke( *Pointer, Forward<ArgTypes>( Args )... ))
    {
        return Invoke( *Pointer, Forward<ArgTypes>( Args )... );
    }

private:
    Type* Pointer;
};
