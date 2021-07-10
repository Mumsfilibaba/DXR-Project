#pragma once
#include "Utilities.h"
#include "Allocator.h"

// TMemberFunction - Encapsulates a member function

template<typename T, typename TInvokable>
class TMemberFunction;

template<typename T, typename TReturn, typename... TArgs>
class TMemberFunction<T, TReturn( TArgs... )>
{
public:
    typedef TReturn( T::* TFunctionType )(TArgs...);

    TMemberFunction( T* InThis, TFunctionType InFunc ) noexcept
        : This( InThis )
        , Func( InFunc )
    {
    }

    TReturn Invoke( TArgs&&... Args ) noexcept
    {
        return ((*This).*Func)(Forward<TArgs>( Args )...);
    }

    FORCEINLINE TReturn operator()( TArgs&&... Args ) noexcept
    {
        return Invoke( Forward<TArgs>( Args )... );
    }

private:
    T* This;
    TFunctionType Func;
};

// TConstMemberFunction - Encapsulates a const member function

template<typename T, typename TInvokable>
class TConstMemberFunction;

template<typename T, typename TReturn, typename... TArgs>
class TConstMemberFunction<T, TReturn( TArgs... )>
{
public:
    typedef TReturn( T::* TFunctionType )(TArgs...) const;

    TConstMemberFunction( const T* InThis, TFunctionType InFunc ) noexcept
        : This( InThis )
        , Func( InFunc )
    {
    }

    TReturn Invoke( TArgs&&... Args ) noexcept
    {
        return ((*This).*Func)(Forward<TArgs>( Args )...);
    }

    FORCEINLINE TReturn operator()( TArgs&&... Args ) noexcept
    {
        return Invoke( Forward<TArgs>( Args )... );
    }

private:
    const T* This;
    TFunctionType Func;
};

// BindFunction

template<typename T, typename TReturn, typename... TArgs>
inline TMemberFunction<T, TReturn( TArgs... )> BindFunction( T* This, TReturn( T::* Func )(TArgs...) ) noexcept
{
    return ::Move( TMemberFunction<T, TReturn( TArgs... )>( This, Func ) );
}

template<typename T, typename TReturn, typename... TArgs>
inline TConstMemberFunction<T, TReturn( TArgs... )> BindFunction( const T* This, TReturn( T::* Func )(TArgs...) const ) noexcept
{
    return ::Move( TConstMemberFunction<T, TReturn( TArgs... )>( This, Func ) );
}

// TFunction - Encapsulates callables similar to std::function

template<typename TInvokable>
class TFunction;

template<typename TReturn, typename... TArgs>
class TFunction<TReturn( TArgs... )>
{
private:
    class IFunctor
    {
    public:
        virtual ~IFunctor() = default;

        virtual TReturn Invoke( TArgs&&... Args ) noexcept = 0;

        virtual IFunctor* Clone( void* Memory ) noexcept = 0;
        virtual IFunctor* Move( void* Memory ) noexcept = 0;
    };

    template<typename F>
    class TGenericFunctor : public IFunctor
    {
    public:
        TGenericFunctor( const F& InFunctor ) noexcept
            : IFunctor()
            , mFunctor( InFunctor )
        {
        }

        TGenericFunctor( const TGenericFunctor& Other ) noexcept
            : IFunctor()
            , mFunctor( Other.mFunctor )
        {
        }

        TGenericFunctor( TGenericFunctor&& Other ) noexcept
            : IFunctor()
            , mFunctor( ::Move( Other.mFunctor ) )
        {
            if constexpr ( std::is_pointer<F>() )
            {
                Other.mFunctor = nullptr;
            }
        }

        virtual TReturn Invoke( TArgs&&... Args ) noexcept override final
        {
            return mFunctor( Forward<TArgs>( Args )... );
        }

        virtual IFunctor* Clone( void* Memory ) noexcept override final
        {
            return new(Memory) TGenericFunctor( *this );
        }

        virtual IFunctor* Move( void* Memory ) noexcept override final
        {
            return new(Memory) TGenericFunctor( ::Move( *this ) );
        }

    private:
        F mFunctor;
    };

public:
    TFunction() noexcept
        : Func( nullptr )
        , StackBuffer()
        , StackAllocated( true )
    {
    }

