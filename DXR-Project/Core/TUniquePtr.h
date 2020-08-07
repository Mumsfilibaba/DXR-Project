#pragma once
#include "Defines.h"
#include "Types.h"

template<typename T>
class TUniquePtr
{
public:
	FORCEINLINE TUniquePtr() noexcept
		: Ptr(nullptr)
	{
	}

	FORCEINLINE TUniquePtr(T* InPtr) noexcept
		: Ptr(InPtr)
	{
	}

	FORCEINLINE TUniquePtr(const TUniquePtr& Other) noexcept
		: Ptr(Other.Ptr)
	{
	}

	FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
		: Ptr(Other.Ptr)
	{
		Other.Ptr = nullptr;
	}

	FORCEINLINE ~TUniquePtr()
	{
		Reset();
	}

	FORCEINLINE void Reset() noexcept
	{
		InternalRelease();
		Ptr = nullptr;
	}

	FORCEINLINE T* Get() const noexcept
	{
		return Ptr;
	}

	FORCEINLINE T* const* GetAddressOf() const noexcept
	{
		return &Ptr;
	}

	FORCEINLINE T* operator->() const noexcept
	{
		return Get();
	}

	FORCEINLINE T& operator*() const noexcept
	{
		return (*Ptr);
	}

	FORCEINLINE T* const* operator&() const noexcept
	{
		return GetAddressOf();
	}

	FORCEINLINE T& operator[](Uint32 Index)
	{
		return Ptr[Index];
	}

	FORCEINLINE TUniquePtr& operator=(const TUniquePtr& Other)
	{
		if (this != std::addressof(Other))
		{
			Ptr = Other.Ptr;
			Counter = Other.Counter;

			InternalAddRef();
		}

		return *this;
	}

	FORCEINLINE TUniquePtr& operator=(TUniquePtr&& Other)
	{
		if (this != std::addressof(Other))
		{
			Ptr = Other.Ptr;
			Counter = Other.Counter;

			Other.Ptr = nullptr;
			Other.Counter = nullptr;
		}

		return *this;
	}

	FORCEINLINE TUniquePtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	FORCEINLINE bool operator==(const TUniquePtr& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator==(T* InPtr) const noexcept
	{
		return (Ptr == InPtr);
	}

	FORCEINLINE operator bool() const noexcept
	{
		return (Ptr == nullptr);
	}

private:
	FORCEINLINE void InternalRelease() noexcept
	{
		Counter->Release();
		if (Counter->GetRefCount() <= 0)
		{
			delete Ptr;
			Ptr = nullptr;

			delete Counter;
			Counter = nullptr;
		}
	}

	FORCEINLINE void InternalAddRef() noexcept
	{
		if (Ptr)
		{
			if (Counter)
			{
				Counter->AddRef();
			}
		}
	}

	T* Ptr;
};

template<typename T, typename... Args>
TUniquePtr<T> MakeUnique(Args... Args)
{
	return Move(TUniquePtr<T>(new T(Args)));
}