#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConstructible.h"
#include "Core/Templates/IntegerSequence.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Invoke.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/And.h"

/* Tuple implementation */
namespace Internal
{
    /* Forward declarations */
    template <uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex;

    template <typename WantedType, typename... Types>
    struct TTupleGetByElement;

    /* Leaf of a tuple */
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf
    {
        /* Defaults */
        TTupleLeaf() = default;
        TTupleLeaf( TTupleLeaf&& ) = default;
        TTupleLeaf( const TTupleLeaf& ) = default;
        TTupleLeaf& operator=( TTupleLeaf&& ) = default;
        TTupleLeaf& operator=( const TTupleLeaf& ) = default;

        /* Construct a leaf by the value */
        template<typename... ArgTypes,
            typename = typename TEnableIf<TAnd<TValue<(sizeof... (ArgTypes)) != 0>, TIsConstructible<ValueType, typename TDecay<ArgTypes>::Type...>>::Value>::Type>
            FORCEINLINE explicit TTupleLeaf( ArgTypes&&... Args ) noexcept
            : Value( Forward<ArgTypes>( Args )... )
        {
        }

        /* Construct from ValuType */
        FORCEINLINE explicit TTupleLeaf( const ValueType& InValue ) noexcept
            : Value( InValue )
        {
        }

        /* Construct from ValuType */
        FORCEINLINE explicit TTupleLeaf( ValueType&& InValue ) noexcept
            : Value( Move( InValue ) )
        {
        }

        /* Swap leaf with another */
        FORCEINLINE int32 Swap( TTupleLeaf& Other ) noexcept
        {
            ::Swap<ValueType>( Value, Other.Value );
            return 0;
        }

        ValueType Value;
    };

    /* Reference specialization */
    template<uint32 Index, typename ValueType>
    struct TTupleLeaf<Index, ValueType&>
    {
        /* Defalts */
        TTupleLeaf( const TTupleLeaf& ) = default;
        TTupleLeaf( TTupleLeaf&& ) = default;
        TTupleLeaf& operator=( const TTupleLeaf& ) = delete;
        TTupleLeaf& operator=( TTupleLeaf&& ) = delete;

        /* Move constructor */
        FORCEINLINE explicit TTupleLeaf( ValueType&& InValue ) noexcept
            : Value( Move( InValue ) )
        {
        }

        /* Construct from valuetype */
        FORCEINLINE explicit TTupleLeaf( const ValueType& InValue ) noexcept
            : Value( InValue )
        {
        }

        /* Construct a leaf by the value */
        template<typename...ArgTypes, typename = typename TEnableIf<TIsConstructible<ValueType, ArgTypes&&...>::Value>::Type>
        FORCEINLINE explicit TTupleLeaf( ArgTypes&&... Args ) noexcept
            : Value( Forward<ArgTypes>( Args )... )
        {
        }

        /* Swap leaf with another */
        FORCEINLINE int32 Swap( TTupleLeaf& ) noexcept
        {
            // Cannot move references?
            return 0;
        }

        ValueType Value;
    };

    /* Tuple Get By Index Implementation */
    template <uint32 Iteration, uint32 Index, typename... Types>
    struct TTupleGetByIndexHelper;

    /* Continue to next iteration if index is not the same, extract first type */
    template <uint32 Iteration, uint32 Index, typename ElementType, typename... Types>
    struct TTupleGetByIndexHelper<Iteration, Index, ElementType, Types...> : public TTupleGetByIndexHelper<Iteration + 1, Index, Types...>
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
        static FORCEINLINE ElementType& Get( TupleType& Tuple ) noexcept
        {
            return static_cast<TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }

