#pragma once
#include "Core/Templates/TypeTraits/ArrayTraits.h"
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/BasicTraits.h"
#include "Core/Templates/TypeTraits/EqualTraits.h"
#include "Core/Templates/TypeTraits/NumericTraits.h"
#include "Core/Templates/TypeTraits/PointerTraits.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"

template<typename T>
struct TIsVoid
{
    inline static constexpr bool Value = TIsSame<void, typename TRemoveCV<T>::Type>::Value;
};

template<typename T>
struct TIsClass
{
private:
    template<typename U>
    static int8 Test(int U::*);

    template<typename U>
    static int16 Test(...);

public:
    inline static constexpr bool Value = TNot<TIsUnion<T>>::Value && (sizeof(Test<T>(0)) == 1);
};

template<typename T>
struct TIsObject
{
    inline static constexpr bool Value = TOr<TIsScalar<T>, TIsArray<T>, TIsUnion<T>, TIsClass<T>>::Value;
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
    inline static constexpr bool Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value;
};

template<typename T>
struct TIsCompound
{
    inline static constexpr bool Value = TNot<TIsFundamental<T>>::Value;
};

template<typename T>
struct TIsTrivial
{
    inline static constexpr bool Value = TAnd<TIsTriviallyConstructable<T>, TIsTriviallyCopyable<T>, TIsTriviallyDestructable<T>>::Value;
};

template<typename T, typename = void>
struct TIsCompleteType : TFalseType { };

template<typename T>
struct TIsCompleteType<T, decltype(void(sizeof(T)))> : TTrueType { };
