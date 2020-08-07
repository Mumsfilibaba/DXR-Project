#pragma once
#include "Defines.h"
#include "Types.h"

#include "Utilities/TUtilities.h"

class RefCounter
{
public:
	FORCEINLINE RefCounter()
		: Counter(0)
	{
	}

	FORCEINLINE ~RefCounter()
	{
	}

	FORCEINLINE Uint32 AddRef()
	{
		return ++Counter;
	}

	FORCEINLINE Uint32 Release()
	{
		VALIDATE(Counter > 0);
		return --Counter;
	}

	FORCEINLINE Uint32 GetRefCount() const
	{
		return Counter;
	}

private:
	Uint32 Counter;
};

template<typename T>
class TSharedPtr
{
public:
	FORCEINLINE TSharedPtr() noexcept
		: Ptr(nullptr)
		, Counter(nullptr)
	{
	}

	FORCEINLINE TSharedPtr(T* InPtr) noexcept
		: Ptr(InPtr)
		, Counter(nullptr)
	{
		if (Ptr)
		{
			Counter = new RefCounter();
			Counter->AddRef();
		}
	}

	FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
		: Ptr(Other.Ptr)
		, Counter(Other.Counter)
	{
		InternalAddRef();
	}

	FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
		: Ptr(Other.Ptr)
		, Counter(Other.Counter)
	{
		Other.Ptr		= nullptr;
		Other.Counter	= nullptr;
	}

	FORCEINLINE ~TSharedPtr()
	{
		Reset();
	}

	FORCEINLINE void Reset() noexcept
	{
		InternalRelease();

		Ptr		= nullptr;
		Counter	= nullptr;
	}

	FORCEINLINE T* Get() const noexcept
	{
		return Ptr;
	}

	FORCEINLINE T* const * GetAddressOf() const noexcept
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
		VALIDATE(Ptr != nullptr);
		return Ptr[Index];
	}

	FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Other)
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr		= Other.Ptr;
			Counter	= Other.Counter;

			InternalAddRef();
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Other)
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr		= Other.Ptr;
			Counter = Other.Counter;

			Other.Ptr		= nullptr;
			Other.Counter	= nullptr;
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(T* InPtr)
	{
		if (Ptr != InPtr)
		{
			Reset();

			Ptr = InPtr;
			if (Ptr)
			{
				Counter = new RefCounter();
				Counter->AddRef();
			}
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	FORCEINLINE bool operator==(const TSharedPtr& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator==(TSharedPtr&& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator==(T* InPtr) const noexcept
	{
		return (Ptr == InPtr);
	}

	FORCEINLINE bool operator!=(const TSharedPtr& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator!=(TSharedPtr&& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator!=(T* InPtr) const noexcept
	{
		return (Ptr != InPtr);
	}

	FORCEINLINE operator bool() const noexcept
	{
		return (Ptr == nullptr);
	}

private:
	FORCEINLINE void InternalRelease() noexcept
	{
		if (Ptr)
		{
			// If there are a Ptr value, we must have a counter or something went wrong
			VALIDATE(Counter != nullptr);

			Counter->Release();
			if (Counter->GetRefCount() <= 0)
			{
				delete Ptr;
				Ptr = nullptr;

				delete Counter;
				Counter = nullptr;
			}
		}
	}

	FORCEINLINE void InternalAddRef() noexcept
	{
		if (Ptr)
		{
			// If there are a Ptr value, we must have a counter or something went wrong
			VALIDATE(Counter != nullptr);
			Counter->AddRef();
		}
	}

	T* Ptr;
	RefCounter* Counter;
};

template<typename T, typename... TArgs>
TSharedPtr<T> MakeShared(TArgs&&... Args)
{
	T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
	return Move(TSharedPtr<T>(RefCountedPtr));
}