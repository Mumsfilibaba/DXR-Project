#pragma once
#include "Core/RefCountedObject.h"

#include <type_traits>

/*
* TSharedRef - Helper class when using objects with RefCountedObject as a base
*/

template<typename TRefCountedObject>
class TSharedRef
{
public:
	template<typename TOther>
	friend class TSharedRef;

	FORCEINLINE TSharedRef() noexcept
		: RefPtr(nullptr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
	}

	FORCEINLINE TSharedRef(const TSharedRef& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		AddRef();
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(const TSharedRef<TOther>& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		AddRef();
	}

	FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		Other.RefPtr = nullptr;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TSharedRef<TOther>&& Other) noexcept
		: RefPtr(Other.RefPtr)
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		Other.RefPtr = nullptr;
	}

	FORCEINLINE TSharedRef(TRefCountedObject* InPtr) noexcept
		: RefPtr(InPtr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TOther* InPtr) noexcept
		: RefPtr(InPtr)
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());
	}

	FORCEINLINE ~TSharedRef()
	{
		Reset();
	}

	FORCEINLINE TRefCountedObject* Reset() noexcept
	{
		TRefCountedObject* WeakPtr = RefPtr;
		InternalRelease();

		return WeakPtr;
	}

	FORCEINLINE void AddRef() noexcept
	{
		if (RefPtr)
		{
			RefPtr->AddRef();
		}
	}

	FORCEINLINE void Swap(TRefCountedObject* InPtr) noexcept
	{
		Reset();
		RefPtr = InPtr;
	}

	template<typename TOther>
	FORCEINLINE void Swap(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		Reset();
		RefPtr = InPtr;
	}

	FORCEINLINE TRefCountedObject* Get() const noexcept
	{
		return RefPtr;
	}

	FORCEINLINE TRefCountedObject* GetAndAddRef() noexcept
	{
		AddRef();
		return RefPtr;
	}

	template<typename TCastType>
	FORCEINLINE TCastType* GetAs() const noexcept
	{
		static_assert(std::is_convertible<TCastType*, TRefCountedObject*>());
		return static_cast<TCastType*>(RefPtr);
	}

	FORCEINLINE TRefCountedObject* const* GetAddressOf() const noexcept
	{
		return &RefPtr;
	}

	FORCEINLINE TRefCountedObject* operator->() const noexcept
	{
		return Get();
	}

	FORCEINLINE TRefCountedObject* const* operator&() const noexcept
	{
		return GetAddressOf();
	}

	FORCEINLINE bool operator==(TRefCountedObject* InPtr) const noexcept
	{
		return (RefPtr == InPtr);
	}

	FORCEINLINE bool operator==(const TSharedRef& Other) const noexcept
	{
		return (RefPtr == Other.RefPtr);
	}

	FORCEINLINE bool operator!=(TRefCountedObject* InPtr) const noexcept
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
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (RefPtr == InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator==(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (RefPtr == Other.RefPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(TOther* InPtr) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (RefPtr != InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

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
			Reset();

			RefPtr = Other.RefPtr;
			AddRef();
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(const TSharedRef<TOther>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		if (this != std::addressof(Other))
		{
			Reset();

			RefPtr = Other.RefPtr;
			AddRef();
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(TSharedRef&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();

			RefPtr			= Other.RefPtr;
			Other.RefPtr	= nullptr;
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(TSharedRef<TOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		if (this != std::addressof(Other))
		{
			Reset();

			RefPtr			= Other.RefPtr;
			Other.RefPtr	= nullptr;
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(TRefCountedObject* InPtr) noexcept
	{
		if (RefPtr != InPtr)
		{
			Reset();
			RefPtr = InPtr;
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		if (RefPtr != InPtr)
		{
			Reset();
			RefPtr = InPtr;
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

private:
	FORCEINLINE void InternalRelease() noexcept
	{
		if (RefPtr)
		{
			RefPtr->Release();
			RefPtr = nullptr;
		}
	}

	TRefCountedObject* RefPtr;
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
	InRefCountedObject->AddRef();
	return TSharedRef<T>(static_cast<T*>(InRefCountedObject));
}