#pragma once
#include "CoreTypes.h"
#include "Allocators.h"
#include "Tuple.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/IsInvokable.h"
#include "Core/Templates/FunctionType.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/Decay.h"

/* A payloaded function */
template<typename FunctionType, typename... PayloadTypes>
class TBindPayload
{
public:

    /* Constructor */
    FORCEINLINE TBindPayload( FunctionType InFunc, PayloadTypes&&... PayloadArgs ) noexcept
        : Payload( Forward<PayloadTypes>( PayloadArgs )... )
        , Func( Move( InFunc ) )
    {
    }

    /* Invoke function */
    template<typename... ArgTypes>
    FORCEINLINE auto Execute( ArgTypes&&... Args ) noexcept
    {
        return Payload.ApplyBefore( Func, Forward<ArgTypes>( Args )... );
    }

    /* Operator for invoking function */
    template<typename... ArgTypes>
    FORCEINLINE auto operator()( ArgTypes&&... Args ) noexcept
    {
        return Execute( Forward<ArgTypes>( Args )... );
    }

private:

    /* Arguments stored when calling bind and then applied to the function when invoked */
    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    /* Function pointer */
    FunctionType Func;
};

/* Empty payloaded function */
template<typename FunctionType>
class TBindPayload<FunctionType>
{
public:

    /* Constructor */
    FORCEINLINE TBindPayload( FunctionType InFunc ) noexcept
        : Func( Move( InFunc ) )
    {
    }

    /* Invoke function */
    template<typename... ArgTypes>
    FORCEINLINE auto Execute( ArgTypes&&... Args ) noexcept
    {
        return Invoke( Func, Forward<ArgTypes>( Args )... );
    }

    /* Operator for invoking function */
    template<typename... ArgTypes>
    FORCEINLINE auto operator()( ArgTypes&&... Args ) noexcept
    {
        return Execute( Forward<ArgTypes>( Args )... );
    }

private:

    /* Function pointer */
    FunctionType Func;
};


/* Bind a function with payload */
template<typename FunctionType, typename... ArgTypes>
FORCEINLINE auto Bind( FunctionType Function, ArgTypes&&... Args )
{
    return TBindPayload<FunctionType, ArgTypes...>( Function, Forward<ArgTypes>( Args )... );
}

/* TFunction - Encapsulates callables similar to std::function */
template<typename InvokableType>
class TFunction;

template<typename ReturnType, typename... ArgTypes>
class TFunction<ReturnType( ArgTypes... )>
{
    enum
    {
        // TODO: Look into padding so we can use larger structs?
        InlineBytes = 24
    };

    // TODO: Should allocator use the element type at all? 
    using AllocatorType = TInlineAllocator<int8, InlineBytes>;

    /* Generic functor interface */
    class IFunctor
    {
    public:
        virtual ~IFunctor() = default;

        virtual ReturnType Invoke( ArgTypes&&... Args ) noexcept = 0;
        virtual IFunctor* Clone( void* Memory ) const noexcept = 0;
    };

    /* Generic functor implementation */
    template<typename FunctorType>
    class TGenericFunctor : public IFunctor
    {
    public:

        /* Constructor */
        FORCEINLINE TGenericFunctor( const FunctorType& InFunctor ) noexcept
            : IFunctor()
            , Functor( InFunctor )
        {
        }

        /* Copy constructor */
        FORCEINLINE TGenericFunctor( const TGenericFunctor& Other ) noexcept
            : IFunctor()
            , Functor( Other.Functor )
        {
        }

        /* Move constructor */
        FORCEINLINE TGenericFunctor( TGenericFunctor&& Other ) noexcept
            : IFunctor()
            , Functor( Move( Other.Functor ) )
        {
            Memory::Memzero<TGenericFunctor>( &Other );
        }

        /* Invoke the functor */
        virtual ReturnType Invoke( ArgTypes&&... Args ) noexcept override final
        {
            return Functor( Forward<ArgTypes>( Args )... );
        }

