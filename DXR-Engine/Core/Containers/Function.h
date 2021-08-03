#pragma once
#include "CoreTypes.h"
#include "Allocators.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsNullptr.h"

/* TMemberFunction - Encapsulates a member function */
template<typename T, typename InvokableType>
class TMemberFunction;

template<typename T, typename ReturnType, typename... ArgTypes>
class TMemberFunction<T, ReturnType( ArgTypes... )>
{
public:
    typedef ReturnType( T::* TFunctionType )(ArgTypes...);

    TMemberFunction( T* InThis, TFunctionType InFunc ) noexcept
        : This( InThis )
        , Func( InFunc )
    {
    }

    FORCEINLINE ReturnType Invoke( ArgTypes&&... Args ) noexcept
    {
        return ((*This).*Func)(Forward<ArgTypes>( Args )...);
    }

    FORCEINLINE ReturnType operator()( ArgTypes&&... Args ) noexcept
    {
        return Invoke( Forward<ArgTypes>( Args )... );
    }

private:
    T* This;
    TFunctionType Func;
};

/* TConstMemberFunction - Encapsulates a const member function */
template<typename T, typename InvokableType>
class TConstMemberFunction;

template<typename T, typename ReturnType, typename... ArgTypes>
class TConstMemberFunction<T, ReturnType( ArgTypes... )>
{
public:
    typedef ReturnType( T::* TFunctionType )(ArgTypes...) const;

    TConstMemberFunction( const T* InThis, TFunctionType InFunc ) noexcept
        : This( InThis )
        , Func( InFunc )
    {
    }

    FORCEINLINE ReturnType Invoke( ArgTypes&&... Args ) noexcept
    {
        return ((*This).*Func)(Forward<ArgTypes>( Args )...);
    }

    FORCEINLINE ReturnType operator()( ArgTypes&&... Args ) noexcept
    {
        return Invoke( Forward<ArgTypes>( Args )... );
    }

private:
    const T* This;
    TFunctionType Func;
};

/* Bind member function */
template<typename T, typename ReturnType, typename... ArgTypes>
FORCEINLINE TMemberFunction<T, ReturnType( ArgTypes... )> BindFunction( T* This, ReturnType( T::* Func )(ArgTypes...) ) noexcept
{
    return TMemberFunction<T, ReturnType( ArgTypes... )>( This, Func );
}

/* Bind const member function */
template<typename T, typename ReturnType, typename... ArgTypes>
FORCEINLINE TConstMemberFunction<T, ReturnType( ArgTypes... )> BindFunction( const T* This, ReturnType( T::* Func )(ArgTypes...) const ) noexcept
{
    return TConstMemberFunction<T, ReturnType( ArgTypes... )>( This, Func ) );
}

/* TFunction - Encapsulates callables similar to std::function */
template<typename InvokableType>
class TFunction;

template<typename ReturnType, typename... ArgTypes>
class TFunction<ReturnType( ArgTypes... )>
{
private:
    enum
    {
        InlineBytes = 32
    };

    /* Generic functor interface */
    class IFunctor
    {
    public:
        virtual ~IFunctor() = default;

        virtual ReturnType Invoke( ArgTypes&&... Args ) noexcept = 0;
        virtual IFunctor* Clone( void* Memory ) noexcept = 0;
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
        inline virtual ReturnType Invoke( ArgTypes&&... Args ) noexcept override final
        {
            return Functor( Forward<ArgTypes>( Args )... );
        }

        /* Copy construct a clone with memory provided to the function */
        inline virtual IFunctor* Clone( void* Memory ) noexcept override final
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
        ConstructFrom( Forward<FunctorType>( Functor ) );
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
        return Storage.HasAllocation() && (GetFunctor() != nullptr);
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
    FORCEINLINE void Assign( FunctorType&& Functor ) noexcept
    {
        Release();
        ConstructFrom( Forward<FunctorType>( Functor ) );
    }

    /* Invoke the function */
    FORCEINLINE ReturnType Invoke( ArgTypes&&... Args ) noexcept
    {
        return GetFunctor()->Invoke( Forward<ArgTypes>( Args )... );
    }

    /* Invoke the function */
    FORCEINLINE ReturnType operator()( ArgTypes&&... Args ) noexcept
    {
        return Invoke( Forward<ArgTypes>( Args )... );
    }

    /* Cheak weather the function is valid or not */
    FORCEINLINE explicit operator bool() const noexcept
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
    FORCEINLINE void Release() noexcept
    {
        if ( IsValid() )
        {
            GetFunctor()->~IFunctor();
            Storage.Free();
        }
    }

    /* Construct a new functor */
    template<typename FunctorType>
    FORCEINLINE typename TEnableIf<std::is_invocable_v<F, TArgs...>>::Type ConstructFrom( FunctorType&& Functor ) noexcept
    {
        Storage.Allocate( sizeof( FunctorType ) );
        new(Storage.Raw()) TGenericFunctor<FunctorType>( Forward<FunctorType>( Functor ) );
    }

    /* Copy from another function */
    FORCEINLINE void CopyFrom( const TFunction& Other ) noexcept
    {
        Storage.Allocate( Other.Storage.GetSize() );
        Other.Func->Clone( Storage.Raw() ) );
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE IFunctor* GetFunctor() noexcept
    {
        Assert( IsValid() );
        return reinterpret_cast<IFunctor*>(Storage.Raw());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const IFunctor* GetFunctor() const noexcept
    {
        Assert( IsValid() );
        return reinterpret_cast<const IFunctor*>(Storage.Raw());
    }

    TInlineAllocator<int8, InlineBytes> Storage;
};
