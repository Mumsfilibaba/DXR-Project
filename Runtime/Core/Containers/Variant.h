#pragma once
#include "Core/Templates/AlignedStorage.h"
#include "Core/Templates/MinMax.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/InPlace.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TVariant - Similar to std::variant

template<typename... Types>
class TVariant
{
    using TypeIndexType = int32;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Constants

    inline static constexpr TypeIndexType SizeInBytes      = TMax<sizeof(Types)...>::Value;
    inline static constexpr TypeIndexType AlignmentInBytes = TMax<alignof(Types)...>::Value;
    inline static constexpr TypeIndexType InvalidTypeIndex = -1;
    inline static constexpr TypeIndexType MaxTypeIndex     = sizeof... (Types);

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantIndex - Retrieve the Type index

    template<TypeIndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper
    {
        enum
        {
            Value = InvalidTypeIndex
        };
    };

    template<TypeIndexType CurrentIndex, typename WantedType, typename ElementType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, ElementType, OtherTypes...> 
    {
        enum
        {
            Value = TVariantIndexHelper<CurrentIndex + 1, WantedType, OtherTypes...>::Value
        };
    };

    template<TypeIndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, WantedType, OtherTypes...>
    {
        enum
        {
            Value = CurrentIndex
        };
    };

    template<typename WantedType>
    struct TVariantIndex
    {
        enum
        {
            Value = TVariantIndexHelper<0, WantedType, Types...>::Value
        };
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantType - Retrieve type from index

    template<TypeIndexType CurrentIndex, TypeIndexType Index, typename... OtherTypes>
    struct TVariantTypeHelper;

    template<TypeIndexType CurrentIndex, TypeIndexType Index, typename ElementType, typename... OtherTypes>
    struct TVariantTypeHelper<CurrentIndex, Index, ElementType, OtherTypes...>
    {
        using Type = typename TVariantTypeHelper<CurrentIndex + 1, Index, OtherTypes...>::Type;
    };

    template<TypeIndexType Index, typename ElementType, typename... OtherTypes>
    struct TVariantTypeHelper<Index, Index, ElementType, OtherTypes...>
    {
        using Type = ElementType;
    };

    template<TypeIndexType SearchForIndex>
    struct TVariantType
    {
        using Type = typename TVariantTypeHelper<0, SearchForIndex, Types...>::Type;
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantDestructorTable

    template<typename T>
    struct TVariantDestructor
    {
        static void Destruct(void* Memory) noexcept
        {
            typedef T TypeDestructor;
            reinterpret_cast<TypeDestructor*>(Memory)->TypeDestructor::~TypeDestructor();
        }
    };

    struct TVariantDestructorTable
    {
        static void Destruct(TypeIndexType Index, void* Memory) noexcept
        {
            static constexpr void(*Table[])(void*) = { &TVariantDestructor<Types>::Destruct... };

            Assert(Index < ArrayCount(Table));
            Table[Index](Memory);
        }
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantCopyConstructorTable

    template<typename T>
    struct TVariantCopyConstructor
    {
        static void Copy(void* Memory, const void* Value) noexcept
        {
            new(Memory) T(*reinterpret_cast<const T*>(Value));
        }
    };

    struct TVariantCopyConstructorTable
    {
        static void Copy(TypeIndexType Index, void* Memory, const void* Value) noexcept
        {
            static constexpr void(*Table[])(void*, void*) = { &TVariantCopyConstructor<Types>::Copy... };

            Assert(Index < ArrayCount(Table));
            Table[Index](Memory, Value);
        }
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantMoveConstructorTable

    template<typename T>
    struct TVariantMoveConstructor
    {
        static void Move(void* Memory, void* Value) noexcept
        {
            new(Memory) T(::Move(*reinterpret_cast<T*>(Value)));
        }
    };

    struct TVariantMoveConstructorTable
    {
        static void Move(TypeIndexType Index, void* Memory, void* Value) noexcept
        {
            static constexpr void(*Table[])(void*, void*) = { &TVariantMoveConstructor<Types>::Move... };

            Assert(Index < ArrayCount(Table));
            Table[Index](Memory, Value);
        }
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantCompareTable

    template<typename T>
    struct TVariantComparators
    {
        static bool IsEqual(const void* Lhs, const void* Rhs) noexcept
        {
            return (*reinterpret_cast<const T*>(Lhs)) == (*reinterpret_cast<const T*>(Rhs));
        }

        static bool IsLessThan(const void* Lhs, const void* Rhs) noexcept
        {
            return (*reinterpret_cast<const T*>(Lhs)) < (*reinterpret_cast<const T*>(Rhs));
        }
    };

    struct TVariantComparatorsTable
    {
        static bool IsEqual(TypeIndexType Index, const void* Lhs, const void* Rhs) noexcept
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TVariantComparators<Types>::IsEqual... };

            Assert(Index < ArrayCount(Table));
            return Table[Index](Lhs, Rhs);
        }

