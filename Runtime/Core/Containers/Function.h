#pragma once
#include "Allocators.h"
#include "Tuple.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"

#define TFUNCTION_NUM_INLINE_BYTES (24)
#define TFUNCTION_INLINE_ALIGNMENT (8)

namespace FunctionInternal
{
    struct FFunctionStorage;

    template<typename FunctionType, typename... PayloadTypes>
    class TBindPayload
    {
    public:
        TBindPayload(FunctionType InFunc, PayloadTypes&&... PayloadArgs) noexcept
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
        TBindPayload(FunctionType InFunc) noexcept
            : Func(::Move(InFunc))
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
    struct TFunctionFunctorCaller;

    template<typename FunctorType, typename ReturnType, typename... ParamTypes>
    struct TFunctionFunctorCaller<FunctorType, ReturnType(ParamTypes...)>
    {
        static ReturnType CallFunctor(void* InFunctor, ParamTypes&... InParams)
        {
            return ::Invoke(*reinterpret_cast<FunctorType*>(InFunctor), ::Forward<ParamTypes>(InParams)...);
        }
    };

    template<typename FunctorType, typename... ParamTypes>
    struct TFunctionFunctorCaller<FunctorType, void(ParamTypes...)>
    {
        static void CallFunctor(void* InFunctor, ParamTypes&... InParams)
        {
            ::Invoke(*reinterpret_cast<FunctorType*>(InFunctor), ::Forward<ParamTypes>(InParams)...);
        }
    };


    struct IFunctionContainer
    {
        virtual ~IFunctionContainer() = default;
        
        // Copy the functor using the copy constructor to memory
        virtual void CopyToStorage(FFunctionStorage& Storage) const noexcept = 0;

        // Get the pointer to the functor object
        virtual void* GetFunctorPointer() noexcept = 0;
    };


    template<typename FunctorType>
    struct TFunctionContainer : public IFunctionContainer
    {
        inline static constexpr bool bHeapAllocated = (sizeof(FunctorType) > TFUNCTION_NUM_INLINE_BYTES);

        TFunctionContainer(const FunctorType& InFunctor) noexcept
            : IFunctionContainer()
            , Functor(InFunctor)
        {
        }

        TFunctionContainer(const TFunctionContainer& Other) noexcept
            : IFunctionContainer()
            , Functor(Other.Functor)
        {
        }

        TFunctionContainer(TFunctionContainer&& Other) noexcept
            : IFunctionContainer()
            , Functor(::Move(Other.Functor))
        {
        }

        ~TFunctionContainer() override
        {
            if constexpr (bHeapAllocated)
            {
                void* This = this;
                FMemory::Free(This);
            }
        }

        virtual void CopyToStorage(FFunctionStorage& Storage) const noexcept override final;

        virtual void* GetFunctorPointer() noexcept override final
        {
            return &Functor;
        }

        FunctorType Functor;
    };


    struct FFunctionStorage
    {
        FFunctionStorage() noexcept
            : HeapAllocation(nullptr)
        {
        }

        FFunctionStorage(const FFunctionStorage& Other) noexcept
            : HeapAllocation(nullptr)
        {
            Other.GetBoundObject()->CopyToStorage(*this);
        }

        FFunctionStorage(FFunctionStorage&& Other) noexcept
            : HeapAllocation(Other.HeapAllocation)
        {
            FMemory::Memcpy(InlineAllocation.Data, Other.InlineAllocation.Data, sizeof(InlineAllocation.Data));
            Other.HeapAllocation = nullptr;
        }

        FFunctionStorage& operator=(FFunctionStorage&&)      = delete;
        FFunctionStorage& operator=(const FFunctionStorage&) = delete;

