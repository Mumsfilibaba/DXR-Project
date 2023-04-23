#pragma once
#include "TypeTraits.h"

#define ENUM_CLASS_OPERATORS(EnumType)                                                                         \
    CONSTEXPR EnumType operator|(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) | ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
                                                                                                               \
    inline EnumType& operator|=(EnumType& LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) |= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
                                                                                                               \
    CONSTEXPR EnumType operator&(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) & ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
                                                                                                               \
    inline EnumType& operator&=(EnumType& LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) &= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
                                                                                                               \
    CONSTEXPR EnumType operator~(EnumType LHS) noexcept                                                        \
    {                                                                                                          \
        return EnumType(~((TUnderlyingType<EnumType>::Type)LHS));                                              \
    }                                                                                                          \
                                                                                                               \
    CONSTEXPR EnumType operator^(EnumType LHS, EnumType RHS) noexcept                                          \
    {                                                                                                          \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) ^ ((TUnderlyingType<EnumType>::Type)RHS));      \
    }                                                                                                          \
                                                                                                               \
    inline EnumType& operator^=(EnumType&LHS, EnumType RHS) noexcept                                           \
    {                                                                                                          \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) ^= ((TUnderlyingType<EnumType>::Type)RHS)); \
    }                                                                                                          \
                                                                                                               \
    CONSTEXPR bool IsEnumFlagSet(EnumType EnumMask, EnumType EnumFlag) noexcept                                \
    {                                                                                                          \
        return (ToUnderlying((EnumMask) & (EnumFlag)) != 0);                                                   \
    }


enum EInPlace
{
    InPlace = 0
};

template<typename T>
struct TInPlaceType
{
    explicit TInPlaceType() = default;
};

template<int32 Index>
struct TInPlaceIndex
{
    explicit TInPlaceIndex() = default;
};


template<typename ReturnType, typename... ArgTypes>
struct TCallableWrapper
{
    typedef ReturnType(*Type)(ArgTypes...);
};

template<typename ReturnType>
struct TCallableWrapper<ReturnType, void>
{
    typedef ReturnType(*Type)(void);
};


template<typename FunctionType>
struct TFunctionType;

template<typename ReturnType, typename... ArgTypes>
struct TFunctionType<ReturnType(ArgTypes...)>
{
    typedef ReturnType(*Type)(ArgTypes...);
};


template<bool IsConst, typename ClassType, typename FunctionType>
struct TMemberFunctionType;

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<false, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...);
};

template<typename ClassType, typename ReturnType, typename... ArgTypes>
struct TMemberFunctionType<true, ClassType, ReturnType(ArgTypes...)>
{
    typedef ReturnType(ClassType::* Type)(ArgTypes...) const;
};


template<int32 InSize, int32 InAlignment>
struct TAlignedBytes
{
    ALIGN_AS(InAlignment) uint8 Data[InSize];
};

template<typename T>
using TTypeAlignedBytes = TAlignedBytes<sizeof(T), AlignmentOf<T>>;


struct FNonCopyable
{
    FNonCopyable()  = default;
    ~FNonCopyable() = default;

    FNonCopyable(const FNonCopyable&)            = delete;
    FNonCopyable& operator=(const FNonCopyable&) = delete;
};

struct FNonMovable
{
    FNonMovable()  = default;
    ~FNonMovable() = default;

    FNonMovable(const FNonMovable&)            = delete;
    FNonMovable& operator=(const FNonMovable&) = delete;
};

struct FNonCopyAndNonMovable
{
    FNonCopyAndNonMovable()  = default;
    ~FNonCopyAndNonMovable() = default;

    FNonCopyAndNonMovable(const FNonCopyAndNonMovable&)            = delete;
    FNonCopyAndNonMovable& operator=(const FNonCopyAndNonMovable&) = delete;

    FNonCopyAndNonMovable(FNonCopyAndNonMovable&&)            = delete;
    FNonCopyAndNonMovable& operator=(FNonCopyAndNonMovable&&) = delete;
};


