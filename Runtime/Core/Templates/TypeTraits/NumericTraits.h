#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/EqualTraits.h"
#include "Core/Templates/TypeTraits/PointerTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

template<typename T>
struct TIsInteger : TFalseType { };

template<>
struct TIsInteger<bool> : TTrueType { };

template<>
struct TIsInteger<CHAR> : TTrueType { };

template<>
struct TIsInteger<WIDECHAR> : TTrueType { };

template<>
struct TIsInteger<char16_t> : TTrueType { };

template<>
struct TIsInteger<char32_t> : TTrueType { };

template<>
struct TIsInteger<long> : TTrueType { };

template<>
struct TIsInteger<unsigned long> : TTrueType { };

template<>
struct TIsInteger<int8> : TTrueType { };

template<>
struct TIsInteger<uint8> : TTrueType { };

template<>
struct TIsInteger<int16> : TTrueType { };

template<>
struct TIsInteger<uint16> : TTrueType { };

template<>
struct TIsInteger<int32> : TTrueType { };

template<>
struct TIsInteger<uint32> : TTrueType { };

template<>
struct TIsInteger<int64> : TTrueType { };

template<>
struct TIsInteger<uint64> : TTrueType { };

template <typename T>
struct TIsInteger<const T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<volatile T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<const volatile T>
{
    static constexpr bool Value = TIsInteger<T>::Value;
};

template<typename T>
struct TIsIntegerNotBool
{
    static constexpr bool Value = TAnd<TIsInteger<T>, TIsNotSame<T, bool>>::Value;
};

template<typename T>
struct TIsFloatingPoint
{
    static constexpr bool Value = TOr<TIsSame<float, typename TRemoveCV<T>::Type>, TIsSame<double, typename TRemoveCV<T>::Type>, TIsSame<long double, typename TRemoveCV<T>::Type>>::Value;
};

template <typename T>
struct TIsFloatingPoint<const T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<volatile T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<const volatile T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template<typename T>
struct TIsArithmetic
{
    static constexpr bool Value = TOr<TIsInteger<T>, TIsFloatingPoint<T>>::Value;
};

template<typename T>
struct TIsScalar
{
    static constexpr bool Value = TOr<TIsArithmetic<T>, TIsEnum<T>, TIsPointer<T>, TIsMemberPointer<T>, TIsNullptr<T>>::Value;
};

template<typename T>
struct TIsSigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsSignedImpl
    {
        static constexpr bool Value = static_cast<U>(-1) < static_cast<U>(0);
    };

    template<typename U>
    struct TIsSignedImpl<U, false> : TFalseType { };

public:
    static constexpr bool Value = TIsSignedImpl<T>::Value;
};

template<typename T>
struct TIsUnsigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsUnsignedImpl
    {
        static constexpr bool Value = static_cast<U>(0) < static_cast<U>(-1);
    };

    template<typename U>
    struct TIsUnsignedImpl<U, false> : TFalseType { };

public:
    static constexpr bool Value = TIsUnsignedImpl<T>::Value;
};

template<typename T>
struct TMakeSigned;

template<>
struct TMakeSigned<bool>
{
    // NOTE: Invalid type for TMakeSigned
};

template<>
struct TMakeSigned<signed char>
{
    typedef signed char Type;
};

template<>
struct TMakeSigned<unsigned char>
{
    typedef signed char Type;
};

template<>
struct TMakeSigned<signed short>
{
    typedef signed short Type;
};

template<>
struct TMakeSigned<unsigned short>
{
    typedef signed short Type;
};

template<>
struct TMakeSigned<signed int>
{
    typedef signed int Type;
};

template<>
struct TMakeSigned<unsigned int>
{
    typedef signed int Type;
};

template<>
struct TMakeSigned<signed long>
{
    typedef signed long Type;
};

template<>
struct TMakeSigned<unsigned long>
{
    typedef signed long Type;
};

template<>
struct TMakeSigned<signed long long>
{
    typedef signed long long Type;
};

template<>
struct TMakeSigned<unsigned long long>
{
    typedef signed long long Type;
};

template<typename T>
struct TMakeSigned
{
private:
    template<TSIZE Size>
    struct TMakeSignedFromSize
    {
        // NOTE: Invalid type for TMakeSigned
    };

    template<>
    struct TMakeSignedFromSize<sizeof(signed char)>
    {
        typedef signed char Type;
    };

