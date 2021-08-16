#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/FunctionType.h"

/* Basetype for delegates */
class CGenericDelegate
{
public:
    CGenericDelegate() = default;
    virtual ~CGenericDelegate() noexcept = default;

    /* Retrive the owner of the function, returns nullptr for non-member delegates */
    virtual const void* GetOwner() const
    {
        return nullptr;
    }
};

/* Types basetype for delegates */
template<typename ReturnType, typename... ArgTypes>
class IDelegate : public CGenericDelegate
{
public:
    /* Executes the delegates and calls the stored function or functor */
    virtual ReturnType Execute( ArgTypes... Args ) const = 0;

    /* Clones the delegate and stores it in the specified memory */
    virtual IDelegate* Clone( void* Memory ) const = 0;
};

/* Class for storing a "normal" function in a delegate */
template<typename ReturnType, typename... ArgTypes>
class TFunctionDelegate : public IDelegate<ReturnType, ArgTypes...>
{
public:
    typedef ReturnType( *FunctionType )(ArgTypes...);
    typedef typename IDelegate<ReturnType, ArgTypes...> Base;

    /* Constructor taking a function */
    FORCEINLINE TFunctionDelegate( FunctionType InFunction )
        : Base()
        , Function( InFunction )
    {
    }

    /* Execute the function */
    virtual ReturnType Execute( ArgTypes... Args ) const override final
    {
        return Function( Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Base* Clone( void* Memory ) const override final
    {
        return new(Memory) TFunctionDelegate( Function );
    }

private:
    FunctionType Function;
};

/* Stores a memberfunction as a delegate */
template<typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
class TMemberDelegate : public IDelegate<ReturnType, ArgTypes...>
{
public:
    typedef typename IDelegate<ReturnType, ArgTypes...> Base;
    typedef typename TMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type MemberFunctionType;

    /* Constructor */
    FORCEINLINE TMemberDelegate( InstanceType* InThis, MemberFunctionType InFunction )
        : Base()
        , This( InThis )
        , Function( InFunction )
    {
        Assert( This != nullptr );
    }

    /* Execute function */
    virtual ReturnType Execute( ArgTypes... Args ) const override final
    {
        return ((*This).*Function)(Forward<ArgTypes>( Args )...);
    }

    /* Clone this instance and store in the memory */
    virtual Base* Clone( void* Memory ) const override final
    {
        return new(Memory) TMemberDelegate( This, Function );
    }

    /* Returns this */
    virtual const void* GetOwner() const
    {
        return reinterpret_cast<const void*>(This);
    }

private:
    InstanceType* This = nullptr;
    MemberFunctionType Function;
};

/* Stores a const memberfunction as a delegate */
template<typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
class TConstMemberDelegate : public IDelegate<ReturnType, ArgTypes...>
{
public:
    typedef typename IDelegate<ReturnType, ArgTypes...> Base;
    typedef typename TConstMemberFunctionType<ClassType, ReturnType, ArgTypes...>::Type MemberFunctionType;

    /* Constructor */
    FORCEINLINE TConstMemberDelegate( const InstanceType* InThis, MemberFunctionType InFunction )
        : Base()
        , This( InThis )
        , Function( InFunction )
    {
        Assert( This != nullptr );
    }

    /* Execute function */
    virtual ReturnType Execute( ArgTypes... Args ) const override
    {
        return ((*This).*Function)(Forward<ArgTypes>( Args )...);
    }

    /* Clone this instance and store in the memory */
    virtual Base* Clone( void* Memory ) const override
    {
        return new(Memory) TConstMemberDelegate( This, Function );
    }

    /* Returns this */
    virtual const void* GetOwner() const
    {
        return reinterpret_cast<const void*>(This);
    }

private:
    const InstanceType* This = nullptr;
    MemberFunctionType Function;
};

/* Stores a lambda delegate */
template<typename FunctorType, typename ReturnType, typename... ArgTypes>
class TLambdaDelegate : public IDelegate<ReturnType, ArgTypes...>
{
public:
    typedef typename IDelegate<ReturnType, ArgTypes...> Base;

    /* Constructor */
    FORCEINLINE TLambdaDelegate( FunctorType InInvokable )
        : Base()
        , Functor( InInvokable )
    {
    }

    /* Execute Functor */
    virtual ReturnType Execute( ArgTypes... Args ) const override
    {
        return Functor( Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Base* Clone( void* Memory ) const override
    {
        return new(Memory) TLambdaDelegate( Functor );
    }

private:
    FunctorType Functor;
};