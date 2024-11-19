#pragma once
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"

template<typename... Types>
class TVariant
{
    typedef int32 TypeIndexType;

    // Compile-time constants
    inline static constexpr TypeIndexType SizeInBytes      = TMax<sizeof(Types)...>::Value;
    inline static constexpr TypeIndexType AlignmentInBytes = TMax<alignof(Types)...>::Value;
    inline static constexpr TypeIndexType InvalidTypeIndex = TypeIndexType(-1);
    inline static constexpr TypeIndexType MaxTypeIndex     = sizeof...(Types);
    
    // Helper to find the index of a type
    template<TypeIndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper
    {
        inline static constexpr TypeIndexType Value = InvalidTypeIndex;
    };

    template<TypeIndexType CurrentIndex, typename WantedType, typename ElementType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, ElementType, OtherTypes...> 
    {
        inline static constexpr TypeIndexType Value = TVariantIndexHelper<CurrentIndex + 1, WantedType, OtherTypes...>::Value;
    };

    template<TypeIndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, WantedType, OtherTypes...>
    {
        inline static constexpr TypeIndexType Value = CurrentIndex;
    };

    // Struct to get the type index
    template<typename WantedType>
    struct TVariantIndex
    {
        inline static constexpr TypeIndexType Value = TVariantIndexHelper<0, WantedType, Types...>::Value;
    };

    // Helper to get type by index
    template<TypeIndexType CurrentIndex, TypeIndexType Index, typename... OtherTypes>
    struct TVariantTypeHelper;

    template<TypeIndexType CurrentIndex, TypeIndexType Index, typename ElementType, typename... OtherTypes>
    struct TVariantTypeHelper<CurrentIndex, Index, ElementType, OtherTypes...>
    {
        typedef typename TVariantTypeHelper<CurrentIndex + 1, Index, OtherTypes...>::Type Type;
    };

    template<TypeIndexType Index, typename ElementType, typename... OtherTypes>
    struct TVariantTypeHelper<Index, Index, ElementType, OtherTypes...>
    {
        typedef ElementType Type;
    };

    // Helper to get type by index
    template<TypeIndexType SearchForIndex>
    struct TVariantType
    {
        typedef typename TVariantTypeHelper<0, SearchForIndex, Types...>::Type Type;
    };

    template<typename T, typename... TTypes>
    struct IsIn;

    template<typename T>
    struct IsIn<T> : TFalseType { };

    template<typename T, typename U, typename... RestTypes>
    struct IsIn<T, U, RestTypes...> : TConditional<TIsSame<T, U>::Value, TTrueType, IsIn<T, RestTypes...>>::Type { };

    template<typename... TTypes>
    struct TAllUnique;

    template<>
    struct TAllUnique<> : TTrueType { };

    template<typename T, typename... RestTypes>
    struct TAllUnique<T, RestTypes...> : TConditional<TNot<IsIn<T, RestTypes...>>::Value, TAllUnique<RestTypes...>, TFalseType>::Type { };

    // Ensure that all types are unique
    static_assert(TAllUnique<Types...>::Value, "All types in TVariant must be unique.");

    // Destructor helper
    template<typename T>
    struct TDestructor
    {
        static void Destruct(void* Memory)
        {
            typedef T TypeDestructor;
            reinterpret_cast<TypeDestructor*>(Memory)->TypeDestructor::~TypeDestructor();
        }
    };

    struct FDestructorTable
    {
        static void Destruct(TypeIndexType Index, void* Memory)
        {
            static constexpr void(*Table[])(void*) = { &TDestructor<Types>::Destruct... };
            CHECK(Index < MaxTypeIndex);
            Table[Index](Memory);
        }
    };

    // Copy constructor helper
    template<typename T>
    struct TCopyConstructor
    {
        static void Copy(void* Memory, const void* Value)
        {
            new (Memory) T(*reinterpret_cast<const T*>(Value));
        }
    };

    struct FCopyConstructorTable
    {
        static void Copy(TypeIndexType Index, void* Memory, const void* Value)
        {
            static constexpr void(*Table[])(void*, const void*) = { &TCopyConstructor<Types>::Copy... };
            CHECK(Index < MaxTypeIndex);
            Table[Index](Memory, Value);
        }
    };