template <typename T, T... Sequence>
struct TIntegerSequence
{
    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    typedef T Type;
 
    enum { Size = sizeof...(Sequence) };
};


namespace Internal
{
    template <typename T, unsigned N>
    struct TMakeIntegerSequenceImpl;
}

template<typename T, T N>
using TMakeIntegerSequence = typename Internal::TMakeIntegerSequenceImpl<T, N>::Type;

namespace Internal
{
    template<uint32 N, typename FirstSequence, typename SecondSequence>
    struct TSequenceHelper;

    template<uint32 N, typename T, T... First, T... Second>
    struct TSequenceHelper<N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> 
        : TIntegerSequence<T, First..., (T(N + Second))...>
    {
        using Type = TIntegerSequence<T, First..., (T(N + Second))...>;
    };
    

    template<uint32 N, typename FirstSequence, typename SecondSequence>
    using TSequenceHelperType = typename TSequenceHelper<N, FirstSequence, SecondSequence>::Type;


    template<typename T, uint32 N>
    struct TMakeIntegerSequenceImpl 
        : TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 1> 
        : TIntegerSequence<T, T(0)>
    {
        using Type = TIntegerSequence<T, T(0)>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 0> 
        : TIntegerSequence<T>
    {
        using Type = TIntegerSequence<T>;
    };
}


template<typename NewType, typename ClassType>
CONSTEXPR NewType* GetAsBytes(ClassType* Class) noexcept
{
    return reinterpret_cast<NewType*>(Class);
}


// Move an object by converting it into a r-value
template<typename T>
CONSTEXPR typename TRemoveReference<T>::Type&& Move(T&& Value) noexcept
{
    return static_cast<typename TRemoveReference<T>::Type&&>(Value);
}


// Forward an object by converting it into a r-value from an l-value
template<typename T>
CONSTEXPR T&& Forward(typename TRemoveReference<T>::Type& Value) noexcept
{
    return static_cast<T&&>(Value);
}

// Forward an object by converting it into a r-value from an r-value
template<typename T>
CONSTEXPR T&& Forward(typename TRemoveReference<T>::Type&& Value) noexcept
{
    return static_cast<T&&>(Value);
}


// Swap Elements
template<typename T>
FORCEINLINE typename TEnableIf<TNot<TIsConst<T>>::Value>::Type Swap(T& LHS, T& RHS) noexcept
{
    T TempElement = Move(LHS);
    LHS = Move(RHS);
    RHS = Move(TempElement);
}


template<typename EnumType>
CONSTEXPR typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}


template<typename T>
CONSTEXPR typename TEnableIf<TIsPointer<T>::Value, uintptr>::Type ToInteger(T Pointer)
{
    return reinterpret_cast<uintptr>(Pointer);
}


template<typename T>
FORCEINLINE typename TEnableIf<TIsObject<T>::Value, T*>::Type AddressOf(T& Object) noexcept
{
    return reinterpret_cast<T*>(&const_cast<CHAR&>(reinterpret_cast<const volatile CHAR&>(Object)));
}

template<typename T>
FORCEINLINE typename TEnableIf<!TIsObject<T>::Value, T*>::Type AddressOf(T& Object) noexcept
{
    return &Object;
}


template<typename T>
typename TAddReference<T>::RValue DeclVal() noexcept;


template<typename... Packs>
inline void ExpandPacks(Packs&&...) { }


template<typename EnumType>
CONSTEXPR typename TEnableIf<TIsEnum<EnumType>::Value, EnumType>::Type EnumAdd(EnumType Value, typename TUnderlyingType<EnumType>::Type Offset) noexcept
{
    return static_cast<EnumType>(ToUnderlying(Value) + Offset);
}

template<typename EnumType>
CONSTEXPR typename TEnableIf<TIsEnum<EnumType>::Value, EnumType>::Type EnumSub(EnumType Value, typename TUnderlyingType<EnumType>::Type Offset) noexcept
{
    return static_cast<EnumType>(ToUnderlying(Value) - Offset);
}
