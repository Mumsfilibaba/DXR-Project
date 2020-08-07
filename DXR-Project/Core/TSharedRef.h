#pragma once
#include "RefCountedObject.h"

#include <type_traits>

template<typename TRefCountedObject>
class TSharedRef
{
public:
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

	FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
		: Ptr(Other.Ptr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		
		Other.Ptr = nullptr;
	}

	FORCEINLINE TSharedRef(TRefCountedObject* InPtr) noexcept
		: Ptr(InPtr)
	{
		static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
	}
	
	FORCEINLINE ~TSharedRef()
	{
		Reset();
	}

	FORCEINLINE TRefCountedObject* Reset() noexcept
	{
		TRefCountedObject* WeakPtr = Ptr;
		InternalRelease();

		return Ptr;
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

	FORCEINLINE TRefCountedObject* Get() const noexcept
	{
		return Ptr;
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

	FORCEINLINE bool operator==(const TSharedRef& Other) const noexcept
	{
		return (Ptr == other.Ptr);
	}

	FORCEINLINE bool operator==(const TSharedRef& Other) const noexcept
	{
		return (Ptr != other.Ptr);
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

	FORCEINLINE TSharedRef& operator=(TRefCountedObject* InPtr) noexcept
	{
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
	FORCEINLINE void InternalRelease()
	{
		if (Ptr)
		{ 
			Ptr->Release();
			Ptr = nullptr;
		}
	}

	TRefCountedObject* Ptr;
};