    // Move constructor helper
    template<typename T>
    struct TMoveConstructor
    {
        static void Move(void* Memory, void* Value)
        {
            new (Memory) T(Move(*reinterpret_cast<T*>(Value)));
        }
    };

    struct FMoveConstructorTable
    {
        static void Move(TypeIndexType Index, void* Memory, void* Value)
        {
            static constexpr void(*Table[])(void*, void*) = { &TMoveConstructor<Types>::Move... };
            CHECK(Index < MaxTypeIndex);
            Table[Index](Memory, Value);
        }
    };

    // Comparison functions helper
    template<typename T>
    struct TCompareFuncs
    {
        NODISCARD static bool IsEqual(const void* LHS, const void* RHS)
        {
            return (*reinterpret_cast<const T*>(LHS)) == (*reinterpret_cast<const T*>(RHS));
        }

        NODISCARD static bool IsLessThan(const void* LHS, const void* RHS)
        {
            return (*reinterpret_cast<const T*>(LHS)) < (*reinterpret_cast<const T*>(RHS));
        }
    };

    struct FCompareFuncTable
    {
        NODISCARD static bool IsEqual(TypeIndexType Index, const void* LHS, const void* RHS)
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TCompareFuncs<Types>::IsEqual... };
            CHECK(Index < MaxTypeIndex);
            return Table[Index](LHS, RHS);
        }

        NODISCARD static bool IsLessThan(TypeIndexType Index, const void* LHS, const void* RHS)
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TCompareFuncs<Types>::IsLessThan... };
            CHECK(Index < MaxTypeIndex);
            return Table[Index](LHS, RHS);
        }
    };

    // Validate type
    template<typename T>
    struct TIsValidType
    {
        inline static constexpr bool Value = TVariantIndex<T>::Value != InvalidTypeIndex;
    };

    // Validate index
    template<TypeIndexType Index>
    struct TIsValidIndex
    {
        inline static constexpr bool Value = Index < MaxTypeIndex;
    };

