#pragma once
#include "Core/CoreTypes.h"
#include "Core/CoreDefines.h"

#include <initializer_list>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Types

using nullptr_type = decltype(nullptr);
using void_type    = void;

template <typename T>
struct TVoid
{
    typedef void_type Type;
};

template<typename T>
struct TIdentity
{
    typedef T Type;
};

template<bool bInValue>
struct TValue
{
    enum { Value = bInValue };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Type Transformation

template<typename T>
struct TNot
{
    enum { Value = !T::Value };
};


template<typename... ArgsType>
struct TAnd;

template<typename T, typename... ArgsType>
struct TAnd<T, ArgsType...>
{
    enum { Value = (T::Value && TAnd<ArgsType...>::Value) };
};

template<typename T>
struct TAnd<T>
{
    enum { Value = T::Value };
};


template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    enum { Value = (T::Value || TOr<ArgsType...>::Value) };
};

template<typename T>
struct TOr<T>
{
    enum { Value = T::Value };
};


template<int32 Arg0, int32... RestArgs>
struct TMin;

template<int32 Arg0>
struct TMin<Arg0>
{
    enum { Value = Arg0 };
};

template<int32 Arg0, int32 Arg1, int32... RestArgs>
struct TMin<Arg0, Arg1, RestArgs...>
{
    enum { Value = (Arg0 <= Arg1) ? TMin<Arg0, RestArgs...>::Value : TMin<Arg1, RestArgs...>::Value };
};


template<int32 Arg0, int32... RestArgs>
struct TMax;

template<int32 Arg0>
struct TMax<Arg0>
{
    enum { Value = Arg0 };
};

template<int32 Arg0, int32 Arg1, int32... RestArgs>
struct TMax<Arg0, Arg1, RestArgs...>
{
    enum { Value = (Arg0 >= Arg1) ? TMax<Arg0, RestArgs...>::Value : TMax<Arg1, RestArgs...>::Value };
};


template<typename T>
struct TRemoveCV
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<const T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<volatile T>
{
    typedef T Type;
};

template<typename T>
struct TRemoveCV<const volatile T>
{
    typedef T Type;
};


template<typename T>
struct TRemoveConst
{
    typedef T Type;
};

template<typename T>
struct TRemoveConst<const T>
{
    typedef T Type;
};


template<typename T>
struct TRemoveVolatile
{
    typedef T Type;
};

template<typename T>
struct TRemoveVolatile<volatile T>
{
    typedef T Type;
};


template<typename T>
struct TRemoveExtent
{
    typedef T Type;
};

template<typename T>
struct TRemoveExtent<T[]>
{
    typedef T Type;
};

template<typename T, const int32 N>
struct TRemoveExtent<T[N]>
{
    typedef T Type;
};

// TODO: TRemoveAllExtents


template<typename T>
struct TRemovePointer
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T*>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* const>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* volatile>
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T* const volatile>
{
    typedef T Type;
};


template<typename T>
struct TRemoveReference
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&>
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&&>
{
    typedef T Type;
};


template<typename T>
struct TAddCV
{
    typedef const volatile T Type;
};

template<typename T>
struct TAddConst
{
    typedef const T Type;
};
 
template<typename T>
struct TAddVolatile
{
    typedef volatile T Type;
};


template<typename T>
struct TAddLValueReference
{
private:
    template <class U>
    static TIdentity<U&> TryAdd(int);

    template <class U>
    static TIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

template<typename T>
struct TAddRValueReference
{
private:
    template <class U>
    static TIdentity<U&&> TryAdd(int);

    template <class U>
    static TIdentity<U> TryAdd(...);

    typedef decltype(TryAdd<T>(0)) IdentityType;

public:
    typedef typename IdentityType::Type Type;
};

template<typename T>
struct TAddReference
{
    typedef typename TAddLValueReference<T>::Type LValue;
    typedef typename TAddRValueReference<T>::Type RValue;
};


template<typename T>
struct TAddPointer
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type*>::Type Type;
};


