#pragma once
#include "TypeTraits.h"
#include "IntegerSequence.h"

#define ENUM_CLASS_OPERATORS(EnumType) \
    constexpr EnumType operator|(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) | ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator|=(EnumType& LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) |= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr EnumType operator&(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) & ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator&=(EnumType& LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) &= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr EnumType operator~(EnumType LHS) noexcept \
    { \
        return EnumType(~((TUnderlyingType<EnumType>::Type)LHS)); \
    } \
    constexpr EnumType operator^(EnumType LHS, EnumType RHS) noexcept \
    { \
        return EnumType(((TUnderlyingType<EnumType>::Type)LHS) ^ ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    inline EnumType& operator^=(EnumType&LHS, EnumType RHS) noexcept \
    { \
        return (EnumType&)(((TUnderlyingType<EnumType>::Type&)LHS) ^= ((TUnderlyingType<EnumType>::Type)RHS)); \
    } \
    constexpr bool IsEnumFlagSet(EnumType EnumMask, EnumType EnumFlag) noexcept \
    { \
        return ToUnderlying((EnumMask) & (EnumFlag)) != 0; \
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
using TTypeAlignedBytes = TAlignedBytes<sizeof(T), TAlignmentOf<T>::Value>;


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


template<typename NewType, typename ClassType>
constexpr NewType* GetAsBytes(ClassType* Class) noexcept
{
    return reinterpret_cast<NewType*>(Class);
}


// Move an object by converting it into a r-value
template<typename T>
constexpr typename TRemoveReference<T>::Type&& Move(T&& Value) noexcept
{
    return static_cast<typename TRemoveReference<T>::Type&&>(Value);
}


// Forward an object by converting it into a r-value from an l-value
template<typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type& Value) noexcept
{
    return static_cast<T&&>(Value);
}

// Forward an object by converting it into a r-value from an r-value
template<typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type&& Value) noexcept
{
    return static_cast<T&&>(Value);
}


// Swap Elements
template<typename T>
constexpr void Swap(T& LHS, T& RHS) noexcept requires(TNot<TIsConst<T>>::Value)
{
    T TempElement = Move(LHS);
    LHS = Move(RHS);
    RHS = Move(TempElement);
}


template<typename EnumType>
constexpr typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}


template<typename T>
constexpr uintptr ToInteger(T Pointer) requires(TIsPointer<T>::Value)
{
    return reinterpret_cast<uintptr>(Pointer);
}


template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TIsObject<T>::Value)
{
    return reinterpret_cast<T*>(&const_cast<CHAR&>(reinterpret_cast<const volatile CHAR&>(Object)));
}

template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TNot<TIsObject<T>>::Value)
{
    return &Object;
}


template<typename T>
typename TAddReference<T>::RValue DeclVal() noexcept;


template<typename... Packs>
inline void ExpandPacks(Packs&&...) { }


template<typename T, uint32 NumElements>
constexpr bool CompareArrays(const T(&LHS)[NumElements], const T(&RHS)[NumElements])
{
    for (int32 Index = 0; Index < NumElements; Index++)
    {
        if (LHS[Index] != RHS[Index])
        {
            return false;
        }
    }

    return true;
}

template<typename T>
inline bool CompareArrays(const T* LHS, const T* RHS, int32 NumElements)
{
    for (int32 Index = 0; Index < NumElements; Index++)
    {
        if (LHS[Index] != RHS[Index])
        {
            return false;
        }
    }

    return true;
}
