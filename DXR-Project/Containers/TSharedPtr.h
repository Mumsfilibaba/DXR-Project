#pragma once
#include "TUniquePtr.h"

/*
* Struct Counting references in TWeak- and TSharedPtr
*/
struct PtrControlBlock
{
public:
	typedef Uint32 RefType;

	inline PtrControlBlock()
		: WeakReferences(0)
		, StrongReferences(0)
	{
	}

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
* TDelete
*/
template<typename T>
struct TDelete
{
	using TType = T;

	FORCEINLINE void operator()(TType* Ptr)
	{
		delete Ptr;
	}
};

template<typename T>
struct TDelete<T[]>
{
	using TType = TRemoveExtent<T>;

	FORCEINLINE void operator()(TType* Ptr)
	{
		delete[] Ptr;
	}
};

/*
* Base class for TWeak- and TSharedPtr
*/
template<typename T, typename D>
class TPtrBase
{
public:
	template<typename TOther, typename DOther>
	friend class TPtrBase;

	FORCEINLINE T* Get() const noexcept
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
		static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
		static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
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
				if (Counter->GetWeakReferences() <= 0)
				{
					delete Counter;
				}
				
				Deleter(Ptr);
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
			
			PtrControlBlock::RefType StrongRefs = Counter->GetStrongReferences();
			PtrControlBlock::RefType WeakRefs 	= Counter->GetWeakReferences();
			if (WeakRefs <= 0 && StrongRefs <= 0)
			{
				delete Counter;
			}
		}
	}

	FORCEINLINE void InternalSwap(TPtrBase& Other) noexcept
	{
		T* TempPtr = Ptr;
		PtrControlBlock* TempBlock = Counter;

		Ptr		= Other.Ptr;
		Counter	= Other.Counter;

		Other.Ptr		= TempPtr;
		Other.Counter	= TempBlock;
	}

	FORCEINLINE void InternalMove(TPtrBase&& Other) noexcept
	{
		Ptr		= Other.Ptr;
		Counter	= Other.Counter;

		Other.Ptr		= nullptr;
		Other.Counter	= nullptr;
	}

	template<typename TOther, typename DOther>
	FORCEINLINE void InternalMove(TPtrBase<TOther, DOther>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr		= static_cast<TOther*>(Other.Ptr);
		Counter	= Other.Counter;

		Other.Ptr		= nullptr;
		Other.Counter	= nullptr;
	}

	FORCEINLINE void InternalConstructStrong(T* InPtr)
	{
		Ptr		= InPtr;
		Counter	= new PtrControlBlock();
		InternalAddStrongRef();
	}

	template<typename TOther, typename DOther>
	FORCEINLINE void InternalConstructStrong(TOther* InPtr)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr		= static_cast<T*>(InPtr);
		Counter = new PtrControlBlock();
		InternalAddStrongRef();
	}

	FORCEINLINE void InternalConstructStrong(const TPtrBase& Other)
	{
		Ptr		= Other.Ptr;
		Counter	= Other.Counter;
		InternalAddStrongRef();
	}

	template<typename TOther, typename DOther>
	FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther, DOther>& Other)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr		= static_cast<T*>(Other.Ptr);
		Counter	= Other.Counter;
		InternalAddStrongRef();
	}
	
	template<typename TOther, typename DOther>
	FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther, DOther>& Other, T* InPtr)
	{
		Ptr		= InPtr;
		Counter	= Other.Counter;
		InternalAddStrongRef();
	}
	
	template<typename TOther, typename DOther>
	FORCEINLINE void InternalConstructStrong(TPtrBase<TOther, DOther>&& Other, T* InPtr)
	{
		Ptr		= InPtr;
		Counter	= Other.Counter;
		Other.Ptr		= nullptr;
		Other.Counter	= nullptr;
	}

	FORCEINLINE void InternalConstructWeak(T* InPtr)
	{
		Ptr		= InPtr;
		Counter	= new PtrControlBlock();
		InternalAddWeakRef();
	}

	template<typename TOther>
	FORCEINLINE void InternalConstructWeak(TOther* InPtr)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr		= static_cast<T*>(InPtr);
		Counter = new PtrControlBlock();
		InternalAddWeakRef();
	}

	FORCEINLINE void InternalConstructWeak(const TPtrBase& Other)
	{
		Ptr		= Other.Ptr;
		Counter = Other.Counter;
		InternalAddWeakRef();
	}

	template<typename TOther, typename DOther>
	FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther, DOther>& Other)
	{
		static_assert(std::is_convertible<TOther, T>());

		Ptr		= static_cast<T*>(Other.Ptr);
		Counter	= Other.Counter;
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
		Ptr		= nullptr;
		Counter = nullptr;
	}

