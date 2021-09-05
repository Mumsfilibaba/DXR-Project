#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConstructible.h"
#include "Core/Templates/IntegerSequence.h"
#include "Core/Templates/Invoke.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/And.h"

/* Forward declarations */
template <uint32 SearchForIndex, typename... Types>
class TTupleGetByIndex;

template <typename WantedType, typename... Types>
class TTupleGetByElement;

/* Tuple implementation */
namespace Internal
{
    /* Leaf of a tuple */
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf
    {
        /* Empty default */
        FORCEINLINE TTupleLeaf()
            : Value()
        {
        }

        /* Construct a leaf by the value */
        template<typename... ArgTypes,
            typename = typename TEnableIf<TAnd<TValue<(sizeof... (ArgTypes)) != 0>, TIsConstructible<ValueType, typename TDecay<ArgTypes>::Type...>>::Value>::Type>
        FORCEINLINE explicit TTupleLeaf( ArgTypes&&... Args )
            : Value( Forward<ArgTypes>( Args )... )
        {
        }

        /* Move constructor */
        FORCEINLINE TTupleLeaf( TTupleLeaf&& Other )
            : Value( Move( Other.Value ) )
        {
        }

        /* Copy constructor */
        FORCEINLINE TTupleLeaf( const TTupleLeaf& Other )
            : Value( Other.Value )
        {
        }

        /* Move assignment */
        FORCEINLINE TTupleLeaf& operator=( TTupleLeaf&& Other )
        {
            Value = Move( Other.Value );
            return *this;
        }

        /* Copy assignment */
        FORCEINLINE TTupleLeaf& operator=( const TTupleLeaf& Other )
        {
            Value = Other.Value;
            return *this;
        }

        ValueType Value;
    };

    /* Reference specialization */
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf<Index, ValueType&>
    {
        TTupleLeaf() = default;
        TTupleLeaf& operator=( const TTupleLeaf& Other ) = delete;
        TTupleLeaf& operator=( TTupleLeaf&& Other ) = delete;

        /* Construct a leaf by the value */
        template<typename...ArgTypes, typename = typename TEnableIf<TIsConstructible<ValueType, ArgTypes&&...>::Value>::Type>
        FORCEINLINE explicit TTupleLeaf( ArgTypes&&... Args )
            : Value( Forward<ArgTypes>( Args )... )
        {
        }

        FORCEINLINE explicit TTupleLeaf( ValueType& Value )
            : Value( Value )
        {
        }

        /* Copy constructor */
        FORCEINLINE TTupleLeaf( const TTupleLeaf& Other )
            : Value( Other.Value )
        {
        }

        ValueType Value;
    };

    /* Tuple Get By Index Implementation */
    template <uint32 Iteration, uint32 Index, typename... Types>
    struct TTupleGetByIndexHelper;

    /* Continue to next iteration if index is not the same, extract first type */
    template <uint32 Iteration, uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Iteration, Index, ElementType, Types...> : TTupleGetByIndexHelper<Iteration + 1, Index, Types...>
    {
    };

    /* When indices are equal the correct element is found */
    template <uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Index, Index, ElementType, Types...>
    {
        typedef ElementType Type;

        enum
        {
            ElementIndex = Index
        };

        template <typename TupleType>
        FORCEINLINE static ElementType& Get( TupleType& Tuple )
        {
            return static_cast<TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }

        template <typename TupleType>
        FORCEINLINE static const ElementType& Get( const TupleType& Tuple )
        {
            return static_cast<const TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }
    };

    /* Tuple Get By Element Implementation */
    template <uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper;

    /* Continue to next iteration if index is not the same, extract first type */
    template <uint32 Iteration, typename WantedType, typename ElementType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, ElementType, Types...> : TTupleGetByElementHelper<Iteration + 1, WantedType, Types...>
    {
    };

    /* When indices are equal the correct element is found */
    template <uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, WantedType, Types...>
    {
        typedef WantedType Type;

        enum
        {
            ElementIndex = Iteration
        };

        template <typename TupleType>
        FORCEINLINE static WantedType& Get( TupleType& Tuple )
        {
            return static_cast<TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }

        template <typename TupleType>
        FORCEINLINE static const WantedType& Get( const TupleType& Tuple )
        {
            return static_cast<const TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }
    };

    /* Implementation of tuple */
    template <typename Indices, typename... Types>
    struct TTupleStorage;

    template<uint32... Indices, typename... Types>
    class TTupleStorage<TIntegerSequence<uint32, Indices...>, Types...> : public TTupleLeaf<Indices, Types>...
    {
    public:

        /* Defaults */
        TTupleStorage() = default;
        TTupleStorage( TTupleStorage&& ) = default;
        TTupleStorage( const TTupleStorage& ) = default;
        TTupleStorage& operator=( TTupleStorage&& ) = default;
        TTupleStorage& operator=( const TTupleStorage& ) = default;