public:

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TVariant()
        : TypeIndex(InvalidTypeIndex)
    {
    }

    /**
     * @brief      - In-place constructor that constructs a variant of specified type with arguments for the type's constructor
     * @param Args - Arguments for the type's constructor
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceType<T>, ArgTypes&&... Args)
        requires(TIsValidType<T>::Value)
    {
        Construct<T>(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief      - In-place constructor that constructs a variant of specified type with arguments for the type's constructor
     * @param Args - Arguments for the type's constructor
     */
    template<TypeIndexType InIndex, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceIndex<InIndex>, ArgTypes&&... Args)
        requires(TIsValidIndex<InIndex>::Value)
    {
        typedef typename TVariantType<InIndex>::Type ConstructType;
        Construct<ConstructType>(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief       - Copy constructor
     * @param Other - Variant to copy from
     */
    FORCEINLINE TVariant(const TVariant& Other)
        : TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            CopyFrom(Other);
        }
    }

    /**
     * @brief       - Move constructor
     * @param Other - Variant to move from
     */
    FORCEINLINE TVariant(TVariant&& Other)
        : TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            MoveFrom(Other);
            Other.Destruct();
        }
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TVariant()
    {
        Reset();
    }

    /**
     * @brief      - Emplace constructs a new element in the variant and destructs any old value
     * @param Args - Arguments for constructor
     * @return     - Returns a reference to the newly constructed element
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE T& Emplace(ArgTypes&&... Args) requires(TIsValidType<T>::Value)
    {
        Reset();
        Construct<T>(Forward<ArgTypes>(Args)...);
        return *reinterpret_cast<T*>(Value.Data);
    }

    /**
     * @brief - Resets the variant and calls the destructor
     */
    FORCEINLINE void Reset()
    {
        if (IsValid())
        {
            Destruct();
        }
    }

    /**
     * @brief       - Swap this variant with another
     * @param Other - Variant to swap with
     */
    FORCEINLINE void Swap(TVariant& Other)
    {
        if (IsValid() && Other.IsValid())
        {
            if (TypeIndex == Other.TypeIndex)
            {
                // If both hold the same type, swap the contained objects
                typedef typename TVariantType<TypeIndex>::Type SwappedType;
                ::Swap(GetValue<SwappedType>(), Other.GetValue<SwappedType>());
            }
            else
            {
                // Swap the values out
                TAlignedBytes<SizeInBytes, AlignmentInBytes> TempValue;
                FMoveConstructorTable::Move(TypeIndex, TempValue.Data, Value.Data);
                FMoveConstructorTable::Move(Other.TypeIndex, Value.Data, Other.Value.Data);
                FMoveConstructorTable::Move(TypeIndex, Other.Value.Data, TempValue.Data);

                // Then swap the type index
                ::Swap(TypeIndex, Other.TypeIndex);
            }
        }
        else if (IsValid())
        {
            Other.Construct(Move(*this));
            Destruct();
        }
        else if (Other.IsValid())
        {
            Construct(Move(Other));
            Other.Destruct();
        }
    }

    /**
     * @brief  - Check if the templated type is the current type
     * @return - Returns true if the templated type is the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE bool IsType() const requires(TIsValidType<T>::Value)
    {
        return TypeIndex == TVariantIndex<T>::Value;
    }

    /**
     * @brief  - Retrieve the currently held value
     * @return - Returns a reference to the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE T& GetValue() requires(TIsValidType<T>::Value)
    {
        CHECK(IsValid() && IsType<T>());
        return *reinterpret_cast<T*>(Value.Data);
    }

    /**
     * @brief  - Retrieve the currently held value
     * @return - Returns a reference to the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE const T& GetValue() const requires(TIsValidType<T>::Value)
    {
        CHECK(IsValid() && IsType<T>());
        return *reinterpret_cast<const T*>(Value.Data);
    }

    /**
     * @brief  - Try to retrieve the currently held value, or get nullptr if value of specified type is not held
     * @return - Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    NODISCARD FORCEINLINE T* TryGetValue() requires(TIsValidType<T>::Value)
    {
        return IsType<T>() ? reinterpret_cast<T*>(Value.Data) : nullptr;
    }

    /**
     * @brief  - Try to retrieve the currently held value, or get nullptr if value of specified type is not held
     * @return - Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    NODISCARD FORCEINLINE const T* TryGetValue() const requires(TIsValidType<T>::Value)
    {
        return IsType<T>() ? reinterpret_cast<const T*>(Value.Data) : nullptr;
    }

    /**
     * @brief  - Retrieve the type index of the currently held value
     * @return - Returns the index of the current held value
     */
    NODISCARD FORCEINLINE TypeIndexType GetIndex() const
    {
        return TypeIndex;
    }

    /**
     * @brief  - Check if the variant is valid or not
     * @return - Returns true if the variant holds a value
     */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return TypeIndex != InvalidTypeIndex;
    }

    /**
     * @brief         - Retrieve the contained value or a default
     * @param Default - Default value to return if a value of the specified type is not held
     * @return        - Returns the stored value or the default
     */
    template<typename T, typename DefaultType>
    NODISCARD FORCEINLINE const T& GetValueOrDefault(const DefaultType& Default) const requires(TIsValidType<T>::Value)
    {
        if (IsType<T>())
        {
            return GetValue<T>();
        }

        return static_cast<const T&>(Default);
    }

    /**
     * @brief         - Retrieve the contained value or a default
     * @param Default - Default value to return if a value of the specified type is not held
     * @return        - Returns the stored value or the default
     */
    template<typename T, typename DefaultType>
    NODISCARD FORCEINLINE T GetValueOrDefault(DefaultType&& Default) const requires(TIsValidType<T>::Value)
    {
        if (IsType<T>())
        {
            return GetValue<T>();
        }
        
        return static_cast<T>(Forward<DefaultType>(Default));
    }

