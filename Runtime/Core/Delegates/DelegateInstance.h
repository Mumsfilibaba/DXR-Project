#pragma once
#include "Core/Core.h"
#include "Core/Templates/Utility.h"
#include "Core/Containers/Tuple.h"
#include "Core/Threading/AtomicInt.h"

typedef int64 DelegateHandle;

class FDelegateHandle
{
    enum : DelegateHandle
    {
        InvalidHandle = DelegateHandle(-1)
    };

public:

    enum class EGenerateID
    {
        New
    };

    /**
     * @brief - Default constructor
     */
    FORCEINLINE FDelegateHandle()
        : Handle(InvalidHandle)
    { }

    /**
     * @brief - Construct a new delegate handle which generates a new ID
     */
    FORCEINLINE explicit FDelegateHandle(EGenerateID)
        : Handle(GenerateID())
    { }

    /**
     * @brief  - Checks if the handle is equal to nullptr
     * @return - Returns true if the handle is not equal to InvalidHandle
     */
    FORCEINLINE bool IsValid() const
    {
        return (Handle != InvalidHandle);
    }

    /** @brief - Sets the internal handle to an invalid one */
    FORCEINLINE void Reset()
    {
        Handle = InvalidHandle;
    }

    /**
     * @brief  - Retrieve the ID 
     * @return - Returns the delegate-handle
     */
    FORCEINLINE DelegateHandle GetNative() const
    {
        return Handle;
    }

    /**
     * @brief  - Checks if the handle is equal to nullptr
     * @return - Returns true if the handle is not equal to InvalidHandle
     */
    FORCEINLINE operator bool() const
    {
        return IsValid();
    }

    /**
     * @brief       - Checks equality between two handles 
     * @param Other - Other delegate-handle to compare with
     * @return      - Returns true if the delegate-handles are equal to each other
     */
    FORCEINLINE bool operator==(FDelegateHandle Other) const
    {
        return (Handle == Other.Handle);
    }

    /**
     * @brief       - Checks equality between two handles
     * @param Other - Other delegate-handle to compare with
     * @return      - Returns false if the delegate-handles are equal to each other
     */
    FORCEINLINE bool operator!=(FDelegateHandle Other) const
    {
        return !(*this == Other);
    }

private:
    static FORCEINLINE DelegateHandle GenerateID()
    {
        return ++NextID;
    }

    DelegateHandle Handle;

    static CORE_API FAtomicInt64 NextID;
};


struct IDelegateInstance
{
    virtual ~IDelegateInstance() noexcept = default;

    /**
     * @brief  - Retrieve the object of the function, returns nullptr for non-member delegates 
     * @return - Returns the bound object of the delegate
     */
    virtual const void* GetBoundObject() const = 0;

    /**
     * @brief        - Check if the object is the one that is bound to the delegate instance 
     * @param Object - Object to check
     * @return       - Returns true if the object is bound to the delegate
     */
    virtual bool IsObjectBound(const void* Object) const = 0;

    /**
     * @brief  - Retrieve the handle to the delegate 
     * @return - Returns the delegate-handle of this delegate-instance
     */
    virtual FDelegateHandle GetHandle() const = 0;

    /**
     * @brief        - Clones the delegate and stores it in the specified memory 
     * @param Memory - Memory to store the cloned instance into
     * @return       - Returns a clone of the instance 
     */
    virtual IDelegateInstance* Clone(void* Memory) const = 0;
};


template<typename ReturnType, typename... ArgTypes>
class TDelegateInstance 
    : public IDelegateInstance
{
protected:
    FORCEINLINE TDelegateInstance()
        : IDelegateInstance()
        , Handle(FDelegateHandle::EGenerateID::New)
    { }

public:

    /**
     * @brief      - Executes the stored function or functor 
     * @param Args - Arguments to the function-call
     * @return     - The result of the function-call
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

    virtual FDelegateHandle GetHandle() const override final
    {
        return Handle;
    }

protected:
    FDelegateHandle Handle;
};


template<typename FunctionType, typename... PayloadTypes>
class TFunctionDelegateInstance;

template<typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TFunctionDelegateInstance<ReturnType(ArgTypes...), PayloadTypes...> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super        = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType(ArgTypes..., PayloadTypes...)>::Type;

public:
    TFunctionDelegateInstance(const TFunctionDelegateInstance&) = default;

    FORCEINLINE TFunctionDelegateInstance(FunctionType InFunction, PayloadTypes&&... InPayload)
        : Super()
        , Function(InFunction)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    { }

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


template<typename ReturnType, typename... ArgTypes>
class TFunctionDelegateInstance<ReturnType(ArgTypes...)> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super        = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TFunctionType<ReturnType(ArgTypes...)>::Type;

public:
    TFunctionDelegateInstance(const TFunctionDelegateInstance&) = default;

    FORCEINLINE TFunctionDelegateInstance(FunctionType InFunction)
        : Super()
        , Function(InFunction)
    { }

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


template<bool IsConst, typename InstanceType, typename ClassType, typename FunctionType, typename... PayloadTypes>
class TMemberDelegateInstance;

template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType(ArgTypes...), PayloadTypes...> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super        = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType(ArgTypes..., PayloadTypes...)>::Type;

public:
    TMemberDelegateInstance(const TMemberDelegateInstance&) = default;

    FORCEINLINE TMemberDelegateInstance(InstanceType* InThis, FunctionType InFunction, PayloadTypes&&... InPayload)
        : Super()
        , This(InThis)
        , Function(InFunction)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    {
        CHECK(This != nullptr);
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


template<bool IsConst, typename InstanceType, typename ClassType, typename ReturnType, typename... ArgTypes>
class TMemberDelegateInstance<IsConst, InstanceType, ClassType, ReturnType(ArgTypes...)> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super        = TDelegateInstance<ReturnType, ArgTypes...>;
    using FunctionType = typename TMemberFunctionType<IsConst, ClassType, ReturnType(ArgTypes...)>::Type;

public:
    TMemberDelegateInstance(const TMemberDelegateInstance&) = default;

    FORCEINLINE TMemberDelegateInstance(InstanceType* InThis, FunctionType InFunction)
        : Super()
        , This(InThis)
        , Function(InFunction)
    {
        CHECK(This != nullptr);
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


template<typename FunctorType, typename FunctionType, typename... PayloadTypes>
class TLambdaDelegateInstance;

template<typename FunctorType, typename ReturnType, typename... ArgTypes, typename... PayloadTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...), PayloadTypes...> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:
    TLambdaDelegateInstance(const TLambdaDelegateInstance&) = default;

    FORCEINLINE TLambdaDelegateInstance(FunctorType&& InFunctor, PayloadTypes&&... InPayload)
        : Super()
        , Functor(InFunctor)
        , Payload(Forward<PayloadTypes>(InPayload)...)
    { }

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


template<typename FunctorType, typename ReturnType, typename... ArgTypes>
class TLambdaDelegateInstance<FunctorType, ReturnType(ArgTypes...)> 
    : public TDelegateInstance<ReturnType, ArgTypes...>
{
    using Super = TDelegateInstance<ReturnType, ArgTypes...>;

public:
    TLambdaDelegateInstance(const TLambdaDelegateInstance&) = default;

    FORCEINLINE TLambdaDelegateInstance(FunctorType&& InFunctor)
        : Super()
        , Functor(InFunctor)
    { }

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
