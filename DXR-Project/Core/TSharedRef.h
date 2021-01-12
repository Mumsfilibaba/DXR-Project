#pragma once
#include "Core/RefCountedObject.h"

#include <type_traits>

/*
* TSharedRef - Helper class when using objects with RefCountedObject as a base
*/

template<typename T>
class TSharedRef
{
public:
	template<typename TOther>
	friend class TSharedRef;

	FORCEINLINE TSharedRef() noexcept
		: RefPtr(nullptr)
	{
	}

	FORCEINLINE TSharedRef(const TSharedRef& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		AddRef();
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(const TSharedRef<TOther>& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_convertible<TOther*, T*>());
		AddRef();
	}

	FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		Other.RefPtr = nullptr;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TSharedRef<TOther>&& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_convertible<TOther*, T*>());
		Other.RefPtr = nullptr;
	}

	FORCEINLINE TSharedRef(T* InPtr) noexcept
		: RefPtr(InPtr)
	{
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TOther* InPtr) noexcept
		: RefPtr(InPtr)
	{
		static_assert(std::is_convertible<TOther*, T*>());
	}

	FORCEINLINE ~TSharedRef()
	{
		Release();
	}

	FORCEINLINE T* Reset() noexcept
	{
		T* WeakPtr = RefPtr;
		Release();

		return WeakPtr;
	}

	FORCEINLINE T* ReleaseOwnership() noexcept
	{
		T* WeakPtr = RefPtr;
		RefPtr = nullptr;
		return WeakPtr;
	}

	FORCEINLINE void AddRef() noexcept
	{
		if (RefPtr)
		{
			RefPtr->AddRef();
		}
	}

	FORCEINLINE void Swap(T* InPtr) noexcept
	{
		Release();
		RefPtr = InPtr;
	}

	template<typename TOther>
	FORCEINLINE void Swap(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());

		Release();
		RefPtr = InPtr;
	}

	FORCEINLINE T* Get() const noexcept
	{
		return RefPtr;
	}

	FORCEINLINE T* GetRefCount() const noexcept
	{
		return RefPtr->GetRefCount();
	}

	FORCEINLINE T* GetAndAddRef() noexcept
	{
		AddRef();
		return RefPtr;
	}

	template<typename TCastType>
	FORCEINLINE TEnableIf<std::is_convertible_v<TCastType*, T*>, TCastType*> GetAs() const noexcept
	{
		return static_cast<TCastType*>(RefPtr);
	}

	FORCEINLINE T* const* GetAddressOf() const noexcept
	{
		return &RefPtr;
	}

	FORCEINLINE T& Dereference() const noexcept
	{
		return *RefPtr;
	}

	FORCEINLINE T* operator->() const noexcept
	{
		return Get();
	}

	FORCEINLINE T* const* operator&() const noexcept
	{
		return GetAddressOf();
	}

	FORCEINLINE T& operator*() const noexcept
	{
		return Dereference();
	}

	FORCEINLINE bool operator==(T* InPtr) const noexcept
	{
		return (RefPtr == InPtr);
	}

	FORCEINLINE bool operator==(const TSharedRef& Other) const noexcept
	{
		return (RefPtr == Other.RefPtr);
	}

	FORCEINLINE bool operator!=(T* InPtr) const noexcept
	{
		return (RefPtr != InPtr);
	}

	FORCEINLINE bool operator!=(const TSharedRef& Other) const noexcept
	{
		return (RefPtr != Other.RefPtr);
	}

	FORCEINLINE bool operator==(std::nullptr_t) const noexcept
	{
		return (RefPtr == nullptr);
	}

	FORCEINLINE bool operator!=(std::nullptr_t) const noexcept
	{
		return (RefPtr != nullptr);
	}

	template<typename TOther>
	FORCEINLINE bool operator==(TOther* InPtr) const noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());
		return (RefPtr == InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator==(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());
		return (RefPtr == Other.RefPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(TOther* InPtr) const noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());
		return (RefPtr != InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());
		return (RefPtr != Other.RefPtr);
	}

	FORCEINLINE operator bool() const noexcept
	{
		return (RefPtr != nullptr);
	}

	FORCEINLINE TSharedRef& operator=(const TSharedRef& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Release();

			RefPtr = Other.RefPtr;
			AddRef();
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(const TSharedRef<TOther>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());

		if (this != std::addressof(Other))
		{
			Release();

			RefPtr = Other.RefPtr;
			AddRef();
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(TSharedRef&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Release();

			RefPtr			= Other.RefPtr;
			Other.RefPtr	= nullptr;
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(TSharedRef<TOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());

		if (this != std::addressof(Other))
		{
			Release();

			RefPtr			= Other.RefPtr;
			Other.RefPtr	= nullptr;
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(T* InPtr) noexcept
	{
		if (RefPtr != InPtr)
		{
			Release();
			RefPtr = InPtr;
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, T*>());

		if (RefPtr != InPtr)
		{
			Release();
			RefPtr = InPtr;
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(std::nullptr_t) noexcept
	{
		Release();
		return *this;
	}

private:
	FORCEINLINE void Release() noexcept
	{
		if (RefPtr)
		{
			RefPtr->Release();
			RefPtr = nullptr;
		}
	}

	T* RefPtr;
};

/*
* Casting functions
*/

// static_cast
template<typename T, typename U>
TSharedRef<T> StaticCast(const TSharedRef<U>& Pointer)
{
	T* RawPointer = static_cast<T*>(Pointer.Get());
	RawPointer->AddRef();
	return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
TSharedRef<T> StaticCast(TSharedRef<U>&& Pointer)
{
	T* RawPointer = static_cast<T*>(Pointer.Get());
	return TSharedRef<T>(RawPointer);
}

// const_cast
template<typename T, typename U>
TSharedRef<T> ConstCast(const TSharedRef<U>& Pointer)
{
	T* RawPointer = const_cast<T*>(Pointer.Get());
	RawPointer->AddRef();
	return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
TSharedRef<T> ConstCast(TSharedRef<U>&& Pointer)
{
	T* RawPointer = const_cast<T*>(Pointer.Get());
	return TSharedRef<T>(RawPointer);
}

// reinterpret_cast
template<typename T, typename U>
TSharedRef<T> ReinterpretCast(const TSharedRef<U>& Pointer)
{
	T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
	RawPointer->AddRef();
	return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
TSharedRef<T> ReinterpretCast(TSharedRef<U>&& Pointer)
{
	T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
	return TSharedRef<T>(RawPointer);
}

// dynamic_cast
template<typename T, typename U>
TSharedRef<T> DynamicCast(const TSharedRef<U>& Pointer)
{
	T* RawPointer = dynamic_cast<T*>(Pointer.Get());
	RawPointer->AddRef();
	return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
TSharedRef<T> DynamicCast(TSharedRef<U>&& Pointer)
{
	T* RawPointer = dynamic_cast<T*>(Pointer.Get());
	return TSharedRef<T>(RawPointer);
}

/*
* Converts a rawptr into a TSharedRef
*/

template<typename T, typename U>
TSharedRef<T> MakeSharedRef(U* InRefCountedObject)
{
	if (InRefCountedObject)
	{
		InRefCountedObject->AddRef();
		return TSharedRef<T>(static_cast<T*>(InRefCountedObject));
	}

	return TSharedRef<T>();
}