        template<typename InFunctorType>
        void BindFunctor(InFunctorType&& Functor) noexcept
        {
            typedef typename TDecay<InFunctorType>::Type FunctorType;
            constexpr uint64 FunctorSize = sizeof(FunctorType);

            void* Memory;
            constexpr bool bUseInlineStorage = (FunctorSize <= TFUNCTION_NUM_INLINE_BYTES);
            if constexpr (bUseInlineStorage)
            {
                Memory = InlineAllocation.Data;
            }
            else
            {
                Memory = FMemory::Malloc(FunctorSize);
                HeapAllocation = Memory;
            }

            new(Memory) TFunctionContainer<FunctorType>(::Forward<InFunctorType>(Functor));
        }

        FORCEINLINE void Unbind()
        {
            IFunctionContainer* Container = GetBoundObject();
            Container->~IFunctionContainer();
        }

        FORCEINLINE IFunctionContainer* GetBoundObject() noexcept
        {
            void* Result = GetPointer();
            return reinterpret_cast<IFunctionContainer*>(Result);
        }

        FORCEINLINE const IFunctionContainer* GetBoundObject() const noexcept
        {
            const void* Result = GetPointer();
            return reinterpret_cast<const IFunctionContainer*>(Result);
        }

        FORCEINLINE void* GetPointer() noexcept
        {
            return HeapAllocation ? HeapAllocation : InlineAllocation.Data;
        }

        FORCEINLINE const void* GetPointer() const noexcept
        {
            return HeapAllocation ? HeapAllocation : InlineAllocation.Data;
        }

        FORCEINLINE void MoveFrom(FFunctionStorage&& Other)
        {
            FMemory::Memcpy(InlineAllocation.Data, Other.InlineAllocation.Data, sizeof(InlineAllocation.Data));
            HeapAllocation = Other.HeapAllocation;
            Other.HeapAllocation = nullptr;
        }

        void* HeapAllocation;
        TAlignedBytes<TFUNCTION_NUM_INLINE_BYTES, TFUNCTION_INLINE_ALIGNMENT> InlineAllocation;
    };


    template<typename FunctorType>
    void TFunctionContainer<FunctorType>::CopyToStorage(FFunctionStorage& Storage) const noexcept
    {
        void* Memory;
        if constexpr (bHeapAllocated)
        {
            Memory = Storage.InlineAllocation.Data;
        }
        else
        {
            Memory = FMemory::Malloc(sizeof(FunctorType));
            Storage.HeapAllocation = Memory;
        }

        new(Memory) TFunctionContainer(*this);
    }
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
public:

    /**
     * @brief - Default constructor 
     */
    FORCEINLINE TFunction() noexcept
        : FunctorCaller(nullptr)
        , Storage()
    {
    }

    /** 
     * @brief - Create from nullptr. Same as default constructor. 
     */
    FORCEINLINE TFunction(nullptr_type) noexcept
        : FunctorCaller(nullptr)
        , Storage()
    {
    }

    /**
     * @brief         - Construct a function from a functor
     * @param Functor - Functor to store
     */
    template<typename FunctorType>
    FORCEINLINE TFunction(FunctorType&& Functor) noexcept
        requires(TAnd<TIsInvokable<FunctorType, ParamTypes...>, TNot<TIsSame<TFunction, typename TDecay<FunctorType>::Type>>>::Value)
        : FunctorCaller(nullptr)
        , Storage()
    {
        InitializeFrom<FunctorType>(::Forward<FunctorType>(Functor));
    }