        static bool IsLessThan(TypeIndexType Index, const void* Lhs, const void* Rhs) noexcept
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TVariantComparators<Types>::IsLessThan... };

            Assert(Index < ArrayCount(Table));
            return Table[Index](Lhs, Rhs);
        }
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TIsValidType - Checks if the templated type is one of the Variant-types

    template<typename T>
    struct TIsValidType
    {
        enum
        {
            Value = (TVariantIndex<T>::Value != InvalidTypeIndex) ? 1 : 0
        };
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TIsValidIndex - Checks if the templated index valid

    template<TypeIndexType Index>
    struct TIsValidIndex
    {
        enum
        {
            Value = (Index < MaxTypeIndex) ? 1 : 0
        };
    };

public:
    
    /**
     * Default constructor
     */
    FORCEINLINE TVariant() noexcept
        : Value()
        , TypeIndex(InvalidTypeIndex)
    {
    }

    /**
     * In-Place constructor that constructs a variant of specified type with arguments for the types constructor
     * 
     * @param Args: Arguments for the elements constructor
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceType<T>, ArgTypes&&... Args)
        : Value()
        , TypeIndex(TVariantIndex<T>::Value)
    {
        static_assert(TIsValidType<T>::Value, "This TVariant is not specified to hold the specified type");
        Construct<T>(Forward<ArgTypes>(Args)...);
    }

    /**
     * In-Place constructor that constructs a variant of specified type with arguments for the types constructor
     *
     * @param Args: Arguments for the elements constructor
     */
    template<TypeIndexType InIndex, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceIndex<InIndex>, ArgTypes&&... Args)
        : Value()
        , TypeIndex(InIndex)
    {
        static_assert(TIsValidIndex<InIndex>::Value, "Specified index is not valid with this TVariant");

        typedef typename TVariantType<InIndex>::Type ConstructType;
        Construct<ConstructType>(Forward<ArgTypes>(Args)...);
    }

    /**
     * Copy constructor
     * 
     * @param Other: Variant to copy from
     */
    FORCEINLINE TVariant(const TVariant& Other) noexcept
        : Value()
        , TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            CopyFrom(Other);
        }
    }

