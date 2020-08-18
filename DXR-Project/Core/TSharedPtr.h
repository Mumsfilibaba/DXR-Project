#pragma once
#include "TUniquePtr.h"

/*
* Struct Counting references in TWeak- and TSharedPtr
*/
struct PtrControlBlock
{
public:
	typedef Uint32 RefType;

	FORCEINLINE RefType AddWeakRef() noexcept
	{
		return WeakReferences++;
	}

	FORCEINLINE RefType AddStrongRef() noexcept
	{
		return StrongReferences++;
	}

	FORCEINLINE RefType ReleaseWeakRef() noexcept
	{
		return WeakReferences--;
	}

	FORCEINLINE RefType ReleaseStrongRef() noexcept
	{
		return StrongReferences--;
	}

	FORCEINLINE RefType GetWeakReferences() const noexcept
	{
		return WeakReferences;
	}

	FORCEINLINE RefType GetStrongReferences() const noexcept
	{
		return StrongReferences;
	}

private:
	RefType WeakReferences;
	RefType StrongReferences;
};

/*
* Base class for TWeak- and TSharedPtr
*/
template<typename T>
class TPtrBase
{
public:
	template<typename TOther>
	friend class TPtrBase;

	FORCEINLINE T* Get() noexcept
	{
		return Ptr;
	}

	FORCEINLINE const T* Get() const noexcept
	{
		return Ptr;
	}

	FORCEINLINE T* const* GetAddressOf() const noexcept
	{
		return &Ptr;
	}

	FORCEINLINE Uint32 GetStrongReferences() const noexcept
	{
		return Counter ? Counter->GetStrongReferences() : 0;
	}

	FORCEINLINE Uint32 GetWeakReferences() const noexcept
	{
		return Counter ? Counter->GetWeakReferences() : 0;
	}

	FORCEINLINE T* const* operator&() const noexcept
	{
		return GetAddressOf();
	}

	FORCEINLINE T* operator->() const noexcept
	{
		return Get();
	}

	FORCEINLINE T& operator*() const noexcept
	{
		return (*Ptr);
	}

	FORCEINLINE T& operator[](Uint32 Index) const noexcept
	{
		VALIDATE(Ptr != nullptr);
		return Ptr[Index];
	}

	FORCEINLINE bool operator==(T* InPtr) const noexcept
	{
		return (Ptr == InPtr);
	}

	FORCEINLINE bool operator!=(T* InPtr) const noexcept
	{
		return (Ptr != InPtr);
	}

	FORCEINLINE operator bool() const noexcept
	{
		return (Ptr != nullptr);
	}

protected:
	FORCEINLINE TPtrBase() noexcept
		: Ptr(nullptr)
		, Counter(nullptr)
	{
	}

	FORCEINLINE void InternalAddStrongRef() noexcept
	{
		// If the object has a Ptr there must be a Counter or something went wrong
		if (Ptr)
		{
			VALIDATE(Counter != nullptr);
			Counter->AddStrongRef();
		}
	}

	FORCEINLINE void InternalAddWeakRef() noexcept
	{
		// If the object has a Ptr there must be a Counter or something went wrong
		if (Ptr)
		{
			VALIDATE(Counter != nullptr);
			Counter->AddWeakRef();
		}
	}

	FORCEINLINE void InternalReleaseStrongRef() noexcept
	{
		// If the object has a Ptr there must be a Counter or something went wrong
		if (Ptr)
		{
			VALIDATE(Counter != nullptr);
			Counter->ReleaseStrongRef();

			// When releasing the last strong reference we can destroy the pointer and counter
			if (Counter->GetStrongReferences() <= 0)
			{
				delete Ptr;
				delete Counter;
				InternalClear();
			}
		}
	}

	FORCEINLINE void InternalReleaseWeakRef() noexcept
	{
		// If the object has a Ptr there must be a Counter or something went wrong
		if (Ptr)
		{
			VALIDATE(Counter != nullptr);
			Counter->ReleaseWeakRef();
		}
	}

	FORCEINLINE void InternalSwap(TPtrBase& Other) noexcept
	{
		T* TempPtr = Ptr;
		PtrControlBlock* TempBlock = Counter;

		Ptr = Other.Ptr;
		Counter = Other.Counter;

		Other.Ptr = TempPtr;
		Other.Counter = TempBlock;
	}

	FORCEINLINE void InternalMove(TPtrBase&& Other) noexcept
	{
		Ptr = Other.Ptr;
		Counter = Other.Counter;

		Other.Ptr = nullptr;
		Other.Counter = nullptr;
	}

	template<typename TOther>
	FORCEINLINE void InternalMove(TPtrBase<TOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr = static_cast<TOther*>(Other.Ptr);
		Counter = Other.Counter;

		Other.Ptr = nullptr;
		Other.Counter = nullptr;
	}

	FORCEINLINE void InternalConstructStrong(T* InPtr)
	{
		Ptr = InPtr;
		Counter = new PtrControlBlock();
		InternalAddStrongRef();
	}