    TFunction( std::nullptr_t ) noexcept
        : Func( nullptr )
        , StackBuffer()
        , StackAllocated( true )
    {
    }

    template<typename F>
    TFunction( F Functor ) noexcept
        : Func( nullptr )
        , StackBuffer()
        , StackAllocated( true )
    {
        InternalConstruct( Forward<F>( Functor ) );
    }

    TFunction( const TFunction& Other ) noexcept
        : Func( nullptr )
        , StackBuffer()
        , StackAllocated( true )
    {
        InternalCopyConstruct( Other );
    }

    TFunction( TFunction&& Other ) noexcept
        : Func( nullptr )
        , StackBuffer()
        , StackAllocated( true )
    {
        InternalMoveConstruct( Forward<TFunction>( Other ) );
    }

    ~TFunction()
    {
        InternalRelease();
    }

    void Swap( TFunction& Other ) noexcept
    {
        TFunction TempFunc( *this );
        *this = Other;
        Other = TempFunc;
    }

    template<typename F>
    void Assign( F&& Functor ) noexcept
    {
        InternalRelease();
        InternalConstruct( Forward<F>( Functor ) );
    }

    TReturn Invoke( TArgs&&... Args ) noexcept
    {
        Assert( Func != nullptr );
        return Func->Invoke( Forward<TArgs>( Args )... );
    }

    TReturn operator()( TArgs&&... Args ) noexcept
    {
        return Invoke( Forward<TArgs>( Args )... );
    }

    explicit operator bool() const noexcept
    {
        return (Func != nullptr);
    }

    TFunction& operator=( const TFunction& Other ) noexcept
    {
        if ( this != &Other )
        {
            InternalRelease();
            InternalCopyConstruct( Other );
        }

        return *this;
    }

    TFunction& operator=( TFunction&& Other ) noexcept
    {
        if ( this != &Other )
        {
            InternalRelease();
            InternalMoveConstruct( Other );
        }

        return *this;
    }

    TFunction& operator=( std::nullptr_t ) noexcept
    {
        InternalRelease();
        return *this;
    }

private:
    void InternalRelease() noexcept
    {
        if ( Func )
        {
            if ( StackAllocated )
            {
                Func->~IFunctor();
            }
            else
            {
                delete Func;
            }

            Func = nullptr;
        }
    }

    template<typename F>
    TEnableIf<std::is_invocable_v<F, TArgs...>> InternalConstruct( F&& Functor ) noexcept
    {
        if constexpr ( CanStackAllocate<F>() )
        {
            Func = new(reinterpret_cast<void*>(StackBuffer)) TGenericFunctor<F>( Forward<F>( Functor ) );
            StackAllocated = true;
        }
        else
        {
            Func = new TGenericFunctor<F>( Forward<F>( Functor ) );
            SizeInBytes = sizeof( TGenericFunctor<F> );
            StackAllocated = false;
        }
    }

    void InternalMoveConstruct( TFunction&& Other ) noexcept
    {
        if ( Other.StackAllocated )
        {
            Func = Other.Func->Move( reinterpret_cast<void*>(StackBuffer) );
            StackAllocated = true;
        }
        else
        {
            Func = Other.Func->Move( malloc( Other.SizeInBytes ) );
            SizeInBytes = Other.SizeInBytes;
            StackAllocated = false;
        }
    }

    void InternalCopyConstruct( const TFunction& Other ) noexcept
    {
        if ( Other.StackAllocated )
        {
            Func = Other.Func->Clone( reinterpret_cast<void*>(StackBuffer) );
            StackAllocated = true;
        }
        else
        {
            Func = Other.Func->Clone( malloc( Other.SizeInBytes ) );
            SizeInBytes = Other.SizeInBytes;
            StackAllocated = false;
        }
    }

    template<typename F>
    static constexpr bool CanStackAllocate() noexcept
    {
        constexpr uint32 StackSize = sizeof( StackBuffer );
        constexpr uint32 FunctorSize = sizeof( TGenericFunctor<F> );
        return FunctorSize <= StackSize;
    }

private:
    IFunctor* Func = nullptr;
    bool StackAllocated = true;
    union
    {
        char   StackBuffer[23];
        uint32 SizeInBytes;
    };
};