    /**
     * Move constructor
     *
     * @param Other: Variant to move from
     */
    FORCEINLINE TVariant(TVariant&& Other) noexcept
        : Value()
        , TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            MoveFrom(Other);
            Other.Destruct();
            Other.TypeIndex = InvalidTypeIndex;
        }
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TVariant()
    {
        Reset();
    }

    /**
     * Create a value in-place 
     * 
     * @param Args: Arguments for the constructor of the element
     * @return: Returns a reference to the newly created element
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, typename TRemoveReference<T>::Type&>::Type Emplace(ArgTypes&&... Args) noexcept
    {
        Reset();

        Construct<T>(Forward<ArgTypes>(Args)...);

        TypeIndex = TVariantIndex<T>::Value;

        return *Value.CastStorage<T>();
    }

    /**
     * Resets the variant and calls the destructor
     */
    FORCEINLINE void Reset() noexcept
    {
        if (IsValid())
        {
            Destruct();
            TypeIndex = InvalidTypeIndex;
        }
    }

    /**
     * Swap this variant with another
     * 
     * @param Other: Variant to swap with
     */
    FORCEINLINE void Swap(TVariant& Other) noexcept
    {
        TVariant TempVariant;
        if (TypeIndex != InvalidTypeIndex)
        {
            TempVariant.MoveFrom(*this);
            Destruct();

            TempVariant.TypeIndex = TypeIndex;
        }

        if (Other.TypeIndex != InvalidTypeIndex)
        {
            MoveFrom(Other);
            Other.Destruct();

            TypeIndex = Other.TypeIndex;
        }
        else
        {
            TypeIndex = InvalidTypeIndex;
        }

        if (TempVariant.TypeIndex != InvalidTypeIndex)
        {
            Other.MoveFrom(TempVariant);
            Other.TypeIndex = TempVariant.TypeIndex;
        }
        else
        {
            Other.TypeIndex = InvalidTypeIndex;
        }
    }

    /**
     * Check if the templated type is the current type
     * 
     * @return: Returns true if the templated type is the currently held value
     */
    template<typename T>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, bool>::Type IsType() const noexcept
    {
        return (TypeIndex == TVariantIndex<T>::Value);
    }

    /**
     * Retrieve the currently held value
     *
     * @return: Returns a reference to the currently held value
     */
    template<typename T>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, typename TRemoveReference<T>::Type&>::Type GetValue() noexcept
    {
        Assert(IsValid() && IsType<T>());
        return *Value.CastStorage<T>();
    }

    /**
     * Retrieve the currently held value
     * 
     * @return: Returns a reference to the currently held value
     */
    template<typename T>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, const typename TRemoveReference<T>::Type&>::Type GetValue() const noexcept
    {
        Assert(IsValid() && IsType<T>());
        return *Value.CastStorage<T>();
    }

    /**
     * Try and retrieve the currently held value, or get nullptr if value of specified type is not held
     *
     * @return: Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, typename TRemoveReference<T>::Type*>::Type TryGetValue() noexcept
    {
        return IsType<T>() ? Value.CastStorage<T>() : nullptr;
    }

    /**
     * Try and retrieve the currently held value, or get nullptr if value of specified type is not held
     *
     * @return: Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    FORCEINLINE typename TEnableIf<TIsValidType<T>::Value, const typename TRemoveReference<T>::Type*>::Type TryGetValue() const noexcept
    {
        return IsType<T>() ? Value.CastStorage<T>() : nullptr;
    }

    /**
     * Retrieve the type index of the currently held value
     * 
     * @return: Returns the index of the current held value
     */
    FORCEINLINE TypeIndexType GetIndex() const noexcept
    {
        return TypeIndex;
    }

    /**
     * Check if the Variant is valid or not
     * 
     * @return: Returns true if the variant holds a value
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (TypeIndex != InvalidTypeIndex);
    }

public:

    /**
     * Copy assignment operator
     * 
     * @param Rhs: Variant to copy from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(const TVariant& Rhs) noexcept
    {
        TVariant(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move assignment operator
     *
     * @param Rhs: Variant to move from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(TVariant&& Rhs) noexcept
    {
        TVariant(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Comparison operator
     *
     * @param Lhs: Left side to compare with
     * @param Rhs: Right side to compare with
     * @return: Returns true if the variants are equal
     */
    friend FORCEINLINE bool operator==(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        if (Lhs.TypeIndex != Rhs.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!Lhs.IsValid())
        {
            return true;
        }

        return Lhs.IsEqual(Rhs);
    }

    /**
     * Comparison operator
     *
     * @param Lhs: Left side to compare with 
     * @param Rhs: Right side to compare with
     * @return: Returns false if the variants are equal
     */
    friend FORCEINLINE bool operator!=(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        return !(Lhs == Rhs);
    }

    /**
     * Less than comparison operator
     *
     * @param Lhs: Left side to compare with
     * @param Rhs: Right side to compare with
     * @return: Returns true if Lhs is less than Rhs
     */
    friend FORCEINLINE bool operator<(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        if (Lhs.TypeIndex != Rhs.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!Lhs.IsValid())
        {
            return true;
        }

        return Lhs.IsLessThan(Rhs);
    }

    /**
     * Less than or equal comparison operator
     *
     * @param Lhs: Left side to compare with
     * @param Rhs: Right side to compare with
     * @return: Returns true if Lhs is less than or equal to Rhs
     */
    friend FORCEINLINE bool operator<=(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        if (Lhs.TypeIndex != Rhs.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!Lhs.IsValid())
        {
            return true;
        }

        return Lhs.IsLessThan(Rhs) || Lhs.IsEqual(Rhs);
    }

    /**
     * Greater than comparison operator
     *
     * @param Lhs: Left side to compare with
     * @param Rhs: Right side to compare with
     * @return: Returns true if Lhs is greater than Rhs
     */
    friend FORCEINLINE bool operator>(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        return !(Lhs <= Rhs);
    }

    /**
     * Greater than or equal comparison operator
     *
     * @param Lhs: Left side to compare with
     * @param Rhs: Right side to compare with
     * @return: Returns true if Lhs is greater than or equal to Rhs
     */
    friend FORCEINLINE bool operator>=(const TVariant& Lhs, const TVariant& Rhs) noexcept
    {
        return !(Lhs < Rhs);
    }

private:

    template<typename T, typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args) noexcept
    {
        new(Value.GetStorage()) T(Forward<ArgTypes>(Args)...);
    }

    FORCEINLINE void MoveFrom(TVariant& Other) noexcept
    {
        TVariantMoveConstructorTable::Move(Other.TypeIndex, Value.GetStorage(), Other.Value.GetStorage());
    }

    FORCEINLINE void CopyFrom(const TVariant& Other) noexcept
    {
        TVariantCopyConstructorTable::Copy(Other.TypeIndex, Value.GetStorage(), Other.Value.GetStorage());
    }

    FORCEINLINE void Destruct() noexcept
    {
        TVariantDestructorTable::Destruct(TypeIndex, Value.GetStorage());
    }

    FORCEINLINE bool IsEqual(const TVariant& Rhs) const noexcept
    {
        return TVariantComparatorsTable::IsEqual(TypeIndex, Value.GetStorage(), Rhs.Value.GetStorage());
    }

    FORCEINLINE bool IsLessThan(const TVariant& Rhs) const noexcept
    {
        return TVariantComparatorsTable::IsLessThan(TypeIndex, Value.GetStorage(), Rhs.Value.GetStorage());
    }

    /** Storage that fit the largest element */
    TAlignedStorage<SizeInBytes, AlignmentInBytes> Value;
    /** Storage that fit the largest element */
    TypeIndexType TypeIndex;
};