        /* Copy construct a clone with memory provided to the function */
        virtual IFunctor* Clone( void* Memory ) const noexcept override final
        {
            return new(Memory) TGenericFunctor( *this );
        }

    private:
        FunctorType Functor;
    };

public:

    /* Default constructor */
    FORCEINLINE TFunction() noexcept
        : Storage()
    {
    }

    /* Create from nullptr */
    FORCEINLINE TFunction( NullptrType ) noexcept
        : Storage()
    {
    }

    /* Create from functor */
    template<typename FunctorType>
    FORCEINLINE TFunction( FunctorType Functor ) noexcept
        : Storage()
    {
        ConstructFrom<FunctorType>( Forward<FunctorType>( Functor ) );
    }

    /* Copy constructor */
    FORCEINLINE TFunction( const TFunction& Other ) noexcept
        : Storage()
    {
        CopyFrom( Other );
    }

    /* Move constructor */
    FORCEINLINE TFunction( TFunction&& Other ) noexcept
        : Storage( Move( Other.Storage ) )
    {
    }

    FORCEINLINE ~TFunction()
    {
        Release();
    }

    /* Cheacks weather the function is valid or not */
    FORCEINLINE bool IsValid() const noexcept
    {
        return Storage.HasAllocation();
    }

    /* Swap this and another function */
    FORCEINLINE void Swap( TFunction& Other ) noexcept
    {
        TFunction Temp( Move( *this ) );
        *this = Move( Other );
        Other = Move( Temp );
    }

    /* Assign a new functor */
    template<typename FunctorType >
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ArgTypes...>::Value>::Type Assign( FunctorType&& Functor ) noexcept
    {
        Release();
        ConstructFrom<FunctorType>( Forward<FunctorType>( Functor ) );
    }

    /* Invoke the function */
    FORCEINLINE ReturnType Invoke( ArgTypes&&... Args ) noexcept
    {
        IsValid();
        return GetFunctor()->Invoke( Forward<ArgTypes>( Args )... );
    }

    /* Invoke the function */
    FORCEINLINE ReturnType operator()( ArgTypes&&... Args ) noexcept
    {
        return Invoke( Forward<ArgTypes>( Args )... );
    }

    /* Cheak weather the function is valid or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy assignment */
    FORCEINLINE TFunction& operator=( const TFunction& Other ) noexcept
    {
        if ( this != &Other )
        {
            Release();
            CopyFrom( Other );
        }

        return *this;
    }

    /* Move assignment */
    FORCEINLINE TFunction& operator=( TFunction&& Other ) noexcept
    {
        if ( this != &Other )
        {
            Release();
            Storage = Move( Other.Storage );
        }

        return *this;
    }

    /* Nullptr assignment */
    FORCEINLINE TFunction& operator=( NullptrType ) noexcept
    {
        Release();
        return *this;
    }

private:

    /* Release the function */
    FORCEINLINE void Release() noexcept
    {
        if ( Storage.HasAllocation() )
        {
            GetFunctor()->~IFunctor();
            Storage.Free();
        }
    }

    /* Construct a new functor */
    template<typename FunctorType>
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ArgTypes...>::Value>::Type ConstructFrom( FunctorType&& Functor ) noexcept
    {
        Release();

        void* Memory = Storage.Allocate( sizeof( TGenericFunctor<FunctorType> ) );
        new(Memory) TGenericFunctor<FunctorType>( Forward<FunctorType>( Functor ) );
    }

    /* Copy from another function */
    FORCEINLINE void CopyFrom( const TFunction& Other ) noexcept
    {
        if ( Other.IsValid() )
        {
            Storage.Allocate( Other.Storage.GetSize() );
            Other.GetFunctor()->Clone( Storage.Raw() );
        }
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE IFunctor* GetFunctor() noexcept
    {
        return reinterpret_cast<IFunctor*>(Storage.Raw());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const IFunctor* GetFunctor() const noexcept
    {
        return reinterpret_cast<const IFunctor*>(Storage.Raw());
    }

    AllocatorType Storage;
};
