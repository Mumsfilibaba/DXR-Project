#pragma once
#include "RefCountedObject.h"

template<typename TRefCountedObject>
class TSharedRef
{
public:
	FORCEINLINE TSharedRef()
		: Object(nullptr)
	{
	}

	FORCEINLINE TSharedRef(TRefCountedObject* InObject)
		: Object(InObject)
	{
	}

	FORCEINLINE ~TSharedRef()
	{
		Release();
	}

	FORCEINLINE void Reset()
	{
		if (Object)
		{
			Object->Release();
			Object = nullptr;
		}
	}

	FORCEINLINE TRefCountedObject* Release()
	{
		TRefCountedObject* WeakPtr = Object;
		Reset();

		return WeakPtr;
	}

	FORCEINLINE void Swap(TRefCountedObject* Other)
	{
		Reset();
		Object = Other;
	}

	FORCEINLINE TRefCountedObject* Get() const
	{
		return Object;
	}

	FORCEINLINE TRefCountedObject* const* GetAddressOf() const
	{
		return &Object;
	}

	FORCEINLINE TRefCountedObject* operator->() const
	{
		return Get();
	}

	FORCEINLINE TRefCountedObject* const* operator&() const
	{
		return GetAddressOf();
	}

	FORCEINLINE operator bool() const
	{
		return (Object != nullptr);
	}

private:
	TRefCountedObject* Object;
};