#pragma once
#include "TUniquePtr.h"

/*
* Private Refcounter for sharedptr
*/
class SharedPtrRefCounter
{
public:
	FORCEINLINE SharedPtrRefCounter() noexcept
		: Counter(0)
	{
	}

	FORCEINLINE ~SharedPtrRefCounter() noexcept
	{
	}

	FORCEINLINE Uint32 AddRef() noexcept
	{
		return ++Counter;
	}

	FORCEINLINE Uint32 Release() noexcept
	{
		VALIDATE(Counter > 0);
		return --Counter;
	}

	FORCEINLINE Uint32 GetRefCount() const noexcept
	{
		return Counter;
	}

private:
	Uint32 Counter;
};

/*
* TSharedPtr - RefCounted Pointer similar to std::shared_ptr
*/

template<typename T>
class TSharedPtr
{
public:
	template<typename TOther>
	friend class TSharedPtr;

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
			Counter = new SharedPtrRefCounter();
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

	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& Other) noexcept
		: Ptr(Other.Ptr)
		, Counter(Other.Counter)
	{
		InternalAddRef();
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& Other) noexcept
		: Ptr(Other.Ptr)
		, Counter(Other.Counter)
	{
		Other.Ptr		= nullptr;
		Other.Counter	= nullptr;
	}

	FORCEINLINE TSharedPtr(TUniquePtr<T>&& Unique) noexcept
		: Ptr(Unique.Ptr)
		, Counter(nullptr)
	{
		Unique.Ptr = nullptr;

		if (Ptr)
		{
			Counter = new SharedPtrRefCounter();
			Counter->AddRef();
		}
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

	FORCEINLINE void Swap(TSharedPtr& Other) noexcept
	{
		T* TempPtr = Ptr;
		Ptr = Other.Ptr;
		Other.Ptr = TempPtr;
	}

	FORCEINLINE T* Get() const noexcept
	{
		return Ptr;
	}

	FORCEINLINE T* const * GetAddressOf() const noexcept
	{
		return &Ptr;
	}

	FORCEINLINE Uint32 GetRefCount() const noexcept
	{
		return Counter->GetRefCount();
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

	FORCEINLINE T& operator[](Uint32 Index) noexcept
	{
		VALIDATE(Ptr != nullptr);
		return Ptr[Index];
	}

	FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Other) noexcept
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

	FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Other) noexcept
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

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther>& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr = Other.Ptr;
			Counter = Other.Counter;

			InternalAddRef();
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther>&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();

			Ptr = Other.Ptr;
			Counter = Other.Counter;

			Other.Ptr = nullptr;
			Other.Counter = nullptr;
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(TUniquePtr<T>&& Unique) noexcept
	{
		if (Ptr != Unique.Ptr)
		{
			Reset();

			Ptr = Unique.Ptr;
			Unique.Ptr = nullptr;

			if (Ptr)
			{
				Counter = new SharedPtrRefCounter();
				Counter->AddRef();
			}
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(T* InPtr) noexcept
	{
		if (Ptr != InPtr)
		{
			Reset();

			Ptr = InPtr;
			if (Ptr)
			{
				Counter = new SharedPtrRefCounter();
				Counter->AddRef();
			}
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(std::nullptr_t) noexcept
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
		return (Ptr != nullptr);
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
	SharedPtrRefCounter* Counter;
};

// Creates a new object together with a SharedPtr
template<typename T, typename... TArgs>
TSharedPtr<T> MakeShared(TArgs&&... Args) noexcept
{
	T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
	return Move(TSharedPtr<T>(RefCountedPtr));
}