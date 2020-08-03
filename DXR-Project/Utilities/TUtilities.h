#pragma once

// Removes reference retrives the type
template<typename T>
struct _TRemoveReference
{
	using TType = T;
	using TConstRefType = const T;
};

template<typename T>
struct _TRemoveReference<T&>
{
	using TType = T;
	using TConstRefType = const T&;
};

template<typename T>
struct _TRemoveReference<T&&>
{
	using TType = T;
	using TConstRefType = const T&&;
};

template<typename T>
using TRemoveReference = typename _TRemoveReference<T>::TType;

// Removes pointer and retrives the type
template<typename T>
struct _TRemovePointer
{
	using TType = T;
};

template<typename T>
struct _TRemovePointer<T*>
{
	using TType = T;
};

template<typename T>
struct _TRemovePointer<T* const>
{
	using TType = T;
};

template<typename T>
struct _TRemovePointer<T* volatile>
{
	using TType = T;
};

template<typename T>
struct _TRemovePointer<T* const volatile>
{
	using TType = T;
};

template<typename T>
using TRemovePointer = typename _TRemovePointer<T>::TType;

// Move an object by converting it into a rvalue
template<typename T>
constexpr TRemoveReference<T>&& Move(T&& Object) noexcept
{
	return static_cast<TRemoveReference<T>&&>(Object);
}