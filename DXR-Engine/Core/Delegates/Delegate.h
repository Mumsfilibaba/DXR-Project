#pragma once
#include "DelegateBase.h"

#include "Core/Containers/Allocators.h"
#include "Core/Templates/FunctionType.h"

/* Delegate class, similar to TFunction, but allows direct binding of functions instead of binding a functor type*/
template<typename InvokableType>
class TDelegate;

template<typename ReturnType, typename... ArgTypes>
class TDelegate<ReturnType( ArgTypes... )>
{
    typedef IDelegate<ReturnType, ArgTypes...> IDelegateType;

    typedef ReturnType( *FunctionType )(ArgTypes...);

    template<typename ClassType>
    using MemberFunctionType = typename TMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type;

    template<typename ClassType>
    using ConstMemberFunctionType = typename TConstMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type;

    enum
    {
        // TODO: Look into padding so we can use larger structs?
        InlineBytes = 24
    };

public:

    /* Constructor for a empty delegate */
    FORCEINLINE TDelegate()
        : Storage()
    {
    }

    /* Copy constructor */
    FORCEINLINE TDelegate( const TDelegate& Other )
        : Storage()
    {
        CopyFrom( Other );
    }

    /* Move constructor */
    FORCEINLINE TDelegate( TDelegate&& Other )
        : Storage( Move( Other.Storage ) )
    {
    }

    FORCEINLINE ~TDelegate()
    {
        Unbind();
    }

    /* Bind "normal" function */
    FORCEINLINE void BindStatic( FunctionType Function )
    {
        Bind<typename TFunctionDelegate<ReturnType, ArgTypes...>>( Function );
    }

    /* Bind member function */
    template<typename InstanceType>
    FORCEINLINE void BindRaw( InstanceType* This, MemberFunctionType<InstanceType> Function )
    {
        Bind<typename TMemberDelegate<InstanceType, InstanceType, ReturnType, ArgTypes...>>( This, Function );
    }

    /* Bind member function */
    template<typename InstanceType, typename ClassType>
    FORCEINLINE void BindRaw( InstanceType* This, MemberFunctionType<ClassType> Function )
    {
        Bind<typename TMemberDelegate<InstanceType, ClassType, ReturnType, ArgTypes...>>( This, Function );
    }

    /* Bind const member function */
    template<typename InstanceType>
    FORCEINLINE void BindRaw( const InstanceType* This, ConstMemberFunctionType<InstanceType> Function )
    {
        Bind<typename TConstMemberDelegate<InstanceType, InstanceType, ReturnType, ArgTypes...>>( This, Function );
    }

    /* Bind const member function */
    template<typename InstanceType, typename ClassType>
    FORCEINLINE void BindRaw( const InstanceType* This, ConstMemberFunctionType<ClassType> Function )
    {
        Bind<typename TConstMemberDelegate<InstanceType, ClassType, ReturnType, ArgTypes...>>( This, Function );
    }

    /* Bind Lambda or other functor */
    template<typename FunctorType>
    FORCEINLINE void BindLambda( FunctorType Functor )
    {
        Bind<typename TLambdaDelegate<FunctorType, ReturnType, ArgTypes...>>( Forward<FunctorType>( Functor ) );
    }

    /* Unbinds the delegate */
    FORCEINLINE void Unbind()
    {
        Release();
    }

    /* Executes the delegate */
    FORCEINLINE ReturnType Execute( ArgTypes... Args )
    {
        Assert( IsBound() );
        return GetDelegate()->Execute( Forward<ArgTypes>( Args )... );
    }

    /* Executes the delegate if there is any bound */
    FORCEINLINE bool ExecuteIfBound( ArgTypes... Args )
    {
        if ( IsBound() )
        {
            GetDelegate()->Execute( Forward<ArgTypes>( Args )... );
            return true;
        }
        else
        {
            return false;
        }
    }

