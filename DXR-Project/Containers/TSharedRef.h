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
		: Ptr(nullptr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
	}

	FORCEINLINE TSharedRef(const TSharedRef& Other) noexcept
		: Ptr(Other.Ptr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		AddRef();
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(const TSharedRef<TOther>& Other) noexcept
		: Ptr(Other.Ptr)
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		AddRef();
	}

	FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
		: Ptr(Other.Ptr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		Other.Ptr = nullptr;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TSharedRef<TOther>&& Other) noexcept
		: Ptr(Other.Ptr)
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		Other.Ptr = nullptr;
	}

	FORCEINLINE TSharedRef(TRefCountedObject* InPtr) noexcept
		: Ptr(InPtr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
	}

	template<typename TOther>
	FORCEINLINE TSharedRef(TOther* InPtr) noexcept
		: Ptr(InPtr)
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
		TRefCountedObject* WeakPtr = Ptr;
		InternalRelease();

		return WeakPtr;
	}

	FORCEINLINE void AddRef() noexcept
	{
		if (Ptr)
		{
			Ptr->AddRef();
		}
	}

	FORCEINLINE void Swap(TRefCountedObject* InPtr) noexcept
	{
		Reset();
		Ptr = InPtr;
	}

	template<typename TOther>
	FORCEINLINE void Swap(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		Reset();
		Ptr = InPtr;
	}

	FORCEINLINE TRefCountedObject* Get() const noexcept
	{
		return Ptr;
	}

	FORCEINLINE TRefCountedObject* GetAndAddRef() noexcept
	{
		AddRef();
		return Ptr;
	}

	template<typename TCastType>
	FORCEINLINE TCastType* GetAs() const noexcept
	{
		static_assert(std::is_convertible<TCastType*, TRefCountedObject*>());
		return static_cast<TCastType*>(Ptr);
	}

	FORCEINLINE TRefCountedObject* const* GetAddressOf() const noexcept
	{
		return &Ptr;
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
		return (Ptr == InPtr);
	}

	FORCEINLINE bool operator==(const TSharedRef& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TRefCountedObject* InPtr) const noexcept
	{
		return (Ptr != InPtr);
	}

	FORCEINLINE bool operator!=(const TSharedRef& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(std::nullptr_t) const noexcept
	{
		return (Ptr == nullptr);
	}

	FORCEINLINE bool operator!=(std::nullptr_t) const noexcept
	{
		return (Ptr != nullptr);
	}

	template<typename TOther>
	FORCEINLINE bool operator==(TOther* InPtr) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (Ptr == InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator==(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (Ptr == Other.Ptr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(TOther* InPtr) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (Ptr != InPtr);
	}

	template<typename TOther>
	FORCEINLINE bool operator!=(const TSharedRef<TOther>& Other) const noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		return (Ptr != Other.Ptr);
	}

	FORCEINLINE operator bool() const noexcept
	{
		return (Ptr != nullptr);
	}

	FORCEINLINE TSharedRef& operator=(const TSharedRef& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr = Other.Ptr;
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

			Ptr = Other.Ptr;
			AddRef();
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(TSharedRef&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr = Other.Ptr;
			Other.Ptr = nullptr;
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

			Ptr = Other.Ptr;
			Other.Ptr = nullptr;
		}

		return *this;
	}

	FORCEINLINE TSharedRef& operator=(TRefCountedObject* InPtr) noexcept
	{
		if (Ptr != InPtr)
		{
			Reset();
			Ptr = InPtr;
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedRef& operator=(TOther* InPtr) noexcept
	{
		static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
		static_assert(std::is_base_of<RefCountedObject, TOther>());

		if (Ptr != InPtr)
		{
			Reset();
			Ptr = InPtr;
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
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
	}

	TRefCountedObject* Ptr;
};