template<bool Condition, typename TrueType, typename FalseType>
struct TConditional
{
    typedef TrueType Type;
};

template<typename TrueType, typename FalseType>
struct TConditional<false, TrueType, FalseType>
{
    typedef FalseType Type;
};


template<bool Condition, typename T = void>
struct TEnableIf
{
};

template<typename T>
struct TEnableIf<true, T>
{
    typedef T Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Type Traits

template<typename T>
struct TIsEmpty
{
    enum { Value = __is_empty(T) };
};


template<typename T>
struct TIsEnum
{
    enum { Value = __is_enum(T) };
};


template<typename T>
struct TIsUnion
{
    enum { Value = __is_union(T) };
};


template<typename T>
struct TIsFinal
{
    enum { Value = __is_final(T) };
};


template<typename T>
struct TIsPOD
{
    enum { Value = __is_pod(T) };
};


template<typename T>
struct TIsPolymorphic
{
    enum { Value = __is_polymorphic(T) };
};


template<typename T, typename FromType>
struct TIsAssignable
{
    enum { Value = __is_assignable(T, FromType) };
};


template<typename BaseType, typename DerivedType>
struct TIsBaseOf
{
    enum { Value = __is_base_of(BaseType, DerivedType) };
};


template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    enum { Value = __is_constructible(T, ArgTypes...) };
};


template<typename FromType, typename ToType>
struct TIsConvertible
{
    enum { Value = __is_convertible_to(FromType, ToType) };
};

template<typename FromType, typename ToType>
struct TIsPointerConvertible
{
    enum { Value = TIsConvertible<TAddPointer<FromType>::Type, TAddPointer<ToType>::Type>::Value };
};


template<typename T>
struct TIsTriviallyCopyable
{
    enum { Value = __is_trivially_copyable(T) };
};

template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    enum { Value = __is_trivially_constructible(T, ArgTypes...) };
};

template<typename T>
struct TIsTriviallyDestructable
{
    enum { Value = __is_trivially_destructible(T) };
};

template<typename T>
struct TIsTrivial
{
    enum { Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value };
};


template<typename T>
struct TUnderlyingType
{
    typedef typename TIdentity<__underlying_type(T)>::Type Type;
};


template<typename T>
struct TIsConst
{
    enum { Value = false };
};

template<typename T>
struct TIsConst<const T>
{
    enum { Value = true };
};


template<typename T>
struct TIsVolatile
{
    enum { Value = false };
};

template<typename T>
struct TIsVolatile<volatile T>
{
    enum { Value = true };
};


template<typename T>
struct TIsCopyConstructable
{
    enum { Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value };
};

template<typename T>
struct TIsCopyAssignable
{
    enum { Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value };
};


template<typename T>
struct TIsMoveConstructable
{
    enum { Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value };
};

template<typename T>
struct TIsMoveAssignable
{
    enum { Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value };
};


template<typename T>
struct TIsPointer
{
    enum { Value = false };
};

template<typename T>
struct TIsPointer<T*>
{
    enum { Value = true };
};


template<typename T>
struct TMemberPointerTraits
{
};

template<typename T, typename U>
struct TMemberPointerTraits<U T::*>
{
    typedef U Type;
};


template<typename T>
struct TIsLValueReference
{
    enum { Value = false };
};

template<typename T>
struct TIsLValueReference<T&>
{
    enum { Value = true };
};

template<typename T>
struct TIsRValueReference
{
    enum { Value = false };
};

template<typename T>
struct TIsRValueReference<T&&>
{
    enum { Value = true };
};

template<typename T>
struct TIsReference
{
    enum { Value = TOr<TIsLValueReference<T>, TIsRValueReference<T>>::Value };
};


template<typename T, typename U>
struct TIsSame
{
    enum { Value = false };
};

template<typename T>
struct TIsSame<T, T>
{
    enum { Value = true };
};

