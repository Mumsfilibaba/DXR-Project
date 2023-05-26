#pragma once
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"

template<typename... Types>
class TVariant
{
    using TypeIndexType = int32;

    inline static constexpr TypeIndexType SizeInBytes      = TMax<sizeof(Types)...>::Value;
    inline static constexpr TypeIndexType AlignmentInBytes = TMax<alignof(Types)...>::Value;
    inline static constexpr TypeIndexType InvalidTypeIndex = -1;
    inline static constexpr TypeIndexType MaxTypeIndex     = sizeof... (Types);

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

    template<typename WantedType>
    struct TVariantIndex
    {
        inline static constexpr TypeIndexType Value = TVariantIndexHelper<0, WantedType, Types...>::Value;
    };


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


    template<typename T>
    struct TDestructor
    {
        static void Destruct(void* Memory) noexcept
        {
            typedef T TypeDestructor;
            reinterpret_cast<TypeDestructor*>(Memory)->TypeDestructor::~TypeDestructor();
        }
    };

    struct FDestructorTable
    {
        static void Destruct(TypeIndexType Index, void* Memory) noexcept
        {
            static constexpr void(*Table[])(void*) = { &TDestructor<Types>::Destruct... };

            CHECK(Index < ARRAY_COUNT(Table));
            Table[Index](Memory);
        }
    };


    template<typename T>
    struct TCopyConstructor
    {
        static void Copy(void* Memory, const void* Value) noexcept
        {
            new(Memory) T(*reinterpret_cast<const T*>(Value));
        }
    };

    struct FCopyConstructorTable
    {
        static void Copy(TypeIndexType Index, void* Memory, const void* Value) noexcept
        {
            static constexpr void(*Table[])(void*, void*) = { &TCopyConstructor<Types>::Copy... };

            CHECK(Index < ARRAY_COUNT(Table));
            Table[Index](Memory, Value);
        }
    };


    template<typename T>
    struct TMoveConstructor
    {
        static void Move(void* Memory, void* Value) noexcept
        {
            new(Memory) T(::Move(*reinterpret_cast<T*>(Value)));
        }
    };

    struct FMoveConstructorTable
    {
        static void Move(TypeIndexType Index, void* Memory, void* Value) noexcept
        {
            static constexpr void(*Table[])(void*, void*) = { &TMoveConstructor<Types>::Move... };

            CHECK(Index < ARRAY_COUNT(Table));
            Table[Index](Memory, Value);
        }
    };


    template<typename T>
    struct TCompareFuncs
    {
        NODISCARD static bool IsEqual(const void* LHS, const void* RHS) noexcept
        {
            return (*reinterpret_cast<const T*>(LHS)) == (*reinterpret_cast<const T*>(RHS));
        }

        NODISCARD static bool IsLessThan(const void* LHS, const void* RHS) noexcept
        {
            return (*reinterpret_cast<const T*>(LHS)) < (*reinterpret_cast<const T*>(RHS));
        }
    };

    struct FCompareFuncTable
    {
        NODISCARD static bool IsEqual(TypeIndexType Index, const void* LHS, const void* RHS) noexcept
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TCompareFuncs<Types>::IsEqual... };

            CHECK(Index < ARRAY_COUNT(Table));
            return Table[Index](LHS, RHS);
        }

        NODISCARD static bool IsLessThan(TypeIndexType Index, const void* LHS, const void* RHS) noexcept
        {
            static constexpr bool(*Table[])(const void*, const void*) = { &TCompareFuncs<Types>::IsLessThan... };

            CHECK(Index < ARRAY_COUNT(Table));
            return Table[Index](LHS, RHS);
        }
    };


    template<typename T>
    struct TIsValidType
    {
        inline static constexpr bool Value = (TVariantIndex<T>::Value != InvalidTypeIndex) ? true : false;
    };


    template<TypeIndexType Index>
    struct TIsValidIndex
	{
        inline static constexpr bool Value = (Index < MaxTypeIndex) ? true : false;
    };