    template<>
    struct TMakeSignedFromSize<sizeof(signed short)>
    {
        typedef signed short Type;
    };

    template<>
    struct TMakeSignedFromSize<sizeof(signed int)>
    {
        typedef signed int Type;
    };

    template<>
    struct TMakeSignedFromSize<sizeof(signed long long)>
    {
        typedef signed long long Type;
    };

    template<typename U, bool bIsValid = TOr<TIsEnum<U>, TIsInteger<U>>::Value>
    struct TMakeSignedImpl 
    {
        typedef TMakeSignedFromSize<sizeof(U)>::Type Type;
    };

    template<typename U>
    struct TMakeSignedImpl<U, false> 
    {
        // NOTE: Invalid type for TMakeSigned
    };

public:
    typedef typename TMakeSignedImpl<T>::Type Type;
};

// Handle CV-qualified types
template<typename T>
struct TMakeSigned<const T> 
{
    typedef typename TAddConst<typename TMakeSigned<T>::Type>::Type Type;
};

template<typename T>
struct TMakeSigned<volatile T> 
{
    typedef typename TAddVolatile<typename TMakeSigned<T>::Type>::Type Type;
};

template<typename T>
struct TMakeSigned<const volatile T> 
{
    typedef typename TAddCV<typename TMakeSigned<T>::Type>::Type Type;
};

template<typename T>
struct TMakeUnsigned;

template<>
struct TMakeUnsigned<bool>
{
    // NOTE: Invalid type for TMakeUnsigned
};

template<>
struct TMakeUnsigned<signed char>
{
    typedef unsigned char Type;
};

template<>
struct TMakeUnsigned<unsigned char>
{
    typedef unsigned char Type;
};

template<>
struct TMakeUnsigned<signed short>
{
    typedef unsigned short Type;
};

template<>
struct TMakeUnsigned<unsigned short>
{
    typedef unsigned short Type;
};

template<>
struct TMakeUnsigned<signed int>
{
    typedef unsigned int Type;
};

template<>
struct TMakeUnsigned<unsigned int>
{
    typedef unsigned int Type;
};

template<>
struct TMakeUnsigned<signed long>
{
    typedef unsigned long Type;
};

template<>
struct TMakeUnsigned<unsigned long>
{
    typedef unsigned long Type;
};

template<>
struct TMakeUnsigned<signed long long>
{
    typedef unsigned long long Type;
};

template<>
struct TMakeUnsigned<unsigned long long>
{
    typedef unsigned long long Type;
};

template<typename T>
struct TMakeUnsigned
{
private:
    template<TSIZE Size>
    struct TMakeUnsignedFromSize
    {
        // NOTE: Invalid type for TMakeUnsigned
    };

    template<>
    struct TMakeUnsignedFromSize<sizeof(unsigned char)>
    {
        typedef unsigned char Type;
    };

    template<>
    struct TMakeUnsignedFromSize<sizeof(unsigned short)>
    {
        typedef unsigned short Type;
    };

    template<>
    struct TMakeUnsignedFromSize<sizeof(unsigned int)>
    {
        typedef unsigned int Type;
    };

    template<>
    struct TMakeUnsignedFromSize<sizeof(unsigned long long)>
    {
        typedef unsigned long long Type;
    };

    template<typename U, bool bIsValid = TOr<TIsEnum<U>, TIsInteger<U>>::Value>
    struct TMakeUnsignedImpl 
    {
        typedef TMakeUnsignedFromSize<sizeof(U)>::Type Type;
    };

    template<typename U>
    struct TMakeUnsignedImpl<U, false> 
    {
        // NOTE: Invalid type for TMakeUnsigned
    };

public:
    typedef typename TMakeUnsignedImpl<T>::Type Type;
};

// Handle CV-qualified types
template<typename T>
struct TMakeUnsigned<const T> 
{
    typedef typename TAddConst<typename TMakeUnsigned<T>::Type>::Type Type;
};

template<typename T>
struct TMakeUnsigned<volatile T> 
{
    typedef typename TAddVolatile<typename TMakeUnsigned<T>::Type>::Type Type;
};

template<typename T>
struct TMakeUnsigned<const volatile T> 
{
    typedef typename TAddCV<typename TMakeUnsigned<T>::Type>::Type Type;
};