template<typename T, typename U>
struct TIsNotSame
{
    enum { Value = TNot<TIsSame<T, U>>::Value };
};


template<typename T>
struct TIsNullptr
{
    enum { Value = TIsSame<nullptr_type, typename TRemoveCV<T>::Type>::Value };
};


template<typename T>
struct TIsVoid
{
    enum { Value = TIsSame<void, typename TRemoveCV<T>::Type>::Value };
};


template<typename T>
struct TIsArray
{
    enum { Value = false };
};

template<typename T>
struct TIsArray<T[]>
{
    enum { Value = true };
};

template<typename T, const int32 N>
struct TIsArray<T[N]>
{
    enum { Value = true };
};


template<typename T>
struct TIsBoundedArray
{
    enum { Value = false };
};

template<typename T, const int32 N>
struct TIsBoundedArray<T[N]>
{
    enum { Value = true };
};


template<typename T>
struct TIsUnboundedArray
{
    enum { Value = false };
};

template<typename T>
struct TIsUnboundedArray<T[]>
{
    enum { Value = true };
};


template<typename T>
struct TIsInteger
{
    enum { Value = false };
};

template<>
struct TIsInteger<bool>
{
    enum { Value = true };
};

template<>
struct TIsInteger<CHAR>
{
    enum { Value = true };
};

template<>
struct TIsInteger<signed char>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned char>
{
    enum { Value = true };
};

template<>
struct TIsInteger<WIDECHAR>
{
    enum { Value = true };
};

template<>
struct TIsInteger<char16_t>
{
    enum { Value = true };
};

template<>
struct TIsInteger<char32_t>
{
    enum { Value = true };
};

template<>
struct TIsInteger<short>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned short>
{
    enum { Value = true };
};

template<>
struct TIsInteger<int>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned int>
{
    enum { Value = true };
};

template<>
struct TIsInteger<long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<long long>
{
    enum { Value = true };
};

template<>
struct TIsInteger<unsigned long long>
{
    enum { Value = true };
};

template <typename T>
struct TIsInteger<const T>
{
    enum { Value = TIsInteger<T>::Value };
};

template <typename T>
struct TIsInteger<volatile T>
{
    enum { Value = TIsInteger<T>::Value };
};

template <typename T>
struct TIsInteger<const volatile T>
{
    enum { Value = TIsInteger<T>::Value };
};


template <typename T>
struct TIsMemberPointer
{
    enum { Value = false };
};

template <typename T, typename U>
struct TIsMemberPointer<T U::*>
{
    enum { Value = true };
};

template <typename T>
struct TIsMemberPointer<const T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};

template <typename T>
struct TIsMemberPointer<volatile T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};

template <typename T>
struct TIsMemberPointer<const volatile T>
{
    enum { Value = TIsMemberPointer<T>::Value };
};


template<typename T>
struct TIsFloatingPoint
{
    enum
    {
        Value = (TOr<
                     TIsSame<float      , typename TRemoveCV<T>::Type>,
                     TIsSame<double     , typename TRemoveCV<T>::Type>, 
                     TIsSame<long double, typename TRemoveCV<T>::Type>
                    >::Value)
    };
};

template <typename T>
struct TIsFloatingPoint<const T>
{
    enum { Value = TIsFloatingPoint<T>::Value };
};

template <typename T>
struct TIsFloatingPoint<volatile T>
{
    enum { Value = TIsFloatingPoint<T>::Value };
};

template <typename T>
struct TIsFloatingPoint<const volatile T>
{
    enum { Value = TIsFloatingPoint<T>::Value };
};


template<typename T>
struct TIsIntegerNotBool
{
    enum { Value = TAnd<TIsInteger<T>, TIsNotSame<T, bool>>::Value };
};


template<typename T>
struct TIsArithmetic
{
    enum { Value = TOr<TIsInteger<T>, TIsFloatingPoint<T>>::Value };
};