    /**
     * @brief       - Copy-constructor
     * @param Other - Function to copy from
     */
    FORCEINLINE TFunction(const TFunction& Other) noexcept
        : FunctorCaller(Other.FunctorCaller)
        , Storage(Other.Storage)
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - Function to move from
     */
    FORCEINLINE TFunction(TFunction&& Other) noexcept
        : FunctorCaller(Other.FunctorCaller)
        , Storage(::Move(Other.Storage))
    {
        Other.FunctorCaller = nullptr;
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TFunction()
    {
        Reset();
    }

    /** 
     * @return - Returns True if the pointer is not nullptr otherwise false 
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (FunctorCaller != nullptr);
    }
    
    /** 
     * @brief - Resets the object to default state
     */
    FORCEINLINE void Reset() noexcept
    {
        if (IsValid())
        {
            Storage.Unbind();
            FunctorCaller = nullptr;
        }
    }

    /**
     * @brief       - Swap functor with another instance
     * @param Other - Function to swap with
     */
    FORCEINLINE void Swap(TFunction& Other) noexcept
    {
        TFunction TmpFunction(::Move(*this));
        MoveFrom(::Move(Other));
        Other.MoveFrom(::Move(TmpFunction));
    }

    /**
     * @brief         - Assign a new functor
     * @param Functor - New functor to store
     */
    template<typename FunctorType>
    FORCEINLINE void Bind(FunctorType&& Functor) noexcept requires(TIsInvokable<FunctorType, ParamTypes...>::Value)
    {
        Reset();
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
        FunctionInternal::IFunctionContainer* Container = Storage.GetBoundObject();
        return FunctorCaller(Container->GetFunctorPointer(), Params...);
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
        Reset();
        return *this;
    }

private:
    template<typename InFunctorType>
    FORCEINLINE void InitializeFrom(InFunctorType&& Functor) noexcept requires(TIsInvokable<InFunctorType, ParamTypes...>::Value)
    {
        typedef typename TDecay<InFunctorType>::Type FunctorType;
        if constexpr (TIsNullable<FunctorType>::Value)
        {
            if (!Functor)
            {
                FunctorCaller = nullptr;
                return;
            }
        }

        FunctorCaller = &FunctionInternal::TFunctionFunctorCaller<FunctorType, ReturnType(ParamTypes...)>::CallFunctor;
        Storage.BindFunctor<InFunctorType>(::Forward<InFunctorType>(Functor));
    }

    FORCEINLINE void MoveFrom(TFunction&& Other)
    {
        Storage.MoveFrom(::Move(Other.Storage));
        FunctorCaller = Other.FunctorCaller;
        Other.FunctorCaller = nullptr;
    }

    ReturnType(*FunctorCaller)(void* Functor, ParamTypes&...);
    FunctionInternal::FFunctionStorage Storage;
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
    FORCEINLINE TFunctionRef(FunctorType&& InFunctor) noexcept requires(TIsInvokable<FunctorType, ParamTypes...>::Value)
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
        : FunctorCaller(Other.FunctorCaller)
        , Functor(Other.Functor)
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - Function to move from
     */
    FORCEINLINE TFunctionRef(TFunctionRef&& Other) noexcept
        : FunctorCaller(Other.FunctorCaller)
        , Functor(Other.Functor)
    {
        Other.Reset();
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
     * @brief - Resets the object to default state
     */
    FORCEINLINE void Reset()
    {
        FunctorCaller = nullptr;
        Functor = nullptr;
    }

    /**
     * @brief       - Swap functor with another instance
     * @param Other - Function to swap with
     */
    FORCEINLINE void Swap(TFunctionRef& Other) noexcept
    {
        TFunctionRef TmpFunction(::Move(*this));
        MoveFrom(::Move(Other));
        Other.MoveFrom(::Move(TmpFunction));
    }

    /**
     * @brief         - Assign a new functor
     * @param Functor - New functor to store
     */
    template<typename FunctorType >
    FORCEINLINE void Bind(FunctorType&& Functor) noexcept requires(TIsInvokable<FunctorType, ParamTypes...>::Value)
    {
        Reset();
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
    template<typename InFunctorType>
    FORCEINLINE typename TEnableIf<TIsInvokable<InFunctorType, ParamTypes...>::Value>::Type InitializeFrom(InFunctorType&& InFunctor) noexcept
    {
        typedef typename TDecay<InFunctorType>::Type FunctorType;
        if constexpr (TIsNullable<FunctorType>::Value)
        {
            if (!Functor)
            {
                FunctorCaller = nullptr;
                return;
            }
        }

        FunctorCaller = &FunctionInternal::TFunctionFunctorCaller<FunctorType, ReturnType(ParamTypes...)>::CallFunctor;
        Functor = reinterpret_cast<void*>(&InFunctor);
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