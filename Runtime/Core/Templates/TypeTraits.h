#pragma once
#include "Core/CoreDefines.h"
#include "Core/Templates/TypeTraits/AddTraits.h"
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/IdentityTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

#include <initializer_list>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Type Transformation

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// DeclVal and ExpandPacks helpers

template<typename T>
typename TAddRValueReference<T>::Type DeclVal() noexcept;

template<typename... Packs>
inline void ExpandPacks(Packs&&...) { }

///////////////////////////////////////////////////////////////////////////////////////////////////
// Type Traits

template<typename T>
struct TIsEmpty
{
    inline static constexpr bool Value = __is_empty(T);
};

template<typename T>
struct TIsEnum
{
    inline static constexpr bool Value = __is_enum(T);
};

template<typename T>
struct TIsUnion
{
    inline static constexpr bool Value = __is_union(T);
};

template<typename T>
struct TIsFinal
{
    inline static constexpr bool Value = __is_final(T);
};

template<typename T>
struct TIsPOD
{
    inline static constexpr bool Value = __is_pod(T);
};

template<typename T>
struct TIsPolymorphic
{
    inline static constexpr bool Value = __is_polymorphic(T);
};

template<typename T, typename F>
struct TIsAssignable
{
    inline static constexpr bool Value = __is_assignable(T, F);
};

template<typename BaseType, typename DerivedType>
struct TIsBaseOf
{
    inline static constexpr bool Value = __is_base_of(BaseType, DerivedType);
};

template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    inline static constexpr bool Value = __is_constructible(T, ArgTypes...);
};

template<typename F, typename T>
struct TIsConvertible
{
    inline static constexpr bool Value = __is_convertible_to(F, T);
};

template<typename F, typename T>
struct TIsPointerConvertible
{
    inline static constexpr bool Value = TIsConvertible<typename TAddPointer<F>::Type, typename TAddPointer<T>::Type>::Value;
};

template<typename T>
struct TIsTriviallyCopyable
{
    inline static constexpr bool Value = __is_trivially_copyable(T);
};

template<typename T, typename... ArgTypes>
struct TIsTriviallyConstructable
{
    inline static constexpr bool Value = __is_trivially_constructible(T, ArgTypes...);
};

template<typename T>
struct TIsTriviallyDestructable
{
    inline static constexpr bool Value = __is_trivially_destructible(T);
};

template<typename T>
struct TIsTrivial
{
    inline static constexpr bool Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value;
};

template<typename T, typename... Args>
struct TIsNothrowConstructible
{
    static constexpr bool Value = TIsConstructible<T, Args...>::Value && noexcept(T(DeclVal<Args>()...));
};

template<typename T, typename U>
struct TIsNothrowAssignable
{
    static constexpr bool Value = TIsAssignable<T, U>::Value && noexcept(DeclVal<T>() = DeclVal<U>());
};

template<typename T>
struct TUnderlyingType
{
    typedef typename TIdentity<__underlying_type(T)>::Type Type;
};

