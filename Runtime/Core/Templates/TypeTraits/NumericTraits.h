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
struct TIsInteger<signed char> : TTrueType { };

template<>
struct TIsInteger<unsigned char> : TTrueType { };

template<>
struct TIsInteger<WIDECHAR> : TTrueType { };

template<>
struct TIsInteger<char16_t> : TTrueType { };

template<>
struct TIsInteger<char32_t> : TTrueType { };

template<>
struct TIsInteger<short> : TTrueType { };

template<>
struct TIsInteger<unsigned short> : TTrueType { };

template<>
struct TIsInteger<int> : TTrueType { };

template<>
struct TIsInteger<unsigned int> : TTrueType { };

template<>
struct TIsInteger<long> : TTrueType { };

template<>
struct TIsInteger<unsigned long> : TTrueType { };

template<>
struct TIsInteger<long long> : TTrueType { };

template<>
struct TIsInteger<unsigned long long> : TTrueType { };

template <typename T>
struct TIsInteger<const T>
{
    inline static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<volatile T>
{
    inline static constexpr bool Value = TIsInteger<T>::Value;
};

template <typename T>
struct TIsInteger<const volatile T>
{
    inline static constexpr bool Value = TIsInteger<T>::Value;
};

template<typename T>
struct TIsFloatingPoint
{
    inline static constexpr bool Value = TOr<TIsSame<float, typename TRemoveCV<T>::Type>, TIsSame<double, typename TRemoveCV<T>::Type>, TIsSame<long double, typename TRemoveCV<T>::Type>>::Value;
};

template <typename T>
struct TIsFloatingPoint<const T>
{
    inline static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<volatile T>
{
    inline static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<const volatile T>
{
    inline static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template<typename T>
struct TIsIntegerNotBool
{
    inline static constexpr bool Value = TAnd<TIsInteger<T>, TIsNotSame<T, bool>>::Value;
};

template<typename T>
struct TIsArithmetic
{
    inline static constexpr bool Value = TOr<TIsInteger<T>, TIsFloatingPoint<T>>::Value;
};

template<typename T>
struct TIsScalar
{
    inline static constexpr bool Value = TOr<TIsArithmetic<T>, TIsEnum<T>, TIsPointer<T>, TIsMemberPointer<T>, TIsNullptr<T>>::Value;
};

template<typename T>
struct TIsSigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsSignedImpl
    {
        inline static constexpr bool Value = static_cast<U>(-1) < static_cast<U>(0);
    };

    template<typename U>
    struct TIsSignedImpl<U, false> : TFalseType { };

public:
    inline static constexpr bool Value = TIsSignedImpl<T>::Value;
};

template<typename T>
struct TIsUnsigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsUnsignedImpl
    {
        inline static constexpr bool Value = static_cast<U>(0) < static_cast<U>(-1);
    };

    template<typename U>
    struct TIsUnsignedImpl<U, false> : TFalseType { };

public:
    inline static constexpr bool Value = TIsUnsignedImpl<T>::Value;
};