protected:
	T* Ptr;
	PtrControlBlock* Counter;
	D Deleter;
};

/*
* Forward Declarations
*/
template<typename TOther>
class TWeakPtr;

/*
* TSharedPtr - RefCounted Scalar Pointer
*/
template<typename T>
class TSharedPtr : public TPtrBase<T, TDelete<T>>
{
	using TBase = TPtrBase<T, TDelete<T>>;

public:
	FORCEINLINE TSharedPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE TSharedPtr(std::nullptr_t) noexcept
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
		TBase::InternalMove(::Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalMove<TOther>(::Move(Other));
	}
	
	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& Other, T* InPtr) noexcept
		: TBase()
	{
		TBase::template InternalConstructStrong<TOther>(Other, InPtr);
	}
	
	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& Other, T* InPtr) noexcept
		: TBase()
	{
		TBase::template InternalConstructStrong<TOther>(::Move(Other), InPtr);
	}

	template<typename TOther>
	FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TUniquePtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther, TDelete<T>>(Other.Release());
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

	FORCEINLINE T* operator->() const noexcept
	{
		return TBase::Get();
	}
	
	FORCEINLINE T& operator*() const noexcept
	{
		VALIDATE(TBase::Ptr != nullptr);
		return *TBase::Ptr;
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
			TBase::InternalMove(::Move(Other));
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
			TBase::template InternalConstructStrong<TOther>(Other);
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
			TBase::template InternalMove<TOther>(::Move(Other));
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(T* InPtr) noexcept
	{
		if (this->Ptr != InPtr)
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
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(const TSharedPtr& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TSharedPtr&& Other) const noexcept
	{
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TSharedPtr&& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}
};

/*
* TSharedPtr - RefCounted Pointer for array types
*/
template<typename T>
class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
	using TBase = TPtrBase<T, TDelete<T[]>>;

public:
	FORCEINLINE TSharedPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE TSharedPtr(std::nullptr_t) noexcept
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
		TBase::InternalMove(::Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther[]>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther>(Other);
	}
	
	template<typename TOther>
	FORCEINLINE TSharedPtr(const TSharedPtr<TOther[]>& Other, T* InPtr) noexcept
		: TBase()
	{
		TBase::template InternalConstructStrong<TOther>(Other, InPtr);
	}
	
	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther[]>&& Other, T* InPtr) noexcept
		: TBase()
	{
		TBase::template InternalConstructStrong<TOther>(::Move(Other), InPtr);
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TSharedPtr<TOther[]>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalMove<TOther>(::Move(Other));
	}

	template<typename TOther>
	FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther[]>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr(TUniquePtr<TOther[]>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>());
		TBase::template InternalConstructStrong<TOther>(Other.Release());
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

	FORCEINLINE T& operator[](Uint32 Index) noexcept
	{
		VALIDATE(TBase::Ptr != nullptr);
		return TBase::Ptr[Index];
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
			TBase::InternalMove(::Move(Other));
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther[]>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::template InternalConstructStrong<TOther>(Other);
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther[]>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>());

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::template InternalMove<TOther>(::Move(Other));
		}

		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(T* InPtr) noexcept
	{
		if (this->Ptr != InPtr)
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
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(const TSharedPtr& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TSharedPtr&& Other) const noexcept
	{
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TSharedPtr&& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}
};

/*
* TWeakPtr - Weak Pointer for scalar types
*/
template<typename T>
class TWeakPtr : public TPtrBase<T, TDelete<T>>
{
	using TBase = TPtrBase<T, TDelete<T>>;

public:
	FORCEINLINE TWeakPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE TWeakPtr(const TSharedPtr<T>& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(Other);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TSharedPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalConstructWeak<TOther>(Other);
	}

	FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(Other);
	}

	FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
		: TBase()
	{
		TBase::InternalMove(::Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TWeakPtr<TOther>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalConstructWeak<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(TWeakPtr<TOther>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalMove<TOther>(::Move(Other));
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
		return ::Move(TSharedPtr<T>(This));
	}

	FORCEINLINE T* operator->() const noexcept
	{
		return TBase::Get();
	}
	
	FORCEINLINE T& operator*() const noexcept
	{
		VALIDATE(TBase::Ptr != nullptr);
		return *TBase::Ptr;
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
			TBase::InternalMove(::Move(Other));
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
			TBase::template InternalConstructWeak<TOther>(Other);
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
			TBase::InternalMove(::Move(Other));
		}

		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(T* InPtr) noexcept
	{
		if (TBase::Ptr != InPtr)
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
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(const TWeakPtr& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TWeakPtr&& Other) const noexcept
	{
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TWeakPtr&& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}
};

/*
* TWeakPtr - Weak Pointer for scalar types
*/
template<typename T>
class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
	using TBase = TPtrBase<T, TDelete<T[]>>;

public:
	FORCEINLINE TWeakPtr() noexcept
		: TBase()
	{
	}

	FORCEINLINE TWeakPtr(const TSharedPtr<T>& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(Other);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TSharedPtr<TOther[]>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalConstructWeak<TOther>(Other);
	}

	FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
		: TBase()
	{
		TBase::InternalConstructWeak(Other);
	}

	FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
		: TBase()
	{
		TBase::InternalMove(::Move(Other));
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(const TWeakPtr<TOther[]>& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalConstructWeak<TOther>(Other);
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr(TWeakPtr<TOther[]>&& Other) noexcept
		: TBase()
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
		TBase::template InternalMove<TOther>(::Move(Other));
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

	FORCEINLINE TSharedPtr<T[]> MakeShared() noexcept
	{
		const TWeakPtr& This = *this;
		return ::Move(TSharedPtr<T[]>(This));
	}
	
	FORCEINLINE T& operator[](Uint32 Index) noexcept
	{
		VALIDATE(TBase::Ptr != nullptr);
		return TBase::Ptr[Index];
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
			TBase::InternalMove(::Move(Other));
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther[]>& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::template InternalConstructWeak<TOther>(Other);
		}

		return *this;
	}

	template<typename TOther>
	FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther[]>&& Other) noexcept
	{
		static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

		if (this != std::addressof(Other))
		{
			Reset();
			TBase::InternalMove(::Move(Other));
		}

		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(T* InPtr) noexcept
	{
		if (TBase::Ptr != InPtr)
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
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(const TWeakPtr& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}

	FORCEINLINE bool operator==(TWeakPtr&& Other) const noexcept
	{
		return (TBase::Ptr == Other.Ptr);
	}

	FORCEINLINE bool operator!=(TWeakPtr&& Other) const noexcept
	{
		return (TBase::Ptr != Other.Ptr);
	}
};

/*
* Creates a new object together with a SharedPtr
*/
template<typename T, typename... TArgs>
std::enable_if_t<!std::is_array_v<T>, TSharedPtr<T>> MakeShared(TArgs&&... Args) noexcept
{
	T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
	return ::Move(TSharedPtr<T>(RefCountedPtr));
}

template<typename T>
std::enable_if_t<std::is_array_v<T>, TSharedPtr<T>> MakeShared(Uint32 Size) noexcept
{
	using TType = TRemoveExtent<T>;

	TType* RefCountedPtr = new TType[Size];
	return ::Move(TSharedPtr<T>(RefCountedPtr));
}

/*
* Casting functions
*/

// static_cast
template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> StaticCast(const TSharedPtr<T1>& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = static_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> StaticCast(TSharedPtr<T1>&& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = static_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// const_cast
template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ConstCast(const TSharedPtr<T1>& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = const_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ConstCast(TSharedPtr<T1>&& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = const_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// reinterpret_cast
template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ReinterpretCast(const TSharedPtr<T1>& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ReinterpretCast(TSharedPtr<T1>&& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// dynamic_cast
template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> DynamicCast(const TSharedPtr<T1>& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> DynamicCast(TSharedPtr<T1>&& Pointer)
{
	using TType = TRemoveExtent<T0>;
	
	TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
	return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}
