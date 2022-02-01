#pragma once
#include "Core/Templates/AlignedStorage.h"
#include "Core/Templates/MinMax.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TVariant - Similar to std::variant

template<typename... Types>
class TVariant
{
    using IndexType = int32;

    enum
    {
        SizeInBytes      = TMax<sizeof(Types)...>::Value,
        AlignmentInBytes = TMax<alignof(Types)...>::Value,
        InvalidTypeIndex = -1
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TVariantIndex - Retrieve the Type index

    template <IndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper
    {
        enum
        {
            Value = -1
        };
    };

    template <IndexType CurrentIndex, typename WantedType, typename ElementType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, ElementType, OtherTypes...> 
    {
        enum
        {
            Value = TVariantIndexHelper<CurrentIndex + 1, WantedType, OtherTypes...>::Value
        };
    };

    template <IndexType CurrentIndex, typename WantedType, typename... OtherTypes>
    struct TVariantIndexHelper<CurrentIndex, WantedType, WantedType, OtherTypes...>
    {
        enum
        {
            Value = CurrentIndex
        };
    };

    template <typename WantedType>
    struct TVariantIndex
    {
        enum
        {
            Value = TVariantIndexHelper<0, WantedType, Types...>::Value
        };
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
        static void Destruct(IndexType Index, void* Memory)
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
            typedef T TypeConstructor;
            reinterpret_cast<TypeConstructor*>(Memory)->TypeConstructor::TypeConstructor(*reinterpret_cast<T*>(Value));
        }
    };

    struct TVariantCopyConstructorTable
    {
        static void Copy(IndexType Index, void* Memory, const void* Value)
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
            typedef T TypeConstructor;
            reinterpret_cast<TypeConstructor*>(Memory)->TypeConstructor::TypeConstructor(::Move(*reinterpret_cast<T*>(Value)));
        }
    };

    struct TVariantMoveConstructorTable
    {
        static void Move(IndexType Index, void* Memory, void* Value)
        {
            static constexpr void(*Table[])(void*, void*) = { &TVariantMoveConstructor<Types>::Move... };

            Assert(Index < ArrayCount(Table));
            Table[Index](Memory, Value);
        }
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

    template<typename T>
    FORCEINLINE TVariant(const T& InValue) noexcept
        : Value()
        , TypeIndex(TVariantIndex<T>::Value)
    {
    }

    FORCEINLINE TVariant(const TVariant& Other) noexcept
        : Value()
        , TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            TVariantCopyConstructorTable::Copy(Other.TypeIndex, Value.GetStorage(), Other.Value.GetStorage());
        }
    }

    FORCEINLINE TVariant(TVariant&& Other) noexcept
        : Value()
        , TypeIndex(Other.TypeIndex)
    {
        if (Other.IsValid())
        {
            TVariantMoveConstructorTable::Move(Other.TypeIndex, Value.GetStorage(), Other.Value.GetStorage());
            Other.Reset();
        }
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TVariant()
    {
        Reset();
    }

    template<typename T, typename... ArgTypes>
    FORCEINLINE T& Emplace(ArgTypes&&... Args)
    {
        Reset();

        new(Value.GetStorage()) T(Forward<ArgTypes>(Args)...);

        TypeIndex = TVariantIndex<T>::Value;

        return *Value.CastStorage<T>();
    }

    FORCEINLINE void Reset() noexcept
    {
        if (IsValid())
        {
            TVariantDestructorTable::Destruct(TypeIndex, Value.GetStorage());
            TypeIndex = InvalidTypeIndex;
        }
    }

    template<typename T>
    FORCEINLINE bool IsType() const noexcept
    {
        const IndexType WantedIndex = TVariantIndex<T>::Value;
        return (TypeIndex == WantedIndex);
    }

    template<typename T>
    FORCEINLINE T& GetValue() noexcept
    {
        return *Value.CastStorage<T>();
    }

    template<typename T>
    FORCEINLINE const T& GetValue() const noexcept
    {
        return *Value.CastStorage<T>();
    }

    FORCEINLINE IndexType GetIndex() const noexcept
    {
        return TypeIndex;
    }

    FORCEINLINE bool IsValid() const noexcept
    {
        return (TypeIndex != InvalidTypeIndex);
    }

private:

    /** Storage that fit the largest element */
    TAlignedStorage<SizeInBytes, AlignmentInBytes> Value;
    /** Storage that fit the largest element */
    IndexType TypeIndex;
};