public:

    /**
     * @brief       - Copy assignment operator
     * @param Other - Variant to copy from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(const TVariant& Other)
    {
        TVariant(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move assignment operator
     * @param Other - Variant to move from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(TVariant&& Other)
    {
        TVariant(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator from a contained type
     * @param Value - Value to assign to the variant
     * @return      - Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TVariant& operator=(OtherType&& Value) requires(TIsValidType<typename TDecay<OtherType>::Type>::Value)
    {
        typedef typename TDecay<OtherType>::Type DecayedType;
        
        if (IsType<DecayedType>())
        {
            GetValue<DecayedType>() = Forward<OtherType>(Value);
        }
        else
        {
            Reset();
            Construct<DecayedType>(Forward<OtherType>(Value));
        }
        
        return *this;
    }

    /**
     * @brief     - Comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the variants are equal
     */
    NODISCARD friend FORCEINLINE bool operator==(const TVariant& LHS, const TVariant& RHS)
    {
        if (LHS.TypeIndex != RHS.TypeIndex)
        {
            return false;
        }

        if (!LHS.IsValid() && !RHS.IsValid())
        {
            return true; // Both are invalid
        }

        if (!LHS.IsValid() || !RHS.IsValid())
        {
            return false;
        }

        return LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Comparison operator
     * @param LHS - Left side to compare with 
     * @param RHS - Right side to compare with
     * @return    - Returns false if the variants are equal
     */
    NODISCARD friend FORCEINLINE bool operator!=(const TVariant& LHS, const TVariant& RHS)
    {
        return !(LHS == RHS);
    }

    /**
     * @brief     - Less than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is less than RHS
     */
    NODISCARD friend FORCEINLINE bool operator<(const TVariant& LHS, const TVariant& RHS)
    {
        if (LHS.TypeIndex < RHS.TypeIndex)
        {
            return true;
        }

        if (LHS.TypeIndex > RHS.TypeIndex)
        {
            return false;
        }

        if (!LHS.IsValid() && !RHS.IsValid())
        {
            return false; // Both are invalid, neither is less
        }

        if (!LHS.IsValid())
        {
            return true; // Invalid is considered less than valid
        }

        if (!RHS.IsValid())
        {
            return false;
        }

        return LHS.IsLessThan(RHS);
    }

    /**
     * @brief     - Less than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is less than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator<=(const TVariant& LHS, const TVariant& RHS)
    {
        if (LHS.TypeIndex < RHS.TypeIndex)
        {
            return true;
        }

        if (LHS.TypeIndex > RHS.TypeIndex)
        {
            return false;
        }

        if (!LHS.IsValid() && !RHS.IsValid())
        {
            return true;
        }

        if (!LHS.IsValid())
        {
            return true; // Invalid is considered less than valid
        }

        if (!RHS.IsValid())
        {
            return false;
        }

        return LHS.IsLessThan(RHS) || LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Greater than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is greater than RHS
     */
    NODISCARD friend FORCEINLINE bool operator>(const TVariant& LHS, const TVariant& RHS)
    {
        return !(LHS <= RHS);
    }

    /**
     * @brief     - Greater than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is greater than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator>=(const TVariant& LHS, const TVariant& RHS)
    {
        return !(LHS < RHS);
    }

private:

    template<typename T, typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args)
    {
        new(reinterpret_cast<void*>(Value.Data)) T(Forward<ArgTypes>(Args)...);
        TypeIndex = TVariantIndex<T>::Value;
    }

    FORCEINLINE void Destruct()
    {
        FDestructorTable::Destruct(TypeIndex, reinterpret_cast<void*>(Value.Data));
        TypeIndex = InvalidTypeIndex;
    }

    FORCEINLINE void MoveFrom(TVariant& Other)
    {
        FMoveConstructorTable::Move(Other.TypeIndex, reinterpret_cast<void*>(Value.Data), reinterpret_cast<void*>(Other.Value.Data));
    }

    FORCEINLINE void CopyFrom(const TVariant& Other)
    {
        FCopyConstructorTable::Copy(Other.TypeIndex, reinterpret_cast<void*>(Value.Data), reinterpret_cast<const void*>(Other.Value.Data));
    }

    NODISCARD FORCEINLINE bool IsEqual(const TVariant& RHS) const
    {
        return FCompareFuncTable::IsEqual(TypeIndex, reinterpret_cast<const void*>(Value.Data), reinterpret_cast<const void*>(RHS.Value.Data));
    }

    NODISCARD FORCEINLINE bool IsLessThan(const TVariant& RHS) const
    {
        return FCompareFuncTable::IsLessThan(TypeIndex, reinterpret_cast<const void*>(Value.Data), reinterpret_cast<const void*>(RHS.Value.Data));
    }

    // Storage that fits the largest element with proper alignment
    TAlignedBytes<SizeInBytes, AlignmentInBytes> Value;
    TypeIndexType TypeIndex{ InvalidTypeIndex };
};