public:
    
    /**
     * @brief - Default constructor
     */
    FORCEINLINE TVariant() noexcept
        : Value()
        , TypeIndex(InvalidTypeIndex)
    {
    }

    /**
     * @brief      - In-Place constructor that constructs a variant of specified type with arguments for the types constructor
     * @param Args - Arguments for the elements constructor
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceType<T>, ArgTypes&&... Args)
    {
        static_assert(TIsValidType<T>::Value, "This TVariant is not specified to hold the specified type");
        Construct<T>(::Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief      - In-Place constructor that constructs a variant of specified type with arguments for the types constructor
     * @param Args - Arguments for the elements constructor
     */
    template<TypeIndexType InIndex, typename... ArgTypes>
    FORCEINLINE explicit TVariant(TInPlaceIndex<InIndex>, ArgTypes&&... Args)
    {
        static_assert(TIsValidIndex<InIndex>::Value, "Specified index is not valid with this TVariant");
        typedef typename TVariantType<InIndex>::Type ConstructType;
        Construct<ConstructType>(::Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief       - Copy constructor
     * @param Other - Variant to copy from
     */
    FORCEINLINE TVariant(const TVariant& Other) noexcept
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
    FORCEINLINE TVariant(TVariant&& Other) noexcept
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
     * @brief      - Create a value in-place 
     * @param Args - Arguments for the constructor of the element
     * @return     - Returns a reference to the newly created element
     */
    template<typename T, typename... ArgTypes>
    FORCEINLINE T& Emplace(ArgTypes&&... Args) noexcept requires(TIsValidType<T>::Value)
    {
        Reset();
        Construct<T>(::Forward<ArgTypes>(Args)...);
        return *reinterpret_cast<T*>(Value.Data);
    }

    /**
     * @brief - Resets the variant and calls the destructor
     */
    FORCEINLINE void Reset() noexcept
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
    FORCEINLINE void Swap(TVariant& Other) noexcept
    {
        TVariant TempVariant(::Move(*this));
        
        TypeIndex = Other.TypeIndex;
        if (Other.TypeIndex != InvalidTypeIndex)
        {
            MoveFrom(Other);
            Other.Destruct();
        }

        Other.TypeIndex = TempVariant.TypeIndex;
        if (TempVariant.TypeIndex != InvalidTypeIndex)
        {
            Other.MoveFrom(TempVariant);
        }
    }

    /**
     * @brief  - Check if the templated type is the current type
     * @return - Returns true if the templated type is the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE bool IsType() const noexcept requires(TIsValidType<T>::Value)
    {
        return TypeIndex == TVariantIndex<T>::Value;
    }

    /**
     * @brief  - Retrieve the currently held value
     * @return - Returns a reference to the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE T& GetValue() noexcept requires(TIsValidType<T>::Value)
    {
        CHECK(IsValid() && IsType<T>());
        return *reinterpret_cast<T*>(Value.Data);
    }

    /**
     * @brief  - Retrieve the currently held value
     * @return - Returns a reference to the currently held value
     */
    template<typename T>
    NODISCARD FORCEINLINE const T& GetValue() const noexcept requires(TIsValidType<T>::Value)
    {
        CHECK(IsValid() && IsType<T>());
        return *reinterpret_cast<const T*>(Value.Data);
    }

    /**
     * @brief  - Try and retrieve the currently held value, or get nullptr if value of specified type is not held
     * @return - Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    NODISCARD FORCEINLINE T* TryGetValue() noexcept requires(TIsValidType<T>::Value)
    {
        return IsType<T>() ? reinterpret_cast<T*>(Value.Data) : nullptr;
    }

    /**
     * @brief  - Try and retrieve the currently held value, or get nullptr if value of specified type is not held
     * @return - Returns a pointer to the currently stored value or nullptr if not correct type
     */
    template<typename T>
    NODISCARD FORCEINLINE const T* TryGetValue() const noexcept requires(TIsValidType<T>::Value)
    {
        return IsType<T>() ? reinterpret_cast<const T*>(Value.Data) : nullptr;
    }

    /**
     * @brief  - Retrieve the type index of the currently held value
     * @return - Returns the index of the current held value
     */
    NODISCARD FORCEINLINE TypeIndexType GetIndex() const noexcept
    {
        return TypeIndex;
    }

    /**
     * @brief  - Check if the Variant is valid or not
     * @return - Returns true if the variant holds a value
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return TypeIndex != InvalidTypeIndex;
    }

public:

    /**
     * @brief       - Copy assignment operator
     * @param Other - Variant to copy from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(const TVariant& Other) noexcept
    {
        TVariant(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move assignment operator
     * @param Other - Variant to move from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TVariant& operator=(TVariant&& Other) noexcept
    {
        TVariant(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief     - Comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the variants are equal
     */
    NODISCARD friend FORCEINLINE bool operator==(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        if (LHS.TypeIndex != RHS.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!LHS.IsValid())
        {
            return true;
        }

        return LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Comparison operator
     * @param LHS - Left side to compare with 
     * @param RHS - Right side to compare with
     * @return    - Returns false if the variants are equal
     */
    NODISCARD friend FORCEINLINE bool operator!=(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    /**
     * @brief     - Less than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is less than RHS
     */
    NODISCARD friend FORCEINLINE bool operator<(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        if (LHS.TypeIndex != RHS.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!LHS.IsValid())
        {
            return true;
        }

        return LHS.IsLessThan(RHS);
    }

    /**
     * @brief     - Less than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is less than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator<=(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        if (LHS.TypeIndex != RHS.TypeIndex)
        {
            return false;
        }

        // Both indices are equal at this point
        if (!LHS.IsValid())
        {
            return true;
        }

        return LHS.IsLessThan(RHS) || LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Greater than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is greater than RHS
     */
    NODISCARD friend FORCEINLINE bool operator>(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        return !(LHS <= RHS);
    }

    /**
     * @brief     - Greater than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is greater than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator>=(const TVariant& LHS, const TVariant& RHS) noexcept
    {
        return !(LHS < RHS);
    }

private:

    template<typename T, typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args) noexcept
    {
        new(reinterpret_cast<void*>(Value.Data)) T(::Forward<ArgTypes>(Args)...);
        TypeIndex = TVariantIndex<T>::Value;
    }

    FORCEINLINE void MoveFrom(TVariant& Other) noexcept
    {
        FMoveConstructorTable::Move(Other.TypeIndex, reinterpret_cast<void*>(Value.Data), reinterpret_cast<void*>(Other.Value.Data));
    }

    FORCEINLINE void CopyFrom(const TVariant& Other) noexcept
    {
        FCopyConstructorTable::Copy(Other.TypeIndex, reinterpret_cast<void*>(Value.Data), reinterpret_cast<const void*>(Other.Value.Data));
    }

    FORCEINLINE void Destruct() noexcept
    {
        FDestructorTable::Destruct(TypeIndex, reinterpret_cast<void*>(Value.Data));
        TypeIndex = InvalidTypeIndex;
    }

    NODISCARD FORCEINLINE bool IsEqual(const TVariant& RHS) const noexcept
    {
        return FCompareFuncTable::IsEqual(TypeIndex, reinterpret_cast<const void*>(Value.Data), reinterpret_cast<const void*>(RHS.Value.Data));
    }

    NODISCARD FORCEINLINE bool IsLessThan(const TVariant& RHS) const noexcept
    {
        return FCompareFuncTable::IsLessThan(TypeIndex, reinterpret_cast<const void*>(Value.Data), reinterpret_cast<const void*>(RHS.Value.Data));
    }

    // Storage that fit the largest element
    TAlignedBytes<SizeInBytes, AlignmentInBytes> Value;
    TypeIndexType TypeIndex;
};