#pragma once

/* Determine if the type is a function */
template<typename T>
struct TIsFunction
{
    static constexpr bool Value = false;
};

/* Standard function types */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )>
{
    static constexpr bool Value = true;
};

/* Variadic function types */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )>
{
    static constexpr bool Value = true;
};

/* Functions types having const/volatile qualifiers */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile>
{
    static constexpr bool Value = true;
};

/* Function types with ref */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&&>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&&>
{
    static constexpr bool Value = true;
};

/* Function types with noexcept */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) & noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) & noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) && noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) && noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&& noexcept>
{
    static constexpr bool Value = true;
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&& noexcept>
{
    static constexpr bool Value = true;
};