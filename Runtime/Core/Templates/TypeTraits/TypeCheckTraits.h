#pragma once
#include "Core/Templates/TypeTraits/ArrayTraits.h"
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/EqualTraits.h"
#include "Core/Templates/TypeTraits/MetaProgrammingFuncs.h"
#include "Core/Templates/TypeTraits/NumericTraits.h"
#include "Core/Templates/TypeTraits/PointerTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

template<typename T>
struct TIsVoid
{
    static constexpr bool Value = TIsSame<void, typename TRemoveCV<T>::Type>::Value;
};

template<typename T>
struct TIsClass
{
private:
    template<typename U>
    static TIntegralConstant<bool, TNot<TIsUnion<T>>::Value> IsClassFunc(int U::*);

    template<typename U>
    static TFalseType IsClassFunc(...);

    template<typename U>
    struct TIsClassImpl : decltype(IsClassFunc<U>(nullptr)) { };

public:
    static constexpr bool Value = TIsClassImpl<T>::Value;
};

template<typename T>
struct TIsObject
{
    static constexpr bool Value = TOr<TIsScalar<T>, TIsArray<T>, TIsUnion<T>, TIsClass<T>>::Value;
};

// Primary template: defaults to false
template<typename T>
struct TIsFunction : TFalseType { };

// Function types without cv-qualifiers or ref-qualifiers
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...)> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args..., ...)> : TTrueType { };

// Function types with cv-qualifiers
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile> : TTrueType { };

// Function types with ref-qualifiers
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) &> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) &&> : TTrueType { };

// Function types with both cv-qualifiers and ref-qualifiers
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const &> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile &> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile &> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const &&> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile &&> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile &&> : TTrueType { };

// Function types with noexcept specifier (since C++17)
#if __cplusplus >= 201703L

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args..., ...) noexcept> : TTrueType { };

// cv-qualifiers with noexcept
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile noexcept> : TTrueType { };

// ref-qualifiers with noexcept
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) & noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) && noexcept> : TTrueType { };

// cv- and ref-qualifiers with noexcept
template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const & noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile & noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile & noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const && noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) volatile && noexcept> : TTrueType { };

template<typename Ret, typename... Args>
struct TIsFunction<Ret(Args...) const volatile && noexcept> : TTrueType { };

#endif

template<typename T>
struct TIsFundamental
{
    static constexpr bool Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value;
};

template<typename T>
struct TIsCompound
{
    static constexpr bool Value = TNot<TIsFundamental<T>>::Value;
};

template<typename T>
struct TIsTrivial
{
    static constexpr bool Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value;
};

template<typename T, typename = void>
struct TIsCompleteType : TFalseType { };

template<typename T>
struct TIsCompleteType<T, decltype(void(sizeof(T)))> : TTrueType { };

template<typename T>
struct TIsScopedEnum
{
    static constexpr bool Value = TAnd<TIsEnum<T>, TNot<TIsConvertible<T, TUnderlyingType<T>::Type>>>::Value;
};
