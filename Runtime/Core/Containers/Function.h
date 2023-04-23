#pragma once
#include "Allocators.h"
#include "Tuple.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"

namespace FunctionInternal
{
    template<typename FunctionType, typename... PayloadTypes>
    class TBindPayload
    {
    public:
        FORCEINLINE TBindPayload(FunctionType InFunc, PayloadTypes&&... PayloadArgs) noexcept
            : Payload(::Forward<PayloadTypes>(PayloadArgs)...)
            , Func(::Move(InFunc))
        {
        }

        template<typename... ParamTypes>
        FORCEINLINE auto operator()(ParamTypes&&... Params) noexcept
        {
            return Payload.ApplyBefore(Func, ::Forward<ParamTypes>(Params)...);
        }

    private:
        TTuple<typename TDecay<PayloadTypes>::Type...> Payload;
        FunctionType Func;
    };

    template<typename FunctionType>
    class TBindPayload<FunctionType>
    {
    public:
        FORCEINLINE TBindPayload(FunctionType InFunc) noexcept
            : Func(Move(InFunc))
        {
        }

        template<typename... ParamTypes>
        FORCEINLINE auto operator()(ParamTypes&&... Params) noexcept
        {
            return Invoke(Func, ::Forward<ParamTypes>(Params)...);
        }

    private:
        FunctionType Func;
    };


    template<typename FunctorType, typename FunctionType>
    struct TFunctionCallFunctor;

    template<typename FunctorType, typename ReturnType, typename... ParamTypes>
    struct TFunctionCallFunctor<FunctorType, ReturnType(ParamTypes...)>
    {
        static ReturnType CallFunctor(void* InFunctor, ParamTypes&... InParams)
        {
            return ::Invoke(*reinterpret_cast<FunctorType*>(InFunctor), ::Forward<ParamTypes>(InParams)...);
        }
    };

    template<typename FunctorType, typename... ParamTypes>
    struct TFunctionCallFunctor<FunctorType, void(ParamTypes...)>
    {
        static void CallFunctor(void* InFunctor, ParamTypes&... InParams)
        {
            ::Invoke(*reinterpret_cast<FunctorType*>(InFunctor), ::Forward<ParamTypes>(InParams)...);
        }
    };
}

template<typename FunctionType, typename... ParamTypes>
NODISCARD FORCEINLINE auto Bind(FunctionType InFunction, ParamTypes&&... InParams)
{
    return FunctionInternal::TBindPayload<FunctionType, ParamTypes...>(InFunction, ::Forward<ParamTypes>(InParams)...);
}


template<typename FunctionType>
class TFunction;

template<typename ReturnType, typename... ParamTypes>
class TFunction<ReturnType(ParamTypes...)>
{
    inline static CONSTEXPR int32 InlineBytes     = 24;
    inline static CONSTEXPR int32 InlineAlignment = 16;

    struct IFunctor
    {
        virtual ~IFunctor() = default;

        virtual ReturnType Invoke(ArgTypes&&... Args) noexcept = 0;

        virtual IFunctor* Clone(void* Memory) const noexcept = 0;
    };

    template<typename FunctorType>
    class FFunctionContainer : public IFunctor
    {
    public:
        FORCEINLINE FFunctionContainer(const FunctorType& InFunctor) noexcept
            : IFunctor()
            , Functor(InFunctor)
        {
        }

        FORCEINLINE FFunctionContainer(const FFunctionContainer& Other) noexcept
            : IFunctor()
            , Functor(Other.Functor)
        {
        }

        FORCEINLINE FFunctionContainer(FFunctionContainer&& Other) noexcept
            : IFunctor()
            , Functor(Move(Other.Functor))
        {
            FMemory::Memzero(&Other);
        }

        virtual ReturnType Invoke(ArgTypes&&... Args) noexcept override final
        {
            return Functor(Forward<ArgTypes>(Args)...);
        }

        virtual IFunctor* Clone(void* Memory) const noexcept override final
        {
            return new(Memory) FFunctionContainer(*this);
        }

    private:
        FunctorType Functor;
    };

public:

    /**
     * @brief - Default constructor 
     */
    FORCEINLINE TFunction() noexcept
        : Storage()
        , Size(0)
    {
    }

    /** 
     * @brief - Create from nullptr. Same as default constructor. 
     */
    FORCEINLINE TFunction(nullptr_type) noexcept
        : Storage()
        , Size(0)
    {
    }

    /**
     * @brief         - Construct a function from a functor
     * @param Functor - Functor to store
     */
    template<typename FunctorType>
    FORCEINLINE TFunction(FunctorType&& Functor) noexcept
        : Storage()
        , Size(0)
    {
        InitializeFrom<FunctorType>(::Forward<FunctorType>(Functor));
    }