template<typename T>
struct TIsScalar
{
    enum { Value = TOr<TIsArithmetic<T>, TIsEnum<T>, TIsPointer<T>, TIsMemberPointer<T>, TIsNullptr<T>>::Value };
};


template<typename T>
struct TIsSigned
{
private:

    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsSignedImpl
    {
        enum { Value = U(-1) < U(0) };
    };

    template<typename U>
    struct TIsSignedImpl<U, false>
    {
        enum { Value = false };
    };

public:
    enum { Value = TIsSignedImpl<T>::Value };
};

template<typename T>
struct TIsUnsigned
{
private:

    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsUnsignedImpl
    {
        enum { Value = U(0) < U(-1) };
    };

    template<typename U>
    struct TIsUnsignedImpl<U, false>
    {
        enum { Value = false };
    };

public:
    enum { Value = TIsUnsignedImpl<T>::Value };
};


template<typename T>
struct TIsContiguousContainer
{
    enum { Value = TIsBoundedArray<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<T&>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<T&&>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<const T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<volatile T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T> 
struct TIsContiguousContainer<const volatile T>
{
    enum { Value = TIsContiguousContainer<T>::Value };
};

template<typename T>
struct TIsContiguousContainer<std::initializer_list<T>>
{
    enum { Value = true };
};


template<typename T>
struct TIsClass
{
private:
    
    // Has to be declared before usage
    template<typename U>
    static int8 Test(int U::*);

    template<typename U>
    static int16 Test(...);

public:
    enum { Value = (!(TIsUnion<T>::Value) && (sizeof(Test<T>(0)) == 1)) };
};


template<typename T>
struct TIsObject
{
    enum { Value = TOr<TIsScalar<T>, TIsArray<T>, TIsUnion<T>, TIsClass<T>>::Value };
};


#if PLATFORM_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4180)
#endif

template<typename T>
struct TIsFunction
{
    // NOTE: Functions and references cannot be const 
    enum { Value = (!TIsConst<const T>::Value) && (!TIsReference<T>::Value) };
};

#if PLATFORM_COMPILER_MSVC
    #pragma warning(pop)
#endif


template<typename T>
struct TIsFundamental
{
    enum { Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value };
};

template<typename T>
struct TIsCompound
{
    enum { Value = !TIsFundamental<T>::Value };
};


/**
 *  Decays T into the value that can be passed to a function by non-const/volatile value
 *  - T[N] -> T*
 *  - const T& -> T
 *  - etc.
 */

template<typename T>
struct TDecay
{
private:
    typedef typename TRemoveReference<T>::Type U;
    typedef typename TRemoveExtent<U>::Type* TrueType;
    typedef typename TConditional<TIsFunction<U>::Value, typename TAddPointer<U>::Type, typename TRemoveCV<U>::Type>::Type FalseType;

public:
    typedef typename TConditional<TIsArray<U>::Value, TrueType, FalseType>::Type Type;
};


template<typename T>
struct TAlignmentOf
{
    enum { Value = alignof(T) };
};

template<typename T>
inline CONSTEXPR int32 AlignmentOf = TAlignmentOf<T>::Value;

/**
 * Determine if the type can be reallocated using realloc, that is the type does
 * not reference itself or have classes pointing directly to an element. This
 * also means that objects can be memmove:ed without issues.
 */

#define MARK_AS_REALLOCATABLE(Type) \
    template<>                      \
    struct TIsReallocatable<Type>   \
    {                               \
        enum { Value = true };      \
    }

template<typename T>
struct TIsReallocatable
{
    enum { Value = TIsTrivial<T>::Value };
};


// Checks if the type is a TArray-type(TArray, TArrayView, TStaticArray)
template<typename T>
struct TIsTArrayType
{
    enum { Value = false };
};


// Determine if this type is a string-type(TStaticString, TString, or TStringView)
template<typename T>
struct TIsTStringType
{
    enum { Value = false };
};