template<typename T>
struct TIsConst
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsConst<const T>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsVolatile
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsVolatile<volatile T>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsCopyConstructable
{
    inline static constexpr bool Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsCopyAssignable
{
    inline static constexpr bool Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsMoveConstructable
{
    inline static constexpr bool Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsMoveAssignable
{
    inline static constexpr bool Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowMoveConstructable
{
    inline static constexpr bool Value = TIsNothrowConstructible<T, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowMoveAssignable
{
    inline static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddRValueReference<T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyConstructable
{
    inline static constexpr bool Value = TIsNothrowConstructible<T, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsNothrowCopyAssignable
{
    inline static constexpr bool Value = TIsNothrowAssignable<typename TAddLValueReference<T>::Type, typename TAddLValueReference<const T>::Type>::Value;
};

template<typename T>
struct TIsPointer
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsPointer<T*>
{
    inline static constexpr bool Value = true;
};

template <typename T>
struct TIsMemberPointer
{
    inline static constexpr bool Value = false;
};

template <typename T, typename U>
struct TIsMemberPointer<T U::*>
{
    inline static constexpr bool Value = true;
};

template <typename T>
struct TIsMemberPointer<const T>
{
    inline static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<volatile T>
{
    inline static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template <typename T>
struct TIsMemberPointer<const volatile T>
{
    inline static constexpr bool Value = TIsMemberPointer<T>::Value;
};

template<typename T>
struct TIsNullable
{
    inline static constexpr bool Value = TOr<TIsPointer<T>, TIsMemberPointer<T>>::Value;
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
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsLValueReference<T&>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsRValueReference
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsRValueReference<T&&>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsReference
{
    inline static constexpr bool Value = TOr<TIsLValueReference<T>, TIsRValueReference<T>>::Value;
};

template<typename T, typename U>
struct TIsSame
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsSame<T, T>
{
    inline static constexpr bool Value = true;
};

template<typename T, typename U>
struct TIsNotSame
{
    inline static constexpr bool Value = TNot<TIsSame<T, U>>::Value;
};

template<typename T>
struct TIsNullptr
{
    inline static constexpr bool Value = TIsSame<nullptr_type, typename TRemoveCV<T>::Type>::Value;
};

template<typename T>
struct TIsVoid
{
    inline static constexpr bool Value = TIsSame<void, typename TRemoveCV<T>::Type>::Value;
};

template<typename T>
struct TIsArray
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsArray<T[]>
{
    inline static constexpr bool Value = true;
};

template<typename T, const int32 N>
struct TIsArray<T[N]>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsBoundedArray
{
    inline static constexpr bool Value = false;
};

template<typename T, const int32 N>
struct TIsBoundedArray<T[N]>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsUnboundedArray
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsUnboundedArray<T[]>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsInteger
{
    inline static constexpr bool Value = false;
};

template<>
struct TIsInteger<bool>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<CHAR>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<signed char>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned char>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<WIDECHAR>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<char16_t>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<char32_t>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<short>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned short>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<int>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned int>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<long>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned long>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<long long>
{
    inline static constexpr bool Value = true;
};

template<>
struct TIsInteger<unsigned long long>
{
    inline static constexpr bool Value = true;
};

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
    inline static constexpr bool Value = (TOr<TIsSame<float, typename TRemoveCV<T>::Type>, TIsSame<double, typename TRemoveCV<T>::Type>, TIsSame<long double, typename TRemoveCV<T>::Type>>::Value);
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
    struct TIsSignedImpl<U, false>
    {
        inline static constexpr bool Value = false;
    };

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
    struct TIsUnsignedImpl<U, false>
    {
        inline static constexpr bool Value = false;
    };

public:
    inline static constexpr bool Value = TIsUnsignedImpl<T>::Value;
};

// Checks if the type is a TArray-type(TArray, TArrayView, TStaticArray)
template<typename T>
struct TIsTArrayType
{
    inline static constexpr bool Value = false;
};

// Determine if this type is a string-type(TStaticString, TString, or TStringView)
template<typename T>
struct TIsTStringType
{
    inline static constexpr bool Value = false;
};

template<typename T>
struct TIsContiguousContainer
{
    inline static constexpr bool Value = TIsBoundedArray<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<T&&>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<volatile T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T> 
struct TIsContiguousContainer<const volatile T>
{
    inline static constexpr bool Value = TIsContiguousContainer<T>::Value;
};

template<typename T>
struct TIsContiguousContainer<std::initializer_list<T>>
{
    inline static constexpr bool Value = true;
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
    inline static constexpr bool Value = !TIsUnion<T>::Value && (sizeof(Test<T>(0)) == 1);
};

template<typename T>
struct TIsObject
{
    inline static constexpr bool Value = TOr<TIsScalar<T>, TIsArray<T>, TIsUnion<T>, TIsClass<T>>::Value;
};

#if PLATFORM_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable : 4180) //C4180 - A qualifier, such as const, is applied to a function type defined by typedef
#endif

template<typename T>
struct TIsFunction
{
    // NOTE: Functions and references cannot be const 
    inline static constexpr bool Value = !TIsConst<const T>::Value && !TIsReference<T>::Value;
};

#if PLATFORM_COMPILER_MSVC
    #pragma warning(pop)
#endif

template<typename T>
struct TIsFundamental
{
    inline static constexpr bool Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value;
};

template<typename T>
struct TIsCompound
{
    inline static constexpr bool Value = !TIsFundamental<T>::Value;
};

// Decays T into the value that can be passed to a function by non-const/volatile value
// - T[N] -> T*
// - const T& -> T
// - etc.

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
    inline static constexpr uint64 Value = alignof(T);
};

// Determine if the type can be reallocated using realloc, that is the type does not reference itself
// or have classes pointing directly to an element. This also means that objects can be moved using 
// memmove without issues.

#define MARK_AS_REALLOCATABLE(Type) \
    template<> \
    struct TIsReallocatable<Type> \
    { \
        inline static constexpr bool Value = true; \
    }

template<typename T>
struct TIsReallocatable
{
    inline static constexpr bool Value = TIsTrivial<T>::Value;
};
