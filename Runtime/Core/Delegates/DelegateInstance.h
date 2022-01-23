#pragma once
#include "Core/Core.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/FunctionType.h"
#include "Core/Containers/Tuple.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DelegateHandle - A handle for a delegate

typedef int64 DelegateHandle;

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

    /**
     * Default constructor
     */
    FORCEINLINE CDelegateHandle()
        : Handle(InvalidHandle)
    {
    }

    /**
     * Construct a new delegate handle which generates a new ID
     */
    FORCEINLINE explicit CDelegateHandle(EGenerateID)
        : Handle(GenerateID())
    {
    }

    /**
     * Checks if the handle is equal to nullptr
     * 
     * @return: Returns true if the handle is not equal to InvalidHandle
     */
    FORCEINLINE bool IsValid() const
    {
        return (Handle != InvalidHandle);
    }

    /** Sets the internal handle to an invalid one */
    FORCEINLINE void Reset()
    {
        Handle = InvalidHandle;
    }

    /**
     * Retrieve the ID 
     * 
     * @return: Returns the delegate-handle
     */
    FORCEINLINE DelegateHandle GetNative() const
    {
        return Handle;
    }

    /**
     * Checks if the handle is equal to nullptr
     *
     * @return: Returns true if the handle is not equal to InvalidHandle
     */
    FORCEINLINE operator bool() const
    {
        return IsValid();
    }

    /**
     * Checks equality between two handles 
     * 
     * @param RHS: Other delegate-handle to compare with
     * @return: Returns true if the delegate-handles are equal to each other
     */
    FORCEINLINE bool operator==(CDelegateHandle RHS) const
    {
        return (Handle == RHS.Handle);
    }

    /**
     * Checks equality between two handles
     *
     * @param RHS: Other delegate-handle to compare with
     * @return: Returns false if the delegate-handles are equal to each other
     */
    FORCEINLINE bool operator!=(CDelegateHandle RHS) const
    {
        return !(*this == RHS);
    }

private:

    static FORCEINLINE DelegateHandle GenerateID()
    {
        return ++NextID;
    }

    DelegateHandle Handle;

    static CORE_API DelegateHandle NextID;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IDelegateInstance - Base-type for delegates

class IDelegateInstance
{
public:

    virtual ~IDelegateInstance() noexcept = default;

    /**
     * Retrieve the object of the function, returns nullptr for non-member delegates 
     * 
     * @return: Returns the bound object of the delegate
     */
    virtual const void* GetBoundObject() const = 0;

    /**
     * Check if the object is the one that is bound to the delegate instance 
     * 
     * @param Object: Object to check
     * @return: Returns true if the object is bound to the delegate
     */
    virtual bool IsObjectBound(const void* Object) const = 0;

    /**
     * Retrieve the handle to the delegate 
     * 
     * @return: Returns the delegate-handle of this delegate-instance
     */
    virtual CDelegateHandle GetHandle() const = 0;

    /**
     * Clones the delegate and stores it in the specified memory 
     * 
     * @param Memory: Memory to store the cloned instance into
     * @return: Returns a clone of the instance 
     */
    virtual IDelegateInstance* Clone(void* Memory) const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DelegateInstance - Base-class for delegates*/

template<typename ReturnType, typename... ArgTypes>
class TDelegateInstance : public IDelegateInstance
{
public:

    /**
     * Executes the stored function or functor 
     * 
     * @param Args: Arguments to the function-call
     * @return: The result of the function-call
     */
    virtual ReturnType Execute(ArgTypes... Args) = 0;

public:

    virtual const void* GetBoundObject() const override
    {
        return nullptr;
    }

    virtual bool IsObjectBound(const void*) const override
    {
        return false;
    }

    virtual CDelegateHandle GetHandle() const override final
    {
        return Handle;
    }

protected:

    FORCEINLINE TDelegateInstance()
        : IDelegateInstance()
        , Handle(CDelegateHandle::EGenerateID::New)
    {
    }

    CDelegateHandle Handle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for storing a "normal" function in a delegate

template<typename FunctionType, typename... PayloadTypes>
class TFunctionDelegateInstance;

template<typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TFunctionDelegateInstance<ReturnType(ArgTypes...), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType(ArgTypes..., PayloadTypes...)>::Type;

public:

    TFunctionDelegateInstance(const TFunctionDelegateInstance&) = default;

