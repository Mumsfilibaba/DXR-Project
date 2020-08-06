#pragma once
#include "Defines.h"
#include "Types.h"

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
	FORCEINLINE TSharedPtr()
		: Ptr(nullptr)
		, Counter(nullptr)
	{
	}

	FORCEINLINE TSharedPtr(T* InPtr)
		: Ptr(InPtr)
		, Counter(nullptr)
	{
		Counter = new RefCounter();
		Counter->AddRef();
	}

private:
	RefCounter* Counter;
	T* Ptr;
};