        /* Constructor forwarding types */
        template<typename... ArgTypes>
        FORCEINLINE TTupleStorage( ArgTypes&&... Args )
            : TTupleLeaf<Indices, Types>( Forward<ArgTypes>( Args ) )...
        {
        }

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE auto& Get()
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get( *this );
        }

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE const auto& Get() const
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE auto& GetByIndex()
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE const auto& GetByIndex() const
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get( *this );
        }

        /* Apply leafs to a function, after the specified args */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter( FuncType&& Func, ArgTypes&&... Args )
        {
            return ::Invoke( Func, Forward<ArgTypes>( Args )..., this->template GetByIndex<Indices>()... );
        }

        /* Apply leafs to a function, before the specified args */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore( FuncType&& Func, ArgTypes&&... Args )
        {
            return ::Invoke( Func, this->template GetByIndex<Indices>()..., Forward<ArgTypes>( Args )... );
        }
    };

    /* Equality / LessThan / LessThanOrEqual - Helpers */

    template <uint32 Index>
    struct TTupleEquallityHelper
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsEqual( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleEquallityHelper<Index - 1>::IsEqual( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() == RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleEquallityHelper<0>
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsEqual( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }
    };

    template <uint32 Index>
    struct TTupleLessThanHelper
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsLessThan( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleLessThanHelper<Index - 1>::IsLessThan( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() < RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleLessThanHelper<0>
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsLessThan( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }
    };

    template <uint32 Index>
    struct TTupleLessThanOrEqualHelper
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsLessThanOrEqual( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleLessThanOrEqualHelper<Index - 1>::IsLessThanOrEqual( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() <= RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleLessThanOrEqualHelper<0>
    {
        template <typename FirstTupleType, typename SecondTupleType>
        FORCEINLINE static bool IsLessThanOrEqual( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }
    };
}

/* Retrieve the element for a specific index */
template <uint32 SearchForIndex, typename... Types>
class TTupleGetByIndex : Internal::TTupleGetByIndexHelper<0, SearchForIndex, Types...>
{
    using Super = Internal::TTupleGetByIndexHelper<0, SearchForIndex, Types...>;

public:
    using Super::Get;

public:

    enum
    {
        Index = Super::ElementIndex
    };

    using ElementType = typename Super::Type;
};

/* Retrieve a specific element */
template <typename WantedType, typename... Types>
class TTupleGetByElement : Internal::TTupleGetByElementHelper<0, WantedType, Types...>
{
    using Super = Internal::TTupleGetByElementHelper<0, WantedType, Types...>;

public:
    using Super::Get;

public:

    enum
    {
        Index = Super::ElementIndex
    };

    using ElementType = typename Super::Type;
};

/* Tuple implementation */
template<typename... Types>
class TTuple : public Internal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>
{
    using Super = Internal::TTupleStorage<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>;

public:
    using Super::ApplyAfter;
    using Super::ApplyBefore;
    using Super::Get;
    using Super::GetByIndex;

public:

    enum
    {
        NumElements = sizeof...(Types)
    };

    /* Defaults */
    TTuple() = default;
    TTuple( TTuple&& ) = default;
    TTuple( const TTuple& ) = default;
    TTuple& operator=( TTuple&& ) = default;
    TTuple& operator=( const TTuple& ) = default;

    /* Constructor forwarding types */
    template<typename... ArgTypes, typename = typename TEnableIf<(NumElements > 0)>::Type>
    FORCEINLINE explicit TTuple( ArgTypes&&... Args )
        : Super( Forward<ArgTypes>( Args )... )
    {
    }

public:

    /* The number of elements in the tuple */
    constexpr uint32 Size() const
    {
        return NumElements;
    }
};

/* Operators */
template<typename... FirstTypes, typename... SecondTypes>
inline bool operator==( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return Internal::TTupleEquallityHelper<sizeof...(FirstTypes)>::IsEqual( LHS, RHS );
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator!=( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return !(LHS == RHS);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator<=( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
	return Internal::TTupleLessThanOrEqualHelper<sizeof...(FirstTypes)>::IsLessThanOrEqual( LHS, RHS );
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator<( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
	return Internal::TTupleLessThanHelper<sizeof...(FirstTypes)>::IsLessThan( LHS, RHS );
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
	return (Internal::TTupleLessThanOrEqualHelper<sizeof...(FirstTypes)>::IsLessThanOrEqual( LHS, RHS ) == false);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>=( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return (Internal::TTupleLessThanHelper<sizeof...(FirstTypes)>::IsLessThan( LHS, RHS ) == false);
}

/* Helpers */

template<uint32 SearchForIndex, typename... Types>
inline auto& TupleGetByIndex( const TTuple<Types...>& Tuple ) noexcept
{
    return Tuple.template GetByIndex<SearchForIndex>();
}

template<typename ElementType, typename... Types>
inline auto& TupleGet( const TTuple<Types...>& Tuple ) noexcept
{
    return Tuple.template Get<ElementType>();
}