	template<typename TOther>
	FORCEINLINE void InternalConstructStrong(TOther* InPtr)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr = static_cast<T*>(InPtr);
		Counter = new PtrControlBlock();
		InternalAddStrongRef();
	}

	FORCEINLINE void InternalConstructStrong(const TPtrBase& Other)
	{
		Ptr = Other.Ptr;
		Counter = Other.Counter;
		InternalAddStrongRef();
	}

	template<typename TOther>
	FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther>& Other)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr = static_cast<T*>(Other.Ptr);
		Counter = Other.Counter;
		InternalAddStrongRef();
	}

	FORCEINLINE void InternalConstructWeak(T* InPtr)
	{
		Ptr = InPtr;
		Counter = new PtrControlBlock();
		InternalAddWeakRef();
	}

	template<typename TOther>
	FORCEINLINE void InternalConstructWeak(TOther* InPtr)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr = static_cast<T*>(InPtr);
		Counter = new PtrControlBlock();
		InternalAddWeakRef();
	}

	FORCEINLINE void InternalConstructWeak(const TPtrBase& Other)
	{
		Ptr = Other.Ptr;
		Counter = Other.Counter;
		InternalAddWeakRef();
	}

	template<typename TOther>
	FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther>& Other)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr = static_cast<T*>(Other.Ptr);
		Counter = Other.Counter;
		InternalAddWeakRef();
	}

	FORCEINLINE void InternalDestructWeak()
	{
		InternalReleaseWeakRef();
		InternalClear();
	}

	FORCEINLINE void InternalDestructStrong()
	{
		InternalReleaseStrongRef();
		InternalClear();
	}

	FORCEINLINE void InternalClear() noexcept
	{
		Ptr = nullptr;
		Counter = nullptr;
	}

	T* Ptr;
	PtrControlBlock* Counter;
};

/*
* Forward Declarations
*/
template<typename TOther>
class TWeakPtr;

/*
* TSharedPtr - RefCounted Pointer similar to std::shared_ptr
*/
template<typename T>
class TSharedPtr : public TPtrBase<T>
{
	using TBase = TPtrBase<T>;

public:
	FORCEINLINE TSharedPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE explicit TSharedPtr(T* InPtr) noexcept
		: TBase()
	{
		TBase::InternalConstructStrong(InPtr);
	}

	FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructStrong(Other);
	}

	FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
		: TBase()
	{
		TBase::InternalMove(Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::InternalConstructStrong<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::InternalMove<TOther>(Move(Other));
	}

	template<typename TOther>
	FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::InternalConstructStrong<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE explicit TSharedPtr(TUniquePtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::InternalConstructStrong<TOther>(Other.Release());
	}

	FORCEINLINE ~TSharedPtr()
	{
		Reset();
	}

	FORCEINLINE void Reset() noexcept
	{
		TBase::InternalDestructStrong();
	}

	FORCEINLINE void Swap(TSharedPtr& Other) noexcept
	{
		TBase::InternalSwap(Other);
	}

	FORCEINLINE bool IsUnique() const noexcept
	{
		return (TBase::GetStrongReferences() == 1);
	}

	FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalConstructStrong(Other);
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalMove(Move(Other));
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalConstructStrong<TOther>(Other);
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalMove<TOther>(Move(Other));
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(T* InPtr) noexcept
	{
		if (Ptr != InPtr)
		{
			Reset();
			TBase::InternalConstructStrong(InPtr);
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

	FORCEINLINE bool operator!=(const TSharedPtr& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TSharedPtr&& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TSharedPtr&& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}
};

/*
* TWeakPtr - Weak Pointer similar to std::weak_ptr
*/
template<typename T>
class TWeakPtr : public TPtrBase<T>
{
	using TBase = TPtrBase<T>;

public:
	FORCEINLINE TWeakPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE TWeakPtr(const TSharedPtr<T>& InPtr) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(InPtr);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TSharedPtr<TOther>& InPtr) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::InternalConstructWeak<TOther>(InPtr);
	}

	FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(Other);
	}

	FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
		: TBase()
	{
		TBase::InternalMove(Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TWeakPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::InternalConstructWeak<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(TWeakPtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::InternalMove<TOther>(Move(Other));
	}

	FORCEINLINE ~TWeakPtr()
	{
		Reset();
	}

	FORCEINLINE void Reset() noexcept
	{
		TBase::InternalDestructWeak();
	}

	FORCEINLINE void Swap(TWeakPtr& Other) noexcept
	{
		TBase::InternalSwap(Other);
	}

	FORCEINLINE bool IsExpired() const noexcept
	{
		return (TBase::GetStrongReferences() < 1);
	}

	FORCEINLINE TSharedPtr<T> MakeShared() noexcept
	{
		const TWeakPtr& This = *this;
		return Move(TSharedPtr<T>(This));
	}

	FORCEINLINE TWeakPtr& operator=(const TWeakPtr& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalConstructWeak(Other);
		}

		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(TWeakPtr&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalMove(Move(Other));
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalConstructWeak<TOther>(Other);
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalMove(Move(Other));
		}

		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(T* InPtr) noexcept
	{
		if (Ptr != InPtr)
		{
			Reset();
			TBase::InternalConstructWeak(InPtr);
		}

		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	FORCEINLINE bool operator==(const TWeakPtr& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(const TWeakPtr& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TWeakPtr&& Other) const noexcept
	{
		return (Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TWeakPtr&& Other) const noexcept
	{
		return (Ptr != Other.Ptr);
	}
};

/*
* Creates a new object together with a SharedPtr
*/
template<typename T, typename... TArgs>
TSharedPtr<T> MakeShared(TArgs&&... Args) noexcept
{
	T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
	return Move(TSharedPtr<T>(RefCountedPtr));
}