    /**
     * @brief       - Copy-constructor
     * @param Other - Function to copy from
     */
    FORCEINLINE TFunction(const TFunction& Other) noexcept
        : Storage()
        , Size(0)
    {
        CopyFrom(Other);
    }

    /**
     * @brief       - Move-constructor
     * @param Other - Function to move from
     */
    FORCEINLINE TFunction(TFunction&& Other) noexcept
        : Storage()
        , Size(0)
    {
        MoveFrom(::Move(Other));
    }

    /**
     * @brief - Destructor 
     */
    FORCEINLINE ~TFunction()
    {
        Release();
    }

    /** 
     * @return - Returns True if the pointer is not nullptr otherwise false 
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Size > 0);
    }

    /**
     * @brief       - Swap functor with another instance
     * @param Other - Function to swap with
     */
    FORCEINLINE void Swap(TFunction& Other) noexcept
    {
        TFunction TmpFunction;
        TmpFunction.MoveFrom(::Move(*this));
        MoveFrom(::Move(Other));
        Other.MoveFrom(::Move(TmpFunction));
    }

    /**
     * @brief         - Assign a new functor
     * @param Functor - New functor to store
     */
    template<typename FunctorType >
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ParamTypes...>::Value>::Type Assign(FunctorType&& Functor) noexcept
    {
        Release();
        InitializeFrom<FunctorType>(::Forward<FunctorType>(Functor));
    }

    /**
     * @brief        - Invoke the stored function
     * @param Params - Arguments to forward to the function-call
     * @return       - The return value from the function-call
     */
    FORCEINLINE ReturnType operator()(ParamTypes&&... Params) noexcept
    {
        CHECK(IsValid());
        return GetBoundObject()->Invoke(::Forward<ParamTypes>(Params)...);
    }

public:

