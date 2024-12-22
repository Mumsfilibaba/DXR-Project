#pragma once
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Functional.h"

namespace TupleInternal
{
    template<uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex;

    template<typename WantedType, typename... Types>
    struct TTupleGetByElement;

    template<uint32 Index, typename ValueType>
    struct TTupleLeaf
    {
        TTupleLeaf() = default;
        TTupleLeaf(TTupleLeaf&&) = default;
        TTupleLeaf(const TTupleLeaf&) = default;

        TTupleLeaf& operator=(TTupleLeaf&&) = default;
        TTupleLeaf& operator=(const TTupleLeaf&) = default;

        template<typename... ArgTypes>
        FORCEINLINE explicit TTupleLeaf(ArgTypes&&... Args) noexcept requires(TAnd<TValue<(sizeof... (ArgTypes)) != 0>, TIsConstructible<ValueType, typename TDecay<ArgTypes>::Type...>>::Value)
            : Value(Forward<ArgTypes>(Args)...)
        {
        }

        FORCEINLINE explicit TTupleLeaf(const ValueType& InValue) noexcept
            : Value(InValue)
        {
        }

        FORCEINLINE explicit TTupleLeaf(ValueType&& InValue) noexcept
            : Value(Move(InValue))
        {
        }

        NODISCARD FORCEINLINE int32 Swap(TTupleLeaf& Other) noexcept
        {
            ::Swap<ValueType>(Value, Other.Value);
            return 0; // Return zero for the ExpandPacks function
        }

        ValueType Value;
    };

    template<uint32 Index, typename ValueType>
    struct TTupleLeaf<Index, ValueType&>
    {
        TTupleLeaf() = delete;
        TTupleLeaf& operator=(const TTupleLeaf&) = delete;
        TTupleLeaf& operator=(TTupleLeaf&&) = delete;

        TTupleLeaf(const TTupleLeaf&) = default;
        TTupleLeaf(TTupleLeaf&&) = default;

        FORCEINLINE explicit TTupleLeaf(ValueType&& InValue) noexcept
            : Value(Move(InValue))
        {
        }

        FORCEINLINE explicit TTupleLeaf(const ValueType& InValue) noexcept
            : Value(InValue)
        {
        }

        template<typename...ArgTypes>
        FORCEINLINE explicit TTupleLeaf(ArgTypes&&... Args) noexcept requires(TIsConstructible<ValueType, ArgTypes&&...>::Value)
            : Value(Forward<ArgTypes>(Args)...)
        {
        }

        NODISCARD FORCEINLINE int32 Swap(TTupleLeaf&) noexcept
        {
            return 0; // Return zero for the ExpandPacks function
        }

        ValueType Value;
    };

    template<uint32 Iteration, uint32 Index, typename... Types>
    struct TTupleGetByIndexHelper;

    template<uint32 Iteration, uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Iteration, Index, ElementType, Types...> : public TTupleGetByIndexHelper<Iteration + 1, Index, Types...>
    {
    };

    template<uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Index, Index, ElementType, Types...>
    {
        typedef ElementType Type;

        static constexpr uint32 ElementIndex = Index;

        template<typename TupleType>
        NODISCARD static FORCEINLINE auto& GetByIndex(TupleType& Tuple) noexcept
        {
            return static_cast<TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }

        template<typename TupleType>
        NODISCARD static FORCEINLINE const auto& GetByIndex(const TupleType& Tuple) noexcept
        {
            return static_cast<const TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }
    };

    template <uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex : public TTupleGetByIndexHelper<0, SearchForIndex, Types...>
    {
    };

    template<uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper;

    template<uint32 Iteration, typename WantedType, typename ElementType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, ElementType, Types...> : public TTupleGetByElementHelper<Iteration + 1, WantedType, Types...>
    {
    };

    template<uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, WantedType, Types...>
    {
        typedef WantedType Type;

        static constexpr uint32 ElementIndex = Iteration;

        template <typename TupleType>
        NODISCARD static FORCEINLINE auto& GetByType(TupleType& Tuple) noexcept
        {
            return static_cast<TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }

        template <typename TupleType>
        NODISCARD static FORCEINLINE const auto& GetByType(const TupleType& Tuple) noexcept
        {
            return static_cast<const TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }
    };

    template<typename WantedType, typename... Types>
    struct TTupleGetByElement : public TTupleGetByElementHelper<0, WantedType, Types...>
    {
    };

    template<typename Indices, typename... Types>
    class TTupleStorage;

    template<uint32... Indices, typename... Types>
    class TTupleStorage<TIntegerSequence<uint32, Indices...>, Types...> : public TTupleLeaf<Indices, Types>...
    {
    public:
        TTupleStorage() = default;
        TTupleStorage(TTupleStorage&&) = default;
        TTupleStorage(const TTupleStorage&) = default;

