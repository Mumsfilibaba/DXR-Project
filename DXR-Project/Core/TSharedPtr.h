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
		Counter = new RefCounter();
		Counter->AddRef();

		VALIDATE(Counter != nullptr);
	}

	FORCEINLINE ~TSharedPtr()
	{
		Reset();
	}

	FORCEINLINE void Reset() noexcept
	{
		InternalRelease();
		Ptr = nullptr;
	}

	FORCEINLINE T& operator[](Uint32 Index)
	{
		return Ptr[Index];
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
	RefCounter* Counter;
};

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args... Args)
{
	return Move(TSharedPtr<T>(new T(Args)));
}