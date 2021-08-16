#pragma once
#include "Delegate.h"

#include "Core/Containers/Array.h"

/* Handle type */
typedef int64 DelegateHandle;

/* A handle for a multicast-delegate */
class CDelegateHandle
{
public:
    enum class EGenerateID
    {
        New
    };

    /* Empty constructor, makes it storable */
    FORCEINLINE CDelegateHandle()
        : Handle( -1 )
    {
    }

    FORCEINLINE explicit CDelegateHandle( EGenerateID )
        : Handle( GenerateID() )
    {
    }

    /* Checks if the handle is equal to nullptr */
    FORCEINLINE bool IsValid() const
    {
        return Handle != -1;
    }

    /* Retrive the ID */
    FORCEINLINE DelegateHandle GetHandle() const
    {
        return Handle;
    }

    /* Checks if the handle is equal to nullptr */
    FORCEINLINE operator bool() const
    {
        return IsValid();
    }

    /* Checks equallity between two handles */
    FORCEINLINE bool operator==( CDelegateHandle RHS ) const
    {
        return (Handle == RHS.Handle);
    }

    /* Checks equallity between two handles */
    FORCEINLINE bool operator!=( CDelegateHandle RHS ) const
    {
        return !(*this == RHS);
    }

private:
    DelegateHandle Handle;

    //TODO: This needs to be exported when using DLLs
    inline static DelegateHandle NextID = 0;

    /* Generates a new ID */
    FORCEINLINE static DelegateHandle GenerateID()
    {
        return ++NextID;
    }
};

/* Base of multicast delegate */
template<typename... ArgTypes>
class TMulticastBase
{
    using TDelegateType = TDelegate<void( ArgTypes... )>;
    using FunctionType = typename TFunctionType<void, ArgTypes...>::Type;

    template<typename ClassType>
    using MemberFunctionType = typename TMemberFunctionType<ClassType, void, ArgTypes...>::Type;
    template<typename ClassType>
    using ConstMemberFunctionType = typename TConstMemberFunctionType<ClassType, void, ArgTypes...>::Type;

    /* Struct to store delegates in */
    struct SDelegateAndHandle
    {
        TDelegateType   Delegate;
        CDelegateHandle Handle;
    };

public:

    /* Empty constructor */
    FORCEINLINE TMulticastBase()
        : Delegates()
    {
    }

    /* Copy constructor */
    FORCEINLINE TMulticastBase( const TMulticastBase& Other )
        : Delegates( Other.Delegates )
    {
    }

    /* Move constructor */
    FORCEINLINE TMulticastBase( TMulticastBase&& Other )
        : Delegates( Move( Other.Delegates ) )
    {
    }

    /* Destructor Unbind all delegates */
    FORCEINLINE ~TMulticastBase()
    {
        UnbindAll();
    }

    /* Bind "normal" function */
    FORCEINLINE CDelegateHandle AddStatic( FunctionType Function )
    {
        return PushDelegate( StaticDelegate<void, ArgTypes...>( Function ) );
    }

    /* Bind member function */
    template<typename InstanceType>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, MemberFunctionType<InstanceType> Function )
    {
        return PushDelegate( RawDelegate<InstanceType, InstanceType, void, ArgTypes...>( This, Function ) );
    }

    /* Bind member function */
    template<typename InstanceType, typename ClassType>
    FORCEINLINE CDelegateHandle AddRaw( InstanceType* This, MemberFunctionType<ClassType> Function )
    {
        return PushDelegate( RawDelegate<InstanceType, ClassType, void, ArgTypes...>( This, Function ) );
    }

    /* Bind const member function */
    template<typename InstanceType>
    FORCEINLINE CDelegateHandle AddRaw( const InstanceType* This, ConstMemberFunctionType<InstanceType> Function )
    {
        return PushDelegate( RawDelegate<InstanceType, InstanceType, void, ArgTypes...>( This, Function ) );
    }

    /* Bind const member function */
    template<typename InstanceType, typename ClassType>
    FORCEINLINE CDelegateHandle AddRaw( const InstanceType* This, ConstMemberFunctionType<ClassType> Function )
    {
        return PushDelegate( RawDelegate<InstanceType, ClassType, void, ArgTypes...>( This, Function ) );
    }

    /* Bind Lambda or other functor */
    template<typename FunctorType>
    FORCEINLINE CDelegateHandle AddLambda( FunctorType Functor )
    {
        return PushDelegate( LambdaDelegate<FunctorType, void, ArgTypes...>( Functor ) );
    }

    /* Add a "standard" delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle AddDelegate( const TDelegateType& Delegate )
    {
        return PushDelegate( Delegate );
    }

    /* Unbind a handle */
    FORCEINLINE void Unbind( CDelegateHandle Handle )
    {
        for ( auto It = Delegates.StartIterator(); It != Delegates.EndItertator(); It++ )
        {
            if ( Handle == CDelegateHandle( It->Handle ) )
            {
                Delegates.Erase( It );
                return;
            }
        }
    }

    /* Checks if a delegate is bound */
    FORCEINLINE bool IsBound() const
    {
        return !Delegates.IsEmpty();
    }

    /* Checks if a delegate is bound */
    FORCEINLINE operator bool() const
    {
        return IsBound();
    }

    /* Move assignment */
    FORCEINLINE TMulticastBase& operator=( TMulticastBase&& RHS )
    {
        TMulticastDelegate( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    FORCEINLINE TMulticastBase& operator=( const TMulticastBase& RHS )
    {
        TMulticastBase( RHS ).Swap( *this );
        return *this;
    }

protected:

    /* Broadcast to all bound delegates */
    FORCEINLINE void Broadcast( ArgTypes&&... Args )
    {
        for ( SDelegateAndHandle& Pair : Delegates )
        {
            Pair.Delegate.Execute( Forward<ArgTypes>( Args )... );
        }
    }

    /* Unbind all bound delegates */
    FORCEINLINE void UnbindAll()
    {
        for ( SDelegateAndHandle& Pair : Delegates )
        {
            Pair.Delegate.Unbind();
        }

        Delegates.Clear();
    }

    /* Swap */
    FORCEINLINE void Swap( TMulticastBase& Other )
    {
        TArray<TDelegateType> TempDelegates( Move( Delegates ) );
        Delegates = Move( Other.Delegates );
        Other.Delegates = Move( TempDelegates );
    }

    /* Add a new delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle PushDelegate( const TDelegateType& NewDelegate )
    {
        SDelegateAndHandle& NewPair = Delegates.PushBack( { NewDelegate, GetNextID() } );
        return CDelegateHandle( NewPair.Handle );
    }

    TArray<SDelegateAndHandle> Delegates;
};