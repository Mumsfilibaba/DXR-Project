#pragma once
#include "Core/CoreTypes.h"
#include "Core/CoreDefines.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConstructible.h"
#include "Core/Templates/IntegerSequence.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Invoke.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/And.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers for TTuple implementation

namespace Internal
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Forward declarations

    template <uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex;

    template <typename WantedType, typename... Types>
    struct TTupleGetByElement;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Leaf of a tuple
    
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf
    {
        TTupleLeaf() = default;
        TTupleLeaf(TTupleLeaf&&) = default;
        TTupleLeaf(const TTupleLeaf&) = default;
        TTupleLeaf& operator=(TTupleLeaf&&) = default;
        TTupleLeaf& operator=(const TTupleLeaf&) = default;

        template<typename... ArgTypes,
            typename = typename TEnableIf<TAnd<TValue<(sizeof... (ArgTypes)) != 0>, TIsConstructible<ValueType, typename TDecay<ArgTypes>::Type...>>::Value>::Type>
            FORCEINLINE explicit TTupleLeaf(ArgTypes&&... Args) noexcept
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

        FORCEINLINE int32 Swap(TTupleLeaf& Other) noexcept
        {
            ::Swap<ValueType>(Value, Other.Value);
            return 0;
        }

        ValueType Value;
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Reference TTupleLeaf-specialization
    
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf<Index, ValueType&>
    {
        TTupleLeaf(const TTupleLeaf&) = default;
        TTupleLeaf(TTupleLeaf&&) = default;
        TTupleLeaf& operator=(const TTupleLeaf&) = delete;
        TTupleLeaf& operator=(TTupleLeaf&&) = delete;

        FORCEINLINE explicit TTupleLeaf(ValueType&& InValue) noexcept
            : Value(Move(InValue))
        {
        }

        FORCEINLINE explicit TTupleLeaf(const ValueType& InValue) noexcept
            : Value(InValue)
        {
        }

        template<typename...ArgTypes, typename = typename TEnableIf<TIsConstructible<ValueType, ArgTypes&&...>::Value>::Type>
        FORCEINLINE explicit TTupleLeaf(ArgTypes&&... Args) noexcept
            : Value(Forward<ArgTypes>(Args)...)
        {
        }

        FORCEINLINE int32 Swap(TTupleLeaf&) noexcept
        {
            return 0;
        }

        ValueType Value;
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Tuple GetByIndex Implementation
    
    template <uint32 Iteration, uint32 Index, typename... Types>
    struct TTupleGetByIndexHelper;

    template <uint32 Iteration, uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Iteration, Index, ElementType, Types...> : public TTupleGetByIndexHelper<Iteration + 1, Index, Types...>
    { };

    template <uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Index, Index, ElementType, Types...>
    {
        using Type = ElementType;

        enum
        {
            ElementIndex = Index
        };

        template<typename TupleType>
        static FORCEINLINE ElementType& Get(TupleType& Tuple) noexcept
        {
            return static_cast<TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }

        template<typename TupleType>
        static FORCEINLINE const ElementType& Get(const TupleType& Tuple) noexcept
        {
            return static_cast<const TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }
    };

    template <uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex : public TTupleGetByIndexHelper<0, SearchForIndex, Types...>
    { };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Tuple GetByElement Implementation

    template <uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper;

    template <uint32 Iteration, typename WantedType, typename ElementType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, ElementType, Types...> : public TTupleGetByElementHelper<Iteration + 1, WantedType, Types...>
    { };

    template <uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, WantedType, Types...>
    {
        typedef WantedType Type;

        enum
        {
            ElementIndex = Iteration
        };

        template <typename TupleType>
        static FORCEINLINE WantedType& Get(TupleType& Tuple) noexcept
        {
            return static_cast<TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }

        template <typename TupleType>
        static FORCEINLINE const WantedType& Get(const TupleType& Tuple) noexcept
        {
            return static_cast<const TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }
    };

    template <typename WantedType, typename... Types>
    struct TTupleGetByElement : public TTupleGetByElementHelper<0, WantedType, Types...>
    { };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Implementation of tuple

    template <typename Indices, typename... Types>
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

        /**
         * Create a new tuple
         * 
         * @param Args: Arguments for the element
         */
        template<typename... ArgTypes>
        FORCEINLINE explicit TTupleStorage(ArgTypes&&... Args) noexcept
            : TTupleLeaf<Indices, Types>(Forward<ArgTypes>(Args))...
        {
        }

        /**
         * Swap this tuple with another
         * 
         * @param Other: Tuple to swap with  
         */
        FORCEINLINE void Swap(TTupleStorage& Other) noexcept
        {
            ExpandPacks(TTupleLeaf<Indices, Types>::Swap(static_cast<TTupleLeaf<Indices, Types>&>(Other))...);
        }

    public:

        /**
         * Retrieve a certain element based on type
         * 
         * @return: Returns a reference to the data of the specified type
         */
        template<typename ElementType>
        FORCEINLINE ElementType& Get() noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get(*this);
        }

        /**
          * Retrieve a certain element based on type
          *
          * @return: Returns a reference to the data of the specified type
          */
        template<typename ElementType>
        FORCEINLINE const ElementType& Get() const noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get(*this);
        }

        /**
          * Retrieve a certain element with the specified index
          *
          * @return: Returns a reference with the specified index
          */
        template<uint32 Index>
        FORCEINLINE auto& GetByIndex() noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get(*this);
        }

        /**
          * Retrieve a certain element with the specified index
          *
          * @return: Returns a reference with the specified index
          */
        template<uint32 Index>
        FORCEINLINE const auto& GetByIndex() const noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get(*this);
        }

    public:

        /**
         * Apply the elements as arguments to a function after the arguments
         * 
         * @param Func: Function to call
         * @param Args: Arguments to before after the tuple
         * @return: Return the value from the function-call
         */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., this->template GetByIndex<Indices>()...);
        }

        /**
         * Apply the elements as arguments to a function before the arguments
         *
         * @param Func: Function to call
         * @param Args: Arguments to apply after the tuple
         * @return: Return the value from the function-call
         */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, this->template GetByIndex<Indices>()..., Forward<ArgTypes>(Args)...);
        }
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Specialization of a TTuple with only two elements making it similar to TPair

    template<typename FirstType, typename SecondType>
    class TTupleStorage<TIntegerSequence<uint32, 0, 1>, FirstType, SecondType>
    {
        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // GetByIndexHelper

        template<uint32 Index>
        struct TGetByIndexHelper;

        template<>
        struct TGetByIndexHelper<0>
        {
            typedef FirstType Type;

            static FORCEINLINE FirstType& Get(TTupleStorage& Tuple)
            {
                return Tuple.First;
            }

            static FORCEINLINE const FirstType& Get(const TTupleStorage& Tuple)
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByIndexHelper<1>
        {
            typedef SecondType Type;

            static FORCEINLINE SecondType& Get(TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }

            static FORCEINLINE const SecondType& Get(const TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }
        };

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // GetByElement

        template<typename T, bool IsFirstType>
        struct TGetByElement;

        template<typename T>
        using TGetByElementHelper = TGetByElement<T, TIsSame<T, FirstType>::Value>;

        template<>
        struct TGetByElement<FirstType, true>
        {
            typedef FirstType Type;

            static FORCEINLINE FirstType& Get(TTupleStorage& Tuple)
            {
                return Tuple.First;
            }

            static FORCEINLINE const FirstType& Get(const TTupleStorage& Tuple)
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByElement<SecondType, false>
        {
            typedef SecondType Type;

            static FORCEINLINE SecondType& Get(TTupleStorage& Tuple)
            {
                return Tuple.Second;
            }

            static FORCEINLINE const SecondType& Get(const TTupleStorage& Tuple)
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

        /**
         * Constructor
         * 
         * @param InFirst: Element of the first type to copy
         * @param InSecond: Element of the second type to copy
         */
        FORCEINLINE explicit TTupleStorage(const FirstType& InFirst, const SecondType& InSecond)
            : First(InFirst)
            , Second(InSecond)
        {
        }

        /**
         * Constructor with templated types
         *
         * @param InFirst: Element of the first type to copy
         * @param InSecond: Element of the second type to copy
         */
        template<typename OtherFirstType, typename OtherSecondType>
        FORCEINLINE explicit TTupleStorage(OtherFirstType&& InFirst, OtherSecondType&& InSecond)
            : First(Forward<OtherFirstType>(InFirst))
            , Second(Forward<OtherSecondType>(InSecond))
        {
        }

        /**
         * Swap this tuple with another
         *
         * @param Other: Tuple to swap with
         */
        FORCEINLINE void Swap(TTupleStorage& Other) noexcept
        {
            ::Swap<FirstType>(First, Other.First);
            ::Swap<SecondType>(Second, Other.Second);
        }

        /**
         * Retrieve a certain element based on type
         *
         * @return: Returns a reference to the data of the specified type
         */
        template<typename ElementType>
        FORCEINLINE typename TGetByElementHelper<ElementType>::Type& Get() noexcept
        {
            return TGetByElementHelper<ElementType>::Get(*this);
        }

        /**
         * Retrieve a certain element based on type
         *
         * @return: Returns a reference to the data of the specified type
         */
        template<typename ElementType>
        FORCEINLINE const typename TGetByElementHelper<ElementType>::Type& Get() const noexcept
        {
            return TGetByElementHelper<ElementType>::Get(*this);
        }

        /**
          * Retrieve a certain element with the specified index
          *
          * @return: Returns a reference with the specified index
          */
        template<uint32 Index>
        FORCEINLINE typename TGetByIndexHelper<Index>::Type& GetByIndex() noexcept
        {
            return TGetByIndexHelper<Index>::Get(*this);
        }

        /**
          * Retrieve a certain element with the specified index
          *
          * @return: Returns a reference with the specified index
          */
        template<uint32 Index>
        FORCEINLINE const typename TGetByIndexHelper<Index>::Type& GetByIndex() const noexcept
        {
            return TGetByIndexHelper<Index>::Get(*this);
        }

        /**
         * Apply the elements as arguments to a function after the arguments
         *
         * @param Func: Function to call
         * @param Args: Arguments to before after the tuple
         * @return: Return the value from the function-call
         */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, Forward<ArgTypes>(Args)..., First, Second);
        }

        /**
         * Apply the elements as arguments to a function before the arguments
         *
         * @param Func: Function to call
         * @param Args: Arguments to apply after the tuple
         * @return: Return the value from the function-call
         */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)
        {
            return ::Invoke(Func, First, Second, Forward<ArgTypes>(Args)...);
        }

        FirstType First;
        SecondType Second;
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TupleEquallityHelper - Equality / LessThan / LessThanOrEqual - Helpers

    template <uint32 Index>
    struct TTupleComparators
    {
        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsEqual(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsEqual(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() == RHS.template GetByIndex<Index - 1>());
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThan(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsLessThan(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() < RHS.template GetByIndex<Index - 1>());
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThanOrEqual(const FirstTupleType& LHS, const SecondTupleType& RHS)
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleComparators<Index - 1>::IsLessThanOrEqual(LHS, RHS) && (LHS.template GetByIndex<Index - 1>() <= RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleComparators<0>
    {
        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsEqual(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThan(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThanOrEqual(const FirstTupleType&, const SecondTupleType&)
        {
            return true;
        }
    };
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TTuple implementation - similar to std::tuple

template<typename... Types>
class TTuple : public Internal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>
{
    using Super = Internal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>;

public:
    using Super::ApplyAfter;
    using Super::ApplyBefore;
    using Super::Get;
    using Super::GetByIndex;
    using Super::Swap;

public:

    enum
    {
        NumElements = sizeof...(Types)
    };

    TTuple() = default;
    TTuple(TTuple&&) = default;
    TTuple(const TTuple&) = default;
    TTuple& operator=(TTuple&&) = default;
    TTuple& operator=(const TTuple&) = default;

    /**
     * @brief: Constructor
     * 
     * @param Args: Arguments for each type
     */
    template<typename... ArgTypes, typename = typename TEnableIf<(NumElements > 0)>::Type>
    FORCEINLINE explicit TTuple(ArgTypes&&... Args)
        : Super(Forward<ArgTypes>(Args)...)
    { }

public:

    /**
     * @brief: Retrieve the number of elements 
     * 
     * @return: Returns the number of elements for the tuples
     */
    CONSTEXPR uint32 Size() const
    {
        return NumElements;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator==(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return Internal::TTupleComparators<sizeof...(FirstTypes)>::IsEqual(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator!=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return !(LHS == RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator<=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return Internal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThanOrEqual(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator<(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return Internal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThan(LHS, RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return (Internal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThanOrEqual(LHS, RHS) == false);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>=(const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS)
{
    return (Internal::TTupleComparators<sizeof...(FirstTypes)>::IsLessThan(LHS, RHS) == false);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

template<uint32 SearchForIndex, typename... Types>
inline auto& TupleGetByIndex(const TTuple<Types...>& Tuple) noexcept
{
    return Tuple.template GetByIndex<SearchForIndex>();
}

template<typename ElementType, typename... Types>
inline auto& TupleGet(const TTuple<Types...>& Tuple) noexcept
{
    return Tuple.template Get<ElementType>();
}

template<typename... Types>
inline TTuple<Types...> MakeTuple(Types&&... InTypes) noexcept
{
    return TTuple<Types...>(Forward<Types>(InTypes)...);
}