    /* Swaps two delegates */
    FORCEINLINE void Swap( TDelegate& Other )
    {
        TInlineAllocator<int8, InlineBytes> TempStorage( Move( Storage ) );
        Storage = Move( Other.Storage );
        Other.Storage = Move( TempStorage );
    }

    /* Cheacks weather or not there exist any delegate bound */
    FORCEINLINE bool IsBound() const
    {
        return Storage.HasAllocation();
    }

    /* Retrive the owner, returns nullptr for non-member delegates */
    FORCEINLINE const void* GetOwner() const
    {
        Assert( IsBound() );
        return GetDelegate()->GetOwner();
    }

    /* Execute operator */
    FORCEINLINE ReturnType operator()( ArgTypes... Args )
    {
        return Execute( Forward<ArgTypes>( Args )... );
    }

    /* Move assignment */
    FORCEINLINE TDelegate& operator=( TDelegate&& RHS )
    {
        TDelegate( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    FORCEINLINE TDelegate& operator=( const TDelegate& RHS )
    {
        TDelegate( RHS ).Swap( *this );
        return *this;
    }

    /* Check if valid */
    FORCEINLINE operator bool() const
    {
        return IsBound();
    }

private:

    /* Release the delegate */
    FORCEINLINE void Release()
    {
        if ( Storage.HasAllocation() )
        {
            reinterpret_cast<CGenericDelegate*>(GetDelegate())->~CGenericDelegate();
            Storage.Free();
        }
    }

    /* Copy from another function */
    FORCEINLINE void CopyFrom( const TDelegate& Other ) noexcept
    {
        if ( Other.IsBound() )
        {
            Storage.Allocate( Other.Storage.GetSize() );
            Other.GetDelegate()->Clone( Storage.Raw() );
        }
    }

    template<typename DelegateType, typename... ConstructorArgs>
    FORCEINLINE void Bind( ConstructorArgs&&... args )
    {
        Release();

        void* Memory = Storage.Allocate( sizeof( DelegateType ) );
        new (Memory) DelegateType( Forward<ConstructorArgs>( args )... );
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE IDelegateType* GetDelegate() noexcept
    {
        return reinterpret_cast<IDelegateType*>(Storage.Raw());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const IDelegateType* GetDelegate() const noexcept
    {
        return reinterpret_cast<const IDelegateType*>(Storage.Raw());
    }

    // TODO: Should allocator use the element type at all? 
    TInlineAllocator<int8, InlineBytes> Storage;
};

/* Helper for creating a static delegate */
template<typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> StaticDelegate( typename TFunctionType<ReturnType, ArgTypes...>::Type Function )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindStatic( Function );
    return Delegate;
}

/* Helper for creating a member delegate */
template<typename InstanceType, typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> RawDelegate( InstanceType* This, typename TMemberFunctionType<InstanceType, ReturnType, ArgTypes...>::Type Function )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindRaw( Function );
    return Delegate;
}

/* Helper for creating a member delegate */
template<typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> RawDelegate( InstanceType* This, typename TMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type Function )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindRaw( This, Function );
    return Delegate;
}

/* Helper for creating a const member delegate */
template<typename InstanceType, typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> RawDelegate( const InstanceType* This, typename TConstMemberFunctionType<InstanceType, ReturnType, ArgTypes...>::Type Function )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindRaw( This, Function );
    return Delegate;
}

/* Helper for creating a const member delegate */
template<typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> RawDelegate( const InstanceType* This, typename TConstMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type Function )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindRaw( This, Function );
    return Delegate;
}

/* Helper for creating a lambda delegate */
template<typename FunctorType, typename ReturnType, typename... ArgTypes>
inline TDelegate<ReturnType( ArgTypes... )> LambdaDelegate( FunctorType Functor )
{
    TDelegate<ReturnType( ArgTypes... )> Delegate;
    Delegate.BindLambda( Functor );
    return Delegate;
}