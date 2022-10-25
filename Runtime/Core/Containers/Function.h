#pragma once
#include "Allocators.h"
#include "Tuple.h"

#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"

namespace Internal
{
    template<typename FunctionType, typename... PayloadTypes>
    class TBindPayload
    {
    public:
        FORCEINLINE TBindPayload(FunctionType InFunc, PayloadTypes&&... PayloadArgs) noexcept
            : Payload(Forward<PayloadTypes>(PayloadArgs)...)
            , Func(Move(InFunc))
        { }

        template<typename... ArgTypes>
        FORCEINLINE auto Execute(ArgTypes&&... Args) noexcept
        {
            return Payload.ApplyBefore(Func, Forward<ArgTypes>(Args)...);
        }

        template<typename... ArgTypes>
        FORCEINLINE auto operator()(ArgTypes&&... Args) noexcept
        {
            return Execute(Forward<ArgTypes>(Args)...);
        }

    private:
         /** @brief - Arguments stored when calling bind and then applied to the function when invoked */
        TTuple<typename TDecay<PayloadTypes>::Type...> Payload;

        FunctionType Func;
    };


    template<typename FunctionType>
    class TBindPayload<FunctionType>
    {
    public:
        FORCEINLINE TBindPayload(FunctionType InFunc) noexcept
            : Func(Move(InFunc))
        { }

        template<typename... ArgTypes>
        FORCEINLINE auto Execute(ArgTypes&&... Args) noexcept
        {
            return Invoke(Func, Forward<ArgTypes>(Args)...);
        }

        template<typename... ArgTypes>
        FORCEINLINE auto operator()(ArgTypes&&... Args) noexcept
        {
            return Execute(Forward<ArgTypes>(Args)...);
        }

    private:
        FunctionType Func;
    };
}


template<typename FunctionType, typename... ArgTypes>
NODISCARD FORCEINLINE auto Bind(FunctionType Function, ArgTypes&&... Args)
{
    return Internal::TBindPayload<FunctionType, ArgTypes...>(Function, Forward<ArgTypes>(Args)...);
}


template<typename InvokableType>
class TFunction;

template<typename ReturnType, typename... ArgTypes>
class TFunction<ReturnType(ArgTypes...)>
{
    // TODO: Look into padding so we can use larger structs?
    enum { InlineBytes = 24 };

    using AllocatorType = TInlineArrayAllocator<int8, InlineBytes>;

    struct IFunctor
    {
        virtual ~IFunctor() = default;

        virtual ReturnType Invoke(ArgTypes&&... Args) noexcept = 0;
        virtual IFunctor*  Clone(void* Memory) const noexcept = 0;
    };


    template<typename FunctorType>
    class TGenericFunctor 
        : public IFunctor
    {
    public:
        FORCEINLINE TGenericFunctor(const FunctorType& InFunctor) noexcept
            : IFunctor()
            , Functor(InFunctor)
        { }

        FORCEINLINE TGenericFunctor(const TGenericFunctor& Other) noexcept
            : IFunctor()
            , Functor(Other.Functor)
        { }

        FORCEINLINE TGenericFunctor(TGenericFunctor&& Other) noexcept
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
            return new(Memory) TGenericFunctor(*this);
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
    { }

    /** 
     * @brief - Create from nullptr. Same as default constructor. 
     */
    FORCEINLINE TFunction(nullptr_type) noexcept
        : Storage()
        , Size(0)
    { }

    /**
     * @brief         - Construct a function from a functor
     * @param Functor - Functor to store
     */
    template<typename FunctorType>
    FORCEINLINE TFunction(FunctorType Functor) noexcept
        : Storage()
        , Size(0)
    {
        ConstructFrom<FunctorType>(Forward<FunctorType>(Functor));
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
        MoveFrom(Move(Other));
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
        TFunction TempFunction;
        TempFunction.MoveFrom(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempFunction));
    }

    /**
     * @brief         - Assign a new functor
     * @param Functor - New functor to store
     */
    template<typename FunctorType >
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ArgTypes...>::Value>::Type Assign(FunctorType&& Functor) noexcept
    {
        Release();
        ConstructFrom<FunctorType>(Forward<FunctorType>(Functor));
    }

    /**
     * @brief      - Invoke the stored function
     * @param Args - Arguments to forward to the function-call
     * @return     - The return value from the function-call
     */
    FORCEINLINE ReturnType Invoke(ArgTypes&&... Args) noexcept
    {
        CHECK(IsValid());
        return GetFunctor()->Invoke(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief      - Invoke the stored function
     * @param Args - Arguments to forward to the function-call
     * @return     - The return value from the function-call
     */
    FORCEINLINE ReturnType operator()(ArgTypes&&... Args) noexcept
    {
        return Invoke(Forward<ArgTypes>(Args)...);
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
        TFunction(Move(Other)).Swap(*this);
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
            GetFunctor()->~IFunctor();
        }
    }

    template<typename FunctorType>
    FORCEINLINE typename TEnableIf<TIsInvokable<FunctorType, ArgTypes...>::Value>::Type ConstructFrom(FunctorType&& Functor) noexcept
    {
        Release();

        int32 PreviousSize = Size;
        Size = sizeof(TGenericFunctor<FunctorType>);

        void* Memory = Storage.Realloc(PreviousSize, Size);
        new(Memory) TGenericFunctor<FunctorType>(Forward<FunctorType>(Functor));
    }

    FORCEINLINE void CopyFrom(const TFunction& Other) noexcept
    {
        if (Other.IsValid())
        {
            int32 CurrentSize = Size;
            Storage.Realloc(CurrentSize, Other.Size);

            Other.GetFunctor()->Clone(Storage.GetAllocation());

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
        Storage.MoveFrom(Move(Other.Storage));
        Size = Other.Size;
        Other.Size = 0;
    }

    NODISCARD FORCEINLINE IFunctor* GetFunctor() noexcept
    {
        return reinterpret_cast<IFunctor*>(Storage.GetAllocation());
    }

    NODISCARD FORCEINLINE const IFunctor* GetFunctor() const noexcept
    {
        return reinterpret_cast<const IFunctor*>(Storage.GetAllocation());
    }

    AllocatorType Storage;
    int32         Size;
};
