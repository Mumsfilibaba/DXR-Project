#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/FunctionType.h"

/* Handle type */
typedef int64 DelegateHandle;

/* A handle for a multicast-delegate */
class CDelegateHandle
{
    enum
    {
        InvalidHandle = -1
    };

public:
    enum class EGenerateID
    {
        New
    };

    /* Empty constructor, makes it storable */
    FORCEINLINE CDelegateHandle()
        : Handle( InvalidHandle )
    {
    }

    FORCEINLINE explicit CDelegateHandle( EGenerateID )
        : Handle( GenerateID() )
    {
    }

    /* Checks if the handle is equal to nullptr */
    FORCEINLINE bool IsValid() const
    {
        return (Handle != InvalidHandle);
    }

    /* Sets the internal handle to an invalid one*/
    FORCEINLINE void Reset()
    {
        Handle = InvalidHandle;
    }

    /* Retrive the ID */
    FORCEINLINE DelegateHandle GetNative() const
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

    /* Generates a new ID */
    FORCEINLINE static DelegateHandle GenerateID()
    {
        return ++NextID;
    }

    DelegateHandle Handle;

    //TODO: This needs to be exported when using DLLs
    inline static DelegateHandle NextID = 0;
};

/* Basetype for delegates */
class IDelegateInstance
{
public:
    IDelegateInstance() = default;
    virtual ~IDelegateInstance() noexcept = default;

    /* Retrive the object of the function, returns nullptr for non-member delegates */
    virtual const void* GetBoundObject() const = 0;

    /* Check if the object is the one that is bound to the delegate instance */
    virtual bool IsObjectBound( const void* ) const = 0;

    /* Retrive the handle to the delegate */
    virtual CDelegateHandle GetHandle() const = 0;

    /* Clones the delegate and stores it in the specified memory */
    virtual IDelegateInstance* Clone( void* Memory ) const = 0;
};

/* Types basetype for delegates */
template<typename ReturnType, typename... ArgTypes>
class TDelegateInstance : public IDelegateInstance
{
public:

    /* Executes the delegates and calls the stored function or functor */
    virtual ReturnType Execute( ArgTypes&&... Args ) = 0;

public:

    /* Retrive the object of the function, returns nullptr for non-member delegates */
    virtual const void* GetBoundObject() const override
    {
        return nullptr;
    }

    /* Check if the object is the one that is bound to the delegate instance */
    virtual bool IsObjectBound( const void* ) const override
    {
        return false;
    }

    /* Retrive the handle to the delegate */
    virtual CDelegateHandle GetHandle() const override final
    {
        return Handle;
    }

protected:
    FORCEINLINE TDelegateInstance()
        : IDelegateInstance()
        , Handle( CDelegateHandle::EGenerateID::New )
    {
    }

    CDelegateHandle Handle;
};

/* Class for storing a "normal" function in a delegate */
template<typename FunctionType, typename... PayloadTypes>
class TFunctionDelegateInstance;

template<typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TFunctionDelegateInstance<ReturnType( ArgTypes... ), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType( ArgTypes..., PayloadTypes... )>::Type;

public:

    TFunctionDelegateInstance( const TFunctionDelegateInstance& ) = default;

    /* Constructor taking a function */
    FORCEINLINE TFunctionDelegateInstance( FunctionType InFunction, PayloadTypes&&... InPayload )
        : Super()
        , Function( InFunction )
        , Payload( Forward<PayloadTypes>( InPayload )... )
    {
    }

    /* Execute the function */
    virtual ReturnType Execute( ArgTypes&&... Args ) override final
    {
        return Payload.ApplyAfter( Function, Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override final
    {
        return new(Memory) TFunctionDelegateInstance( *this );
    }

private:

    /* Payload sent into the function when executed*/
    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    /* Standard function pointer */
    FunctionType Function;
};

template<typename ReturnType, typename... ArgTypes>
class TFunctionDelegateInstance<ReturnType( ArgTypes... )> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType( ArgTypes... )>::Type;

public:

    TFunctionDelegateInstance( const TFunctionDelegateInstance& ) = default;

    /* Constructor taking a function */
    FORCEINLINE TFunctionDelegateInstance( FunctionType InFunction )
        : Super()
        , Function( InFunction )
    {
    }

    /* Execute the function */
    virtual ReturnType Execute( ArgTypes&&... Args ) override final
    {
        return Function( Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override final
    {
        return new(Memory) TFunctionDelegateInstance( *this );
    }

private:

    /* Standard function pointer */
    FunctionType Function;
};

/* Stores a memberfunction as a delegate */
template<bool IsConst, typename InstanceType, typename ClassType, typename FunctionType, typename... PayloadTypes>
class TMemberDelegateInstance;

template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType( ArgTypes... ), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType( ArgTypes..., PayloadTypes... )>::Type;

public:

    TMemberDelegateInstance( const TMemberDelegateInstance& ) = default;

    /* Constructor */
    FORCEINLINE TMemberDelegateInstance( InstanceType* InThis, FunctionType InFunction, PayloadTypes&&... InPayload )
        : Super()
        , This( InThis )
        , Function( InFunction )
        , Payload( Forward<PayloadTypes>( InPayload )... )
    {
        Assert( This != nullptr );
    }

    /* Execute function */
    virtual ReturnType Execute( ArgTypes&&... Args ) override final
    {
        return Payload.ApplyAfter( Function, This, Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override final
    {
        return new(Memory) TMemberDelegateInstance( *this );
    }

    /* Returns the stored instance */
    virtual const void* GetBoundObject() const override final
    {
        return reinterpret_cast<const void*>(This);
    }

    /* Checks if object is equal to the stored instance */
    virtual bool IsObjectBound( const void* Object ) const override final
    {
        return (GetBoundObject() == Object);
    }

private:

    /* Payload sent into the function when executed*/
    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    /* Instance pointer */
    InstanceType* This = nullptr;

    /* Function pointer */
    FunctionType Function;
};

template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType( ArgTypes... )> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType( ArgTypes... )>::Type;

public:

    TMemberDelegateInstance( const TMemberDelegateInstance& ) = default;

    /* Constructor */
    FORCEINLINE TMemberDelegateInstance( InstanceType* InThis, FunctionType InFunction )
        : Super()
        , This( InThis )
        , Function( InFunction )
    {
        Assert( This != nullptr );
    }

    /* Execute function */
    virtual ReturnType Execute( ArgTypes&&... Args ) override final
    {
        return ((*This).*Function)(Forward<ArgTypes>( Args )...);
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override final
    {
        return new(Memory) TMemberDelegateInstance( *this );
    }

    /* Returns the stored instance */
    virtual const void* GetBoundObject() const override final
    {
        return reinterpret_cast<const void*>(This);
    }

    /* Checks if object is equal to the stored instance */
    virtual bool IsObjectBound( const void* Object ) const override final
    {
        return (GetBoundObject() == Object);
    }

private:

    /* Instance pointer */
    InstanceType* This = nullptr;

    /* Function pointer */
    FunctionType Function;
};

/* Stores a lambda delegate */
template<typename FunctorType, typename FunctionType, typename... PayloadTypes>
class TLambdaDelegateInstance;

template<typename FunctorType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType( ArgTypes... ), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:

    TLambdaDelegateInstance( const TLambdaDelegateInstance& ) = default;

    /* Constructor */
    FORCEINLINE TLambdaDelegateInstance( FunctorType&& InFunctor, PayloadTypes&&... InPayload )
        : Super()
        , Functor( InFunctor )
        , Payload( Forward<PayloadTypes>( InPayload )... )
    {
    }

    /* Execute Functor */
    virtual ReturnType Execute( ArgTypes&&... Args ) override
    {
        return Payload.ApplyAfter( Functor, Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override
    {
        return new(Memory) TLambdaDelegateInstance( *this );
    }

private:

    /* Payload sent into the function when executed*/
    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    /* Functor */
    FunctorType Functor;
};

template<typename FunctorType, typename ReturnType, typename... ArgTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType( ArgTypes... )> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:

    TLambdaDelegateInstance( const TLambdaDelegateInstance& ) = default;

    /* Constructor */
    FORCEINLINE TLambdaDelegateInstance( FunctorType&& InFunctor )
        : Super()
        , Functor( InFunctor )
    {
    }

    /* Execute Functor */
    virtual ReturnType Execute( ArgTypes&&... Args ) override
    {
        return Functor( Forward<ArgTypes>( Args )... );
    }

    /* Clone this instance and store in the memory */
    virtual Super* Clone( void* Memory ) const override
    {
        return new(Memory) TLambdaDelegateInstance( *this );
    }

private:

    /* Functor */
    FunctorType Functor;
};