        TTupleStorage& operator=(TTupleStorage&&) = default;
        TTupleStorage& operator=(const TTupleStorage&) = default;

        template<typename... ArgTypes>
        FORCEINLINE explicit TTupleStorage(ArgTypes&&... Args) noexcept
            : TTupleLeaf<Indices, Types>(Forward<ArgTypes>(Args))...
        {
        }

        FORCEINLINE void Swap(TTupleStorage& Other) noexcept
        {
            ExpandPacks(TTupleLeaf<Indices, Types>::Swap(static_cast<TTupleLeaf<Indices, Types>&>(Other))...);
        }

    public:
        template<typename ElementType>
        NODISCARD FORCEINLINE auto& GetByType() noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::GetByType(*this);
        }

        template<typename ElementType>
        NODISCARD FORCEINLINE const auto& GetByType() const noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::GetByType(*this);
        }

    public:
        template<uint32 Index>
        NODISCARD FORCEINLINE auto& GetByIndex() noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::GetByIndex(*this);
        }

        template<uint32 Index>
        NODISCARD FORCEINLINE const auto& GetByIndex() const noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::GetByIndex(*this);
        }

    public:
        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., this->template GetByIndex<Indices>()...);
        }

        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., this->template GetByIndex<Indices>()...);
        }

    public:
        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, this->template GetByIndex<Indices>()..., Forward<ArgTypes>(Args)...);
        }

        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const
        {
            return ::Invoke(Func, this->template GetByIndex<Indices>()..., Forward<ArgTypes>(Args)...);
        }
    };

    template<typename FirstType, typename SecondType>
    class TTupleStorage<TIntegerSequence<uint32, 0, 1>, FirstType, SecondType>
    {
        template<uint32 Index>
        struct TGetByIndexHelper;

        template<>
        struct TGetByIndexHelper<0>
        {
            typedef FirstType Type;

            NODISCARD static FORCEINLINE auto& GetByIndex(TTupleStorage& Tuple)
            {
                return Tuple.First;
            }

            NODISCARD static FORCEINLINE const auto& GetByIndex(const TTupleStorage& Tuple)
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByIndexHelper<1>
        {
            typedef SecondType Type;

            NODISCARD static FORCEINLINE auto& GetByIndex(TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }

            NODISCARD static FORCEINLINE const auto& GetByIndex(const TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }
        };

        template<typename T, bool bIsFirstType>
        struct TGetByElement;

        template<typename T>
        using TGetByElementHelper = TGetByElement<T, TIsSame<T, FirstType>::Value>;

        template<>
        struct TGetByElement<FirstType, true>
        {
            typedef FirstType Type;

            NODISCARD static FORCEINLINE auto& GetByType(TTupleStorage& Tuple)
            {
                return Tuple.First;
            }

            NODISCARD static FORCEINLINE const auto& GetByType(const TTupleStorage& Tuple)
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByElement<SecondType, false>
        {
            typedef SecondType Type;

            NODISCARD static FORCEINLINE auto& GetByType(TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }

            NODISCARD static FORCEINLINE const auto& GetByType(const TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }
        };

    public:
        TTupleStorage() = default;
        TTupleStorage(TTupleStorage&&) = default;
        TTupleStorage(const TTupleStorage&) = default;

        TTupleStorage& operator=(TTupleStorage&&) = default;
        TTupleStorage& operator=(const TTupleStorage&) = default;

        FORCEINLINE explicit TTupleStorage(const FirstType& InFirst, const SecondType& InSecond)
            : First(InFirst)
            , Second(InSecond)
        {
        }

        template<typename OtherFirstType, typename OtherSecondType>
        FORCEINLINE explicit TTupleStorage(OtherFirstType&& InFirst, OtherSecondType&& InSecond)
            : First(Forward<OtherFirstType>(InFirst))
            , Second(Forward<OtherSecondType>(InSecond))
        {
        }

        FORCEINLINE void Swap(TTupleStorage& Other) noexcept
        {
            ::Swap<FirstType>(First, Other.First);
            ::Swap<SecondType>(Second, Other.Second);
        }

    public:
        template<typename ElementType> 
        NODISCARD FORCEINLINE auto& GetByType() noexcept
        {
            return TGetByElementHelper<ElementType>::GetByType(*this);
        }

        template<typename ElementType> 
        NODISCARD FORCEINLINE const auto& GetByType() const noexcept
        {
            return TGetByElementHelper<ElementType>::GetByType(*this);
        }

    public:
        template<uint32 Index> 
        NODISCARD FORCEINLINE auto& GetByIndex() noexcept
        {
            return TGetByIndexHelper<Index>::GetByIndex(*this);
        }

        template<uint32 Index> 
        NODISCARD FORCEINLINE const auto& GetByIndex() const noexcept
        {
            return TGetByIndexHelper<Index>::GetByIndex(*this);
        }

    public:
        template<typename FuncType, typename... ArgTypes> 
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., First, Second);
        }

        template<typename FuncType, typename... ArgTypes> 
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., First, Second);
        }

    public:
        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, First, Second, Forward<ArgTypes>(Args)...);
        }

        template<typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const
        {
            return ::Invoke(Func, First, Second, Forward<ArgTypes>(Args)...);
        }

    public:
        FirstType First;
        SecondType Second;
    };

    template<uint32 Index>
    struct TTupleComparators
    {
        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static FORCEINLINE bool IsEqual(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::StaticSize() == SecondTupleType::StaticSize(), "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsEqual(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() == RHS.template GetByIndex<Index - 1>());
        }

        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static FORCEINLINE bool IsLessThan(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::StaticSize() == SecondTupleType::StaticSize(), "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsLessThan(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() < RHS.template GetByIndex<Index - 1>());
        }

        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static FORCEINLINE bool IsLessThanOrEqual(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::StaticSize() == SecondTupleType::StaticSize(), "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsLessThanOrEqual(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() <= RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleComparators<0>
    {
        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static  FORCEINLINE bool IsEqual(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }

        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static FORCEINLINE bool IsLessThan(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }

        template<typename FirstTupleType, typename SecondTupleType>
        NODISCARD static FORCEINLINE bool IsLessThanOrEqual(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }
    };
}

template<typename... Types>
class TTuple : public TupleInternal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>
{
    using Super = TupleInternal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>;

public:
    using Super::ApplyAfter;
    using Super::ApplyBefore;
    using Super::GetByType;
    using Super::GetByIndex;
    using Super::Swap;

    /**
     * @brief Retrieve the number of elements 
     * @return Returns the number of elements for the tuples
     */
    NODISCARD static constexpr uint32 StaticSize()
    {
        return sizeof...(Types);
    }

    TTuple() = default;
    TTuple(TTuple&&) = default;
    TTuple(const TTuple&) = default;

    TTuple& operator=(TTuple&&) = default;
    TTuple& operator=(const TTuple&) = default;

    /**
     * @brief Constructor
     * @param Args Arguments for each type
     */
    template<typename... ArgTypes>
    FORCEINLINE explicit TTuple(ArgTypes&&... Args)
        : Super(Forward<ArgTypes>(Args)...)
    {
    }

    /**
     * @brief Retrieve the number of elements 
     * @return Returns the number of elements for the tuples
     */
    NODISCARD constexpr uint32 Size() const
    {
        return StaticSize();
    }
};

template<>
class TTuple<>
{
public:
    /**
     * @brief Retrieve the number of elements 
     * @return Returns the number of elements for the tuples
     */
    NODISCARD static constexpr uint32 StaticSize()
    {
        return 0;
    }

    TTuple() = default;
    TTuple(const TTuple&) = default;
    TTuple(TTuple&&) = default;

    TTuple& operator=(const TTuple&) = default;
    TTuple& operator=(TTuple&&) = default;

    /**
     * @brief Retrieve the number of elements 
     * @return Returns the number of elements for the tuples
     */
    NODISCARD constexpr uint32 Size() const
    {
        return StaticSize();
    }
};

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator==(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return TupleInternal::TTupleComparators<sizeof...(FirstTypes)>::IsEqual(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator!=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return !(LHS == RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator<=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return TupleInternal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThanOrEqual(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator<(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return TupleInternal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThan(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator>(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return TupleInternal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThanOrEqual(LHS, RHS) == false;
}

template<typename... FirstTypes, typename... SecondTypes>
NODISCARD inline bool operator>=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return TupleInternal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThan(LHS, RHS) == false;
}

template<uint32 SearchForIndex, typename... Types>
NODISCARD inline auto& GetByIndex(const TTuple<Types...>& Tuple) noexcept
{
    return Tuple.template GetByIndex<SearchForIndex>();
}

template<typename ElementType, typename... Types>
NODISCARD inline auto& GetByType(const TTuple<Types...>& Tuple) noexcept
{
    return Tuple.template GetByType<ElementType>();
}

template<typename... Types>
NODISCARD inline TTuple<typename TDecay<Types>::Type...> MakeTuple(Types&&... InTypes) noexcept
{
    return TTuple<typename TDecay<Types>::Type...>(Forward<Types>(InTypes)...);
}