        template <typename TupleType>
        static FORCEINLINE const ElementType& Get( const TupleType& Tuple ) noexcept
        {
            return static_cast<const TTupleLeaf<Index, ElementType>&>(Tuple).Value;
        }
    };

    /* Retrieve the element for a specific index */
    template <uint32 SearchForIndex, typename... Types>
    struct TTupleGetByIndex : public TTupleGetByIndexHelper<0, SearchForIndex, Types...>
    {
    };

    /* Tuple Get By Element Implementation */
    template <uint32 Iteration, typename WantedType, typename... Types>
    struct TTupleGetByElementHelper;

    /* Continue to next iteration if index is not the same, extract first type */
    template <uint32 Iteration, typename WantedType, typename ElementType, typename... Types>
    struct TTupleGetByElementHelper<Iteration, WantedType, ElementType, Types...> : public TTupleGetByElementHelper<Iteration + 1, WantedType, Types...>
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
        static FORCEINLINE WantedType& Get( TupleType& Tuple ) noexcept
        {
            return static_cast<TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }

        template <typename TupleType>
        static FORCEINLINE const WantedType& Get( const TupleType& Tuple ) noexcept
        {
            return static_cast<const TTupleLeaf<Iteration, WantedType>&>(Tuple).Value;
        }
    };

    /* Retrieve a specific element */
    template <typename WantedType, typename... Types>
    struct TTupleGetByElement : public TTupleGetByElementHelper<0, WantedType, Types...>
    {
    };

    /* Implementation of tuple */
    template <typename Indices, typename... Types>
    class TTupleStorage;

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
        FORCEINLINE explicit TTupleStorage( ArgTypes&&... Args ) noexcept
            : TTupleLeaf<Indices, Types>( Forward<ArgTypes>( Args ) )...
        {
        }

        /* Swap this storage with another */
        FORCEINLINE void Swap( TTupleStorage& Other ) noexcept
        {
            ExpandPacks( TTupleLeaf<Indices, Types>::Swap( static_cast<TTupleLeaf<Indices, Types>&>(Other) )... );
        }

    public:

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE ElementType& Get() noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get( *this );
        }

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE const ElementType& Get() const noexcept
        {
            typedef TTupleGetByElement<ElementType, Types...> Type;
            return Type::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE auto& GetByIndex() noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE const auto& GetByIndex() const noexcept
        {
            typedef TTupleGetByIndex<Index, Types...> IndexType;
            return IndexType::Get( *this );
        }

    public:

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

    /* Specialization of a TTuple with only two elements making it similar to TPair */
    template<typename FirstType, typename SecondType>
    class TTupleStorage<TIntegerSequence<uint32, 0, 1>, FirstType, SecondType>
    {
        template<uint32 Index>
        struct TGetByIndexHelper;

        template<>
        struct TGetByIndexHelper<0>
        {
            typedef FirstType Type;

            static FORCEINLINE FirstType& Get( TTupleStorage& Tuple )
            {
                return Tuple.First;
            }

            static FORCEINLINE const FirstType& Get( const TTupleStorage& Tuple )
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByIndexHelper<1>
        {
            typedef SecondType Type;

            static FORCEINLINE SecondType& Get( TTupleStorage& Tuple )
            {
                return Tuple.Second;
            }

            static FORCEINLINE const SecondType& Get( const TTupleStorage& Tuple )
            {
                return Tuple.Second;
            }
        };

        /* Helper to get the type by element and not by index */
        template<typename T, bool IsFirstType>
        struct TGetByElement;

        template<typename T>
        using TGetByElementHelper = TGetByElement<T, TIsSame<T, FirstType>::Value>;

        template<>
        struct TGetByElement<FirstType, true>
        {
            typedef FirstType Type;

            static FORCEINLINE FirstType& Get( TTupleStorage& Tuple )
            {
                return Tuple.First;
            }

            static FORCEINLINE const FirstType& Get( const TTupleStorage& Tuple )
            {
                return Tuple.First;
            }
        };

        template<>
        struct TGetByElement<SecondType, false>
        {
            typedef SecondType Type;

            static FORCEINLINE SecondType& Get( TTupleStorage& Tuple )
            {
                return Tuple.Second;
            }

            static FORCEINLINE const SecondType& Get( const TTupleStorage& Tuple )
            {
                return Tuple.Second;
            }
        };

    public:

        /* Defaults */
        TTupleStorage() = default;
        TTupleStorage( TTupleStorage&& ) = default;
        TTupleStorage( const TTupleStorage& ) = default;
        TTupleStorage& operator=( TTupleStorage&& ) = default;
        TTupleStorage& operator=( const TTupleStorage& ) = default;

        /* Init types */
        FORCEINLINE explicit TTupleStorage( const FirstType& InFirst, const SecondType& InSecond )
            : First( InFirst )
            , Second( InSecond )
        {
        }

        /* Init with rvalue types, with other types */
        template<typename OtherFirstType, typename OtherSecondType>
        FORCEINLINE explicit TTupleStorage( OtherFirstType&& InFirst, OtherSecondType&& InSecond )
            : First( Forward<OtherFirstType>( InFirst ) )
            , Second( Forward<OtherSecondType>( InSecond ) )
        {
        }

        /* Swap with another storage */
        FORCEINLINE void Swap( TTupleStorage& Other ) noexcept
        {
            ::Swap<FirstType>( First, Other.First );
            ::Swap<SecondType>( Second, Other.Second );
        }

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE typename TGetByElementHelper<ElementType>::Type& Get() noexcept
        {
            return TGetByElementHelper<ElementType>::Get( *this );
        }

        /* Retrieve an element in the tuple by element */
        template<typename ElementType>
        FORCEINLINE const typename TGetByElementHelper<ElementType>::Type& Get() const noexcept
        {
            return TGetByElementHelper<ElementType>::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE typename TGetByIndexHelper<Index>::Type& GetByIndex() noexcept
        {
            return TGetByIndexHelper<Index>::Get( *this );
        }

        /* Retrieve an element in the tuple by index */
        template<uint32 Index>
        FORCEINLINE const typename TGetByIndexHelper<Index>::Type& GetByIndex() const noexcept
        {
            return TGetByIndexHelper<Index>::Get( *this );
        }

        /* Apply leafs to a function, after the specified args */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyAfter( FuncType&& Func, ArgTypes&&... Args )
        {
            return ::Invoke( Func, Forward<ArgTypes>( Args )..., First, Second );
        }

        /* Apply leafs to a function, before the specified args */
        template <typename FuncType, typename... ArgTypes>
        FORCEINLINE decltype(auto) ApplyBefore( FuncType&& Func, ArgTypes&&... Args )
        {
            return ::Invoke( Func, First, Second, Forward<ArgTypes>( Args )... );
        }

        FirstType First;
        SecondType Second;
    };

    /* Equality / LessThan / LessThanOrEqual - Helpers */

    template <uint32 Index>
    struct TTupleEquallityHelper
    {
        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsEqual( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleEquallityHelper<Index - 1>::IsEqual( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() == RHS.template GetByIndex<Index - 1>());
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThan( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleEquallityHelper<Index - 1>::IsLessThan( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() < RHS.template GetByIndex<Index - 1>());
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThanOrEqual( const FirstTupleType& LHS, const SecondTupleType& RHS )
        {
            static_assert(FirstTupleType::NumElements == SecondTupleType::NumElements, "Tuples must have equal size");
            return TTupleEquallityHelper<Index - 1>::IsLessThanOrEqual( LHS, RHS ) && (LHS.template GetByIndex<Index - 1>() <= RHS.template GetByIndex<Index - 1>());
        }
    };

    template <>
    struct TTupleEquallityHelper<0>
    {
        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsEqual( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThan( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }

        template <typename FirstTupleType, typename SecondTupleType>
        static FORCEINLINE bool IsLessThanOrEqual( const FirstTupleType&, const SecondTupleType& )
        {
            return true;
        }
    };
}

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
    using Super::Swap;

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
    return Internal::TTupleEquallityHelper<sizeof...(FirstTypes)>::IsLessThanOrEqual( LHS, RHS );
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator<( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return Internal::TTupleEquallityHelper<sizeof...(FirstTypes)>::IsLessThan( LHS, RHS );
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return (Internal::TTupleEquallityHelper<sizeof...(FirstTypes)>::IsLessThanOrEqual( LHS, RHS ) == false);
}

template<typename... FirstTypes, typename... SecondTypes>
inline bool operator>=( const TTuple<FirstTypes...>& LHS, const TTuple<SecondTypes...>& RHS )
{
    return (Internal::TTupleEquallityHelper<sizeof...(FirstTypes)>::IsLessThan( LHS, RHS ) == false);
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

template<typename... Types>
inline TTuple<Types...> MakeTuple( Types&&... InTypes ) noexcept
{
    return TTuple<Types...>( Forward<Types>( InTypes )... );
}