    FORCEINLINE TFunctionDelegateInstance(FunctionType InFunction, PayloadTypes&&... InPayload)
        : Super()
        , Function(InFunction)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    {
    }

    virtual ReturnType Execute(ArgTypes... Args) override final
    {
        return Payload.ApplyAfter(Function, Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override final
    {
        return new(Memory) TFunctionDelegateInstance(*this);
    }

private:

    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    FunctionType Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for storing a "normal" function in a delegate when no arguments are bound

template<typename ReturnType, typename... ArgTypes>
class TFunctionDelegateInstance<ReturnType(ArgTypes...)> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType(ArgTypes...)>::Type;

public:

    TFunctionDelegateInstance(const TFunctionDelegateInstance&) = default;

    FORCEINLINE TFunctionDelegateInstance(FunctionType InFunction)
        : Super()
        , Function(InFunction)
    {
    }

    virtual ReturnType Execute(ArgTypes... Args) override final
    {
        return Function(Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override final
    {
        return new(Memory) TFunctionDelegateInstance(*this);
    }

private:
    FunctionType Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores a member-function as a delegate

template<bool IsConst, typename InstanceType, typename ClassType, typename FunctionType, typename... PayloadTypes>
class TMemberDelegateInstance;

template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType(ArgTypes..., PayloadTypes...)>::Type;

public:

    TMemberDelegateInstance(const TMemberDelegateInstance&) = default;

    FORCEINLINE TMemberDelegateInstance(InstanceType* InThis, FunctionType InFunction, PayloadTypes&&... InPayload)
        : Super()
        , This(InThis)
        , Function(InFunction)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    {
        Assert(This != nullptr);
    }

    virtual ReturnType Execute(ArgTypes... Args) override final
    {
        return Payload.ApplyAfter(Function, This, Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override final
    {
        return new(Memory) TMemberDelegateInstance(*this);
    }

    virtual const void* GetBoundObject() const override final
    {
        return reinterpret_cast<const void*>(This);
    }

    virtual bool IsObjectBound(const void* Object) const override final
    {
        return (GetBoundObject() == Object);
    }

private:

    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    InstanceType* This = nullptr;
    FunctionType  Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores a member-function as a delegate, when not arguments are bound

template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType(ArgTypes...)> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType(ArgTypes...)>::Type;

public:

    TMemberDelegateInstance(const TMemberDelegateInstance&) = default;

    FORCEINLINE TMemberDelegateInstance(InstanceType* InThis, FunctionType InFunction)
        : Super()
        , This(InThis)
        , Function(InFunction)
    {
        Assert(This != nullptr);
    }

    virtual ReturnType Execute(ArgTypes... Args) override final
    {
        return ((*This).*Function)(Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override final
    {
        return new(Memory) TMemberDelegateInstance(*this);
    }

    virtual const void* GetBoundObject() const override final
    {
        return reinterpret_cast<const void*>(This);
    }

    virtual bool IsObjectBound(const void* Object) const override final
    {
        return (GetBoundObject() == Object);
    }

private:
    InstanceType* This = nullptr;
    FunctionType  Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores a lambda delegate

template<typename FunctorType, typename FunctionType, typename... PayloadTypes>
class TLambdaDelegateInstance;

template<typename FunctorType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...), PayloadTypes...> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:

    TLambdaDelegateInstance(const TLambdaDelegateInstance&) = default;

    FORCEINLINE TLambdaDelegateInstance(FunctorType&& InFunctor, PayloadTypes&&... InPayload)
        : Super()
        , Functor(InFunctor)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    {
    }

    virtual ReturnType Execute(ArgTypes... Args) override
    {
        return Payload.ApplyAfter(Functor, Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override
    {
        return new(Memory) TLambdaDelegateInstance(*this);
    }

private:

    TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

    FunctorType Functor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores a lambda delegate when no arguments are bound

template<typename FunctorType, typename ReturnType, typename... ArgTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...)> : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:

    TLambdaDelegateInstance(const TLambdaDelegateInstance&) = default;

    FORCEINLINE TLambdaDelegateInstance(FunctorType&& InFunctor)
        : Super()
        , Functor(InFunctor)
    {
    }

    virtual ReturnType Execute(ArgTypes... Args) override
    {
        return Functor(Forward<ArgTypes>(Args)...);
    }

    virtual Super* Clone(void* Memory) const override
    {
        return new(Memory) TLambdaDelegateInstance(*this);
    }

private:
    FunctorType Functor;
};