    /**
     * @return - Returns True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - Instance to copy from
     * @return      - A reference to this object
     */
    FORCEINLINE TFunction& operator=(const TFunction& Other) noexcept
    {
        TFunction(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - Instance to move from
     * @return      - A reference to this object
     */
    FORCEINLINE TFunction& operator=(TFunction&& Other) noexcept
    {
        TFunction(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Set the pointer to nullptr
     * @return - A reference to this object
     */
    FORCEINLINE TFunction& operator=(nullptr_type) noexcept
    {
        Release();
        return *this;
    }

private:
    FORCEINLINE void Release() noexcept
    {
        if (IsValid())
        {
            GetBoundObject()->~IFunctor();
        }
    }

    template<typename FunctorType>
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ParamTypes...>::Value>::Type InitializeFrom(FunctorType&& Functor) noexcept
    {
        Release();

        typedef typename TRemoveReference<InFunctorType>::Type FunctorType;
        FunctorCaller = FunctionInternal::TFunctionCallFunctor<FunctorType, ReturnType(ParamTypes...)>::CallFunctor;

        CONSTEXPR bool bUseInlineStorage = sizeof(FunctorType) <= InlineBytes;
        
        void* Memory;
        if CONSTEXPR (bUseInlineStorage)
        {
            Memory = &InlineAllocation;
        }
        else
        {

        }

        Size = ;
        if 

        int32 PreviousSize = Size;

        void* Memory = Storage.Realloc(PreviousSize, Size);
        new(Memory) FFunctionContainer<FunctorType>(::Forward<FunctorType>(Functor));

    }

    FORCEINLINE void CopyFrom(const TFunction& Other) noexcept
    {
        if (Other.IsValid())
        {
            int32 CurrentSize = Size;
            Storage.Realloc(CurrentSize, Other.Size);

            Other.GetBoundObject()->Clone(Storage.GetAllocation());

            Size = Other.Size;
        }
        else
        {
            Size = 0;
            Storage.Free();
        }
    }

    FORCEINLINE void MoveFrom(TFunction&& Other)
    {
        Storage.MoveFrom(::Move(Other.Storage));
        Size = Other.Size;
        Other.Size = 0;
    }

    NODISCARD FORCEINLINE IFunctor* GetBoundObject() noexcept
    {
        void* Pointer = HeapAllocation ? HeapAllocation : reinterpret_cast<void*>(Storage.Data);
        return reinterpret_cast<IFunctor*>(Pointer);
    }

    NODISCARD FORCEINLINE const IFunctor* GetBoundObject() const noexcept
    {
        void* Pointer = HeapAllocation ? HeapAllocation : reinterpret_cast<void*>(Storage.Data);
        return reinterpret_cast<IFunctor*>(Pointer);
    }

    ReturnType(*FunctorCaller)(void* Functor, ParamTypes&...);

    TAlignedBytes<InlineBytes, InlineAlignment> InlineAllocation;
    void* HeapAllocation;
};


template<typename InvokableType>
class TFunctionRef;

template<typename ReturnType, typename... ParamTypes>
class TFunctionRef<ReturnType(ParamTypes...)>
{
public:

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TFunctionRef() noexcept
        : FunctorCaller(nullptr)
        , Functor(nullptr)
    {
    }

    /**
     * @brief - Create from nullptr. Same as default constructor.
     */
    FORCEINLINE TFunctionRef(nullptr_type) noexcept
        : FunctorCaller(nullptr)
        , Functor(nullptr)
    {
    }

    /**
     * @brief         - Construct a function from a functor
     * @param Functor - Functor to store
     */
    template<typename FunctorType>
    FORCEINLINE TFunctionRef(FunctorType&& InFunctor) noexcept
        : FunctorCaller(nullptr)
        , Functor(nullptr)
    {
        InitializeFrom<FunctorType>(::Forward<FunctorType>(InFunctor));
    }

    /**
     * @brief       - Copy-constructor
     * @param Other - Function to copy from
     */
    FORCEINLINE TFunctionRef(const TFunctionRef& Other) noexcept
        : FunctorCaller(nullptr)
        , Functor(nullptr)
    {
        CopyFrom(Other);
    }

    /**
     * @brief       - Move-constructor
     * @param Other - Function to move from
     */
    FORCEINLINE TFunctionRef(TFunctionRef&& Other) noexcept
        : FunctorCaller(nullptr)
        , Functor(nullptr)
    {
        MoveFrom(::Move(Other));
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TFunctionRef()
    {
        Reset();
    }

    /**
     * @return - Returns True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Functor != nullptr) && (FunctorCaller != nullptr);
    }

    /**
     * @brief       - Swap functor with another instance
     * @param Other - Function to swap with
     */
    FORCEINLINE void Swap(TFunctionRef& Other) noexcept
    {
        TFunctionRef TmpFunction;
        TmpFunction.MoveFrom(::Move(*this));
        MoveFrom(::Move(Other));
        Other.MoveFrom(::Move(TmpFunction));
    }

    /**
     * @brief         - Assign a new functor
     * @param Functor - New functor to store
     */
    template<typename FunctorType >
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ParamTypes...>::Value>::Type Assign(FunctorType&& Functor) noexcept
    {
        Release();
        InitializeFrom<FunctorType>(::Forward<FunctorType>(Functor));
    }

    /**
     * @brief        - Invoke the stored function
     * @param Params - Arguments to forward to the function-call
     * @return       - The return value from the function-call
     */
    FORCEINLINE ReturnType operator()(ParamTypes... Params) noexcept
    {
        CHECK(IsValid());
        return FunctorCaller(Functor, Params...);
    }

public:

    /**
     * @return - Returns True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - Instance to copy from
     * @return      - A reference to this object
     */
    FORCEINLINE TFunctionRef& operator=(const TFunctionRef& Other) noexcept
    {
        TFunctionRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - Instance to move from
     * @return      - A reference to this object
     */
    FORCEINLINE TFunctionRef& operator=(TFunctionRef&& Other) noexcept
    {
        TFunctionRef(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Set the pointer to nullptr
     * @return - A reference to this object
     */
    FORCEINLINE TFunctionRef& operator=(nullptr_type) noexcept
    {
        Reset();
        return *this;
    }

private:
    FORCEINLINE void Reset()
    {
        FunctorCaller = nullptr;
        Functor = nullptr;
    }

    template<typename InFunctorType>
    FORCEINLINE typename TEnableIf<TIsInvokable<InFunctorType, ParamTypes...>::Value>::Type InitializeFrom(InFunctorType&& InFunctor) noexcept
    {
        typedef typename TRemoveReference<InFunctorType>::Type FunctorType;
        FunctorCaller = FunctionInternal::TFunctionCallFunctor<FunctorType, ReturnType(ParamTypes...)>::CallFunctor;
        Functor = reinterpret_cast<void*>(&InFunctor);
    }

    FORCEINLINE void CopyFrom(const TFunctionRef& Other) noexcept
    {
        FunctorCaller = Other.FunctorCaller;
        Functor = Other.Functor;
    }

    FORCEINLINE void MoveFrom(TFunctionRef&& Other)
    {
        FunctorCaller = Other.FunctorCaller;
        Functor = Other.Functor;
        Other.Reset();
    }

    ReturnType(*FunctorCaller)(void* Functor, ParamTypes&...);
    void* Functor;
};