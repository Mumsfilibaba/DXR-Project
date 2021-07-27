#pragma once

/* Determine if the type is a function */
template<typename T>
struct TIsFunction
{
    enum { Value = false };
};

/* Standard function types */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )>
{
    enum { Value = true };
};

/* Variadic function types */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )>
{
    enum { Value = true };
};

/* Functions types having const/volatile qualifiers */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile>
{
    enum { Value = true };
};

/* Function types with ref */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... )&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... )&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&&>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&&>
{
    enum { Value = true };
};

/* Function types with noexcept */
template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) & noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) & noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) && noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const&& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) volatile&& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes... ) const volatile&& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) && noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const&& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) volatile&& noexcept>
{
    enum { Value = true };
};

template<typename ReturnType, typename... ArgTypes>
struct TIsFunction<ReturnType( ArgTypes...... ) const volatile&& noexcept>
{
